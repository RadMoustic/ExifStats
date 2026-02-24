#ifdef IMAGETAGGER_ENABLE

#include <ESImageTaggerManager.h>

// ExifStats
#include "ESImageTagger.h"
#include "ESDatabase.h"
#include "ESImageCache.h"

// Qt
#include <QDir>
#include <QFile>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtConcurrent>

// Stl
#include <set>
#include <variant>

/********************************************************************************/

/*static*/ ESImageTaggerManager& ESImageTaggerManager::getInstance()
{
	static ESImageTaggerManager lsInstance;
	return lsInstance;
}

/********************************************************************************/

ESImageTaggerManager::ESImageTaggerManager()
	: mMaxSizeOfAllTaggerInputs(0)
	, mEnabled(false)
{
	mMaxAsyncTask = QThreadPool::globalInstance()->maxThreadCount();

	mTaggerDirectoryPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/ImageTaggers");
	mEnabled = QDir(mTaggerDirectoryPath).exists();
	if (!mEnabled)
	{
		qWarning() << "Image Tagger directory '" << mTaggerDirectoryPath << "' does not exist. Image Tagging is disabled.";
	}
}

/********************************************************************************/

/*virtual*/ ESImageTaggerManager::~ESImageTaggerManager()
{

}

/********************************************************************************/

bool ESImageTaggerManager::isEnabled() const
{
	return mEnabled;
}

/********************************************************************************/

void ESImageTaggerManager::initialize()
{
	if(!mEnabled)
		return;

	loadTaggersFromDirectory(mTaggerDirectoryPath);
	updateAllTagLabels();

	connect(&ESImageCache::getInstance(), &ESImageCache::imageLoadingProgress, this, &ESImageTaggerManager::onImageCacheLoadingProgress);
	connect(&ESImageCache::getInstance(), &ESImageCache::updateFinished, this, &ESImageTaggerManager::onImageCacheUpdateFinished);

	updateDatabaseMissingTags();
}

/********************************************************************************/

void ESImageTaggerManager::retag()
{
	ESDatabase& lDB = ESDatabase::getInstance();
	{
		std::scoped_lock lLock(lDB.mFilesMutex);
		lDB.mAllTags.clear();
		for (auto&& lFileInfo : lDB.mFiles)
		{
			lFileInfo.second.mTagsGenerated = false;
			lFileInfo.second.mTagIndexes.clear();
		}
	}

	updateAllTagLabels();

	{
		std::scoped_lock lLock(lDB.mFilesMutex);
		for (auto&& lFileInfo : lDB.mFiles)
		{
			std::shared_ptr<ESImage> lImage = ESImageCache::getInstance().getImage(lFileInfo.first);
			queueImageLoading(lImage);
		}
	}
}

/********************************************************************************/

void ESImageTaggerManager::loadTaggersFromDirectory(const QString& pDirectoryPath)
{
	QDir lDir(pDirectoryPath);
	if (!lDir.exists())
	{
		qWarning() << "Tagger directory '" << pDirectoryPath << "' does not exist.";
		return;
	}
	QStringList lTaggerFilePaths = lDir.entryList(QStringList() << "*.estagger", QDir::Files);
	for (const QString& lTaggerFilePath : lTaggerFilePaths)
	{
		addTagger(lDir.filePath(lTaggerFilePath));
	}
}

/********************************************************************************/

void ESImageTaggerManager::addTagger(std::shared_ptr<ESImageTagger> pTagger)
{
	mTaggers.emplace_back(pTagger);
	mMaxSizeOfAllTaggerInputs = std::max(mMaxSizeOfAllTaggerInputs, std::max(pTagger->getFormat()->mInputWidth, pTagger->getFormat()->mInputHeight));
}

/********************************************************************************/

void ESImageTaggerManager::addTagger(const QString& pTaggerFilePath)
{
	QFile lFile(pTaggerFilePath);
	if (!lFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "Failed to open tagger file '" << pTaggerFilePath << "' with error: " << lFile.errorString();
		return;
	}

	QByteArray lJsonData = lFile.readAll();
	lFile.close();
	QJsonParseError lJsonParseError;
	QJsonDocument lJsonDoc = QJsonDocument::fromJson(lJsonData, &lJsonParseError);
	if (lJsonDoc.isNull())
	{
		qWarning() << "Failed to parse JSON Tagger file '" << pTaggerFilePath << "': char " << lJsonParseError.offset << " : " << lJsonParseError.errorString() << "";
		return;
	}

	QJsonObject lRoot = lJsonDoc.object();

	static const char* scEnabledKey = "Enabled";
	if (lRoot.contains(scEnabledKey))
	{
		if(!lRoot.value(scEnabledKey).isBool())
		{
			qWarning() << "Tagger file '" << pTaggerFilePath << "' does not contain correct bool value '" << scEnabledKey << "'";
			return;
		}
		if(!lRoot.value(scEnabledKey).toBool())
			return;
	}

	static const char* scModelPathKey = "ModelPath";
	if (!lRoot.contains(scModelPathKey) || !lRoot.value(scModelPathKey).isString())
	{
		qWarning() << "Tagger file '" << pTaggerFilePath << "' does not contain required string value '" << scModelPathKey << "'";
		return;
	}
	QString lModelPath = lRoot[scModelPathKey].toString();
	if(!getFilePathFromBase(lModelPath, pTaggerFilePath, lModelPath))
	{
		qWarning() << "Model file '" << lModelPath << "' specified in tagger file '" << pTaggerFilePath << "' does not exist.";
		return;
	}

	static const char* scFormatTypeKey = "FormatType";
	if (!lRoot.contains(scFormatTypeKey) || !lRoot.value(scFormatTypeKey).isDouble())
	{
		qWarning() << "Tagger file '" << pTaggerFilePath << "' does not contain required int value '" << scFormatTypeKey << "'";
		return;
	}
	int lFormatType = lRoot[scFormatTypeKey].toInt();

	std::shared_ptr<ESImageTagger::Format> lFormat = ESImageTagger::CreateFormatFromType(ESImageTagger::FormatType(lFormatType));
	if (!lFormat)
	{
		qWarning() << "Tagger file '" << pTaggerFilePath << "' contains invalid format type '" << lFormatType << "'";
		return;
	}

	static const char* scFormatKey = "Format";
	if (!lRoot.contains(scFormatKey) || !lRoot.value(scFormatKey).isObject())
	{
		qWarning() << "Tagger file '" << pTaggerFilePath << "' does not contain required object '" << scFormatKey << "'";
		return;
	}
	QJsonObject lFormatJson = lRoot[scFormatKey].toObject();
	
	if(!lFormat->loadFromJSON(lFormatJson, pTaggerFilePath))
	{
		qWarning() << "Failed to load format from JSON in tagger file '" << pTaggerFilePath << "'";
		return;
	}

	std::shared_ptr<ESImageTagger> lTagger = std::make_shared<ESImageTagger>(lModelPath, lFormat);
	
	addTagger(lTagger);
}

/********************************************************************************/

QVector<uint16_t> ESImageTaggerManager::generateImageTags(const QImage& pImage)
{
	QVector<uint16_t> lAllTagsVector;
	if(!pImage.isNull())
	{
		float lImageRatio = float(pImage.width()) / float(pImage.height());
		int lWidth = lImageRatio > 1.0f ? mMaxSizeOfAllTaggerInputs * lImageRatio : mMaxSizeOfAllTaggerInputs;
		int lHeight = lImageRatio < 1.0f ? mMaxSizeOfAllTaggerInputs / lImageRatio : mMaxSizeOfAllTaggerInputs;
		QImage lMaxInputResizedImage = pImage.scaled(lWidth, lHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

		std::unordered_set<int> lAllTags;
		for (const std::shared_ptr<ESImageTagger>& lTagger : mTaggers)
		{
			QVector<uint16_t> lTaggerTags = lTagger->generateImageTags(lMaxInputResizedImage);
			convertTagsToAllTagIndexes(lTaggerTags, lTagger.get());
			lAllTags.insert(lTaggerTags.begin(), lTaggerTags.end());
		}

		lAllTagsVector = std::move(QVector<uint16_t>(lAllTags.begin(), lAllTags.end()));
	}
	
	return lAllTagsVector;
}

/********************************************************************************/

void ESImageTaggerManager::updateAllTagLabels()
{
	mAllTagLabels.clear();
	mTaggerTagIndexesInAllLabels.clear();

	std::set<QString> lUniqueLabels;
	for (const std::shared_ptr<ESImageTagger>& lTagger : mTaggers)
		for (const QString& lLabel : lTagger->getFormat()->mLabels)
			lUniqueLabels.insert(lLabel);

	for(const QString& lUniqueLabel: lUniqueLabels)
		mAllTagLabels.push_back(lUniqueLabel);

	for (const std::shared_ptr<ESImageTagger>& lTagger : mTaggers)
	{
		QVector<uint16_t>& lTagIndexesInAllLabels = mTaggerTagIndexesInAllLabels[lTagger.get()];
		for (const QString& lLabel : lTagger->getFormat()->mLabels)
		{
			auto lItFound = lUniqueLabels.find(lLabel);
			lTagIndexesInAllLabels.push_back(int(std::distance(lUniqueLabels.begin(), lItFound)));
		}
	}

	ESDatabase& lDB = ESDatabase::getInstance();
	if (lDB.mAllTags != mAllTagLabels)
	{
		std::scoped_lock lLock(lDB.mFilesMutex);
		std::unordered_map<QString, int> lTagLabelToNewIndex;
		for (int i = 0; i < lDB.mAllTags.count(); ++i)
		{
			int lNewIndex = mAllTagLabels.indexOf(lDB.mAllTags[i]);

			if (lNewIndex < 0)
			{
				mAllTagLabels.push_back(lDB.mAllTags[i]);
				lNewIndex = mAllTagLabels.size() - 1;
			}

			lTagLabelToNewIndex[lDB.mAllTags[i]] = lNewIndex;
		}

		for (auto& lFileInfo : lDB.mFiles)
		{
			for (uint16_t& lTagIndex : lFileInfo.second.mTagIndexes)
			{
				if (lTagIndex >= 0 && lTagIndex < lDB.mAllTags.size())
				{
					const QString& lTagLabel = lDB.mAllTags[lTagIndex];
					lTagIndex = lTagLabelToNewIndex[lTagLabel];
				}
				else
				{
					assert(false && "Tag index out of bounds for database all tag labels");
				}
			}
		}

		lDB.setAllTags(mAllTagLabels);
	}
}

/********************************************************************************/

void ESImageTaggerManager::convertTagsToAllTagIndexes(QVector<uint16_t>& pTaggerTags, ESImageTagger* pTagger)
{
	QVector<uint16_t>& lTagIndexesInAllLabels = mTaggerTagIndexesInAllLabels[pTagger];
	for (uint16_t& lTagIndex : pTaggerTags)
	{
		if (lTagIndex >= 0 && lTagIndex < lTagIndexesInAllLabels.size())
		{
			lTagIndex = lTagIndexesInAllLabels[lTagIndex];
		}
		else
		{
			assert(false && "Tag index out of bounds for tagger format labels");
		}
	}
}

/********************************************************************************/

QStringList ESImageTaggerManager::getTagsLabels(const QVector<uint16_t>& pTags)
{
	QStringList lTagLabels;
	for (int lTagIndex : pTags)
	{
		if (lTagIndex >= 0 && lTagIndex < mAllTagLabels.size())
		{
			lTagLabels.push_back(mAllTagLabels[lTagIndex]);
		}
		else
		{
			assert(false && "Tag index out of bounds for all tag labels");
		}
	}
	return lTagLabels;
}

/********************************************************************************/

/*virtual*/ void ESImageTaggerManager::internalLoadImage(const std::shared_ptr<ESImage>& pImage, std::atomic_int32_t& pNumAsyncTaskStarted) /*override*/
{
	++pNumAsyncTaskStarted;
	QtConcurrent::run([this, pImage, &pNumAsyncTaskStarted]()
	{
		if(mPaused)
		{
			--pNumAsyncTaskStarted;
			queueImageLoading(pImage);
		}
		else
		{
			QImage lImage(pImage->getImagePath().getString());
			assert(!lImage.isNull());
			QVector<uint16_t> lTags = generateImageTags(lImage);
			ESDatabase& lDB = ESDatabase::getInstance();
			std::scoped_lock lLock(lDB.mFilesMutex);
			FileInfo& lFileInfo = lDB.mFiles[pImage->getImagePath()];
			lFileInfo.mTagIndexes = std::move(lTags);
			lFileInfo.mTagsGenerated = true;
			--pNumAsyncTaskStarted;
			imageLoadingFinished();
		}
	});
}

/********************************************************************************/

void ESImageTaggerManager::updateDatabaseMissingTags()
{
	if(ESImageCache::getInstance().isUpdating())
		return;
	ESDatabase& lDB = ESDatabase::getInstance();
	std::scoped_lock lLock(lDB.mFilesMutex);
	for(auto&& lFileInfo : lDB.mFiles)
	{
		if(!lFileInfo.second.mTagsGenerated)
		{
			std::shared_ptr<ESImage> lImage = ESImageCache::getInstance().getImage(lFileInfo.first);
			queueImageLoading(lImage);
		}
	}
}

/********************************************************************************/

void ESImageTaggerManager::onImageCacheLoadingProgress(int pCachedCount, int pCachingCount)
{
	if(pCachedCount == pCachingCount)
	{
		updateDatabaseMissingTags();
	}
	else
	{
		stopAndCancelAllLoadings();
	}
}

/********************************************************************************/

void ESImageTaggerManager::onImageCacheUpdateFinished()
{
	updateDatabaseMissingTags();
}

#endif // IMAGETAGGER_ENABLE