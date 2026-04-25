#include "ESTagsFilter.h"

// ES
#include "ESDatabase.h"
#include "ESImageCache.h"
#include "ESPerfLog.h"
#include "ESImageTaggerManager.h"

// Hnswlib
#ifdef HNSWLIB_ENABLED
#pragma warning( push, 0 )
#pragma warning( disable : 4701 )
#include "hnswlib/hnswlib.h"
#pragma warning( pop )
#endif // HNSWLIB_ENABLED

// Quazip
#ifdef Q_OS_ANDROID
#include <quazip/quazip.h>
#include <quazip/quazipfile.h>
#endif // Q_OS_ANDROID

// Qt
#include <QtConcurrent>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

/*static*/ const char* ESTagsFilter::msTokenizerFolderSettingsKey = "TokenizerFolderPath";

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

ESTagsFilter::ESTagsFilter()
	: mMinSimilarityScore(0.25f)
#ifdef IMAGETAGGER_ENABLE
	, mTokenizerEnabled(false)
#ifdef HNSWLIB_ENABLED
	, mUpdatingHNSWIndex(false)
	, mUpdatingHNSWIndexProgress(1.f)
#endif // HNSWLIB_ENABLED
#endif // IMAGETAGGER_ENABLE
{
#ifdef IMAGETAGGER_ENABLE
	loadTokenizerAndHNSW();
#endif // IMAGETAGGER_ENABLE
}

/********************************************************************************/

ESTagsFilter::~ESTagsFilter() = default;

/********************************************************************************/

#ifdef IMAGETAGGER_ENABLE
void ESTagsFilter::loadTokenizerAndHNSW()
{
#ifdef Q_OS_ANDROID
	mTokenizerDirectoryPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/Tokenizer";
#else
	mTokenizerDirectoryPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/Tokenizer");
#endif // Q_OS_ANDROID

	if (QDir(mTokenizerDirectoryPath).exists())
	{
		mTokenizerEnabled = true;

		(void)QtConcurrent::run([this]()
			{
				mTextEncoder.reset(new ESTextEncoder(mTokenizerDirectoryPath + "/model.onnx", mTokenizerDirectoryPath + "/tokenizer.json", mTokenizerDirectoryPath + "/config.json"));

	#ifdef HNSWLIB_ENABLED
				const ESDatabase& lDB = ESDatabase::getInstance();
				if (lDB.getEmbeddingsDimension() > 0)
				{
					std::shared_lock lLock(lDB.getFilesMutex());
					mHnswSpace.reset(new hnswlib::InnerProductSpace(ESDatabase::getInstance().getEmbeddingsDimension()));
					mHnswIndex.reset(new hnswlib::HierarchicalNSW<float>(mHnswSpace.get(), lDB.getFiles().size()));

					if (!loadHnswIndex())
					{
						onImageTaggerManagerLoadingProgress(0, 0);
					}
				}
#ifndef EXIFSTATS_READONLY
				(void)connect(&ESImageTaggerManager::getInstance(), &ESImageTaggerManager::imageLoadingProgress, this, &ESTagsFilter::onImageTaggerManagerLoadingProgress, Qt::QueuedConnection);
#endif //EXIFSTATS_READONLY
	#endif // HNSWLIB_ENABLED

				onDatabaseTagsHaveChanged();
				(void)connect(&ESDatabase::getInstance(), &ESDatabase::tagsChanged, this,
					[this]()
					{
						(void)QtConcurrent::run([this]()
							{
								onDatabaseTagsHaveChanged();
							});
					}, Qt::DirectConnection);
			});
	}
}
#endif // IMAGETAGGER_ENABLE

/********************************************************************************/

/*virtual*/ void ESTagsFilter::reset() /*override*/
{
	mSearchString = "";
	mMinSimilarityScore = 0.3f;
	ESImageCache::getInstance().resetSearchSimilarityScores();
}

/********************************************************************************/

float ESTagsFilter::getMinSimilarityScore() const
{
	return mMinSimilarityScore;
}

/********************************************************************************/

void ESTagsFilter::setMinSimilarityScore(float pMinSimilarityScore)
{
	mMinSimilarityScore = pMinSimilarityScore;
#ifdef HNSWLIB_ENABLED
	updateHnswSearchResults();
#endif // HNSWLIB_ENABLED
}

/********************************************************************************/

#ifdef HNSWLIB_ENABLED
void ESTagsFilter::onImageTaggerManagerLoadingProgress(int pLoadedCount, int pLoadingCount)
{
	if (pLoadedCount != pLoadingCount)
	{
		if(mHnswIndexUpdateFuture.isRunning())
		{
			mHnswIndexUpdatingAbortRequested = true;
		}
		else
		{
			mHnswSpace.reset();
			mHnswIndex.reset();
		}

		QSettings lSettings;
		QString lIndexPath = lSettings.value("HnswIndex").toString();
		if (lIndexPath.isEmpty())
			QFile::remove(lIndexPath);
	}
	else
	{
		if(mHnswIndexUpdateFuture.isRunning())
		{
			mHnswIndexUpdatingAbortRequested = true;
			mHnswIndexUpdateFuture.waitForFinished();
		}
		mHnswIndexUpdateFuture = QtConcurrent::run([this]()
		{
			const ESDatabase& lDB = ESDatabase::getInstance();
			if (lDB.getEmbeddingsDimension() > 0)
			{
				std::shared_lock lLock(lDB.getFilesMutex());

				setUpdatingHNSWIndex(true);
				setUpdatingHNSWIndexProgress(0.f);

				const int lNbFiles = int(lDB.getFiles().size());

				mHnswIndexUpdating = true;
				ESPerfLog lPerfLog("HNSW Index Update");
				qInfo() << "Updating HNSW index with " << lDB.getFiles().size() << " files...";
				mHnswSpace.reset(new hnswlib::InnerProductSpace(ESDatabase::getInstance().getEmbeddingsDimension()));
				mHnswIndex.reset(new hnswlib::HierarchicalNSW<float>(mHnswSpace.get(), lDB.getFiles().size()));
				int lFileIdx = 0;
				for (const auto& [lFileInfoId, lFileInfo] : lDB.getFiles())
				{
					if (lFileInfo.mEmbeddings.size() > 0)
						mHnswIndex->addPoint(lFileInfo.mEmbeddings.data(), lFileInfoId);
					if (mHnswIndexUpdatingAbortRequested || lDB.isUnlockDatabaseRequested())
					{
						mHnswSpace.reset();
						mHnswIndex.reset();
						mHnswIndexUpdatingAbortRequested = false;
						qInfo() << "HNSW index update cancelled.";
						break;
					}
					++lFileIdx;

					float lProgress = float(lFileIdx) / lNbFiles;
					if (abs(lProgress - getUpdatingHNSWIndexProgress()) >= 0.001)
						setUpdatingHNSWIndexProgress(lProgress);
				}
				qInfo() << "HNSW index updated.";
				mHnswIndexUpdating = false;

				setUpdatingHNSWIndex(false);
				setUpdatingHNSWIndexProgress(1.f);
			}
		});
	}
}
#endif // HNSWLIB_ENABLED

/********************************************************************************/

/*virtual*/ bool ESTagsFilter::isFileFilteredOut(const ESFileInfo& pFile) const /*override*/
{
	if (mSearchString.isEmpty())
		return false;

#ifdef IMAGETAGGER_ENABLE
	if(mTextEncoder)
	{
		if (pFile.mEmbeddings.size() == mSearchTagsEmbeddings.mEmbeddings.size())
		{
#ifdef HNSWLIB_ENABLED
			if (!mHnswIndexUpdating && mHnswIndex)
			{
				return mHnswSearchResults.find(pFile.mId) == mHnswSearchResults.end();
			}
			else
#endif // HNSWLIB_ENABLED
			{
				float lSimilarityScore = mSearchTagsEmbeddings.computeSimilarityScore(pFile.mEmbeddings);
				ESImageCache::getInstance().getImage(pFile.mFilePath)->mCurrentSearchSimilarity = lSimilarityScore;
				return lSimilarityScore < mMinSimilarityScore;
			}
		}

		if(mSearchTagsEmbeddings.mEmbeddings.size() > 0)
			return true;

		for (const std::unordered_set<uint16_t>& lSearchTags : mSearchTagIndices)
		{
			bool lHasAtLeastOneSearchTag = false;
			for (uint16_t lTagIndex : pFile.mTagIndexes)
			{
				if (lSearchTags.contains(lTagIndex))
				{
					lHasAtLeastOneSearchTag = true;
					break;
				}
			}
			if (!lHasAtLeastOneSearchTag)
				return true;
		}

		return false;
	}
	else
#endif // IMAGETAGGER_ENABLE
	{
		QString lTags = ESDatabase::getInstance().getTagsLabels(pFile.mTagIndexes).join(" ");
		for (const QString& lPathPart : mSearchString.split(" ", Qt::SkipEmptyParts))
		{
			if (lTags.contains(lPathPart, Qt::CaseInsensitive))
				return false;
		}

		return true;
	}
}

/********************************************************************************/

/*virtual*/ QJsonObject ESTagsFilter::serialize() const /*override*/
{
	QJsonObject lResult;
	lResult["SearchString"] = mSearchString;
	return lResult;
}

/********************************************************************************/

/*virtual*/ bool ESTagsFilter::deserialize(const QJsonObject& pJson) /*override*/
{
	VALIDATE_JSONVALUE(pJson, "SearchString", mSearchString);

	return true;
}

/********************************************************************************/

QString ESTagsFilter::getSearchString() const
{
	return mSearchString;
}

/********************************************************************************/

void ESTagsFilter::setSearchString(const QString& pSearchString)
{
#ifdef IMAGETAGGER_ENABLE
	if(!mDatabaseTagsEmbeddingCacheMutex.tryLock())
		return;
#endif // IMAGETAGGER_ENABLE

	mSearchString = pSearchString;

#ifdef IMAGETAGGER_ENABLE
	if (mTextEncoder)
	{
		mSearchTagIndices.clear();
		mSearchTags.clear();
		ESImageCache::getInstance().resetSearchSimilarityScores();

		if(mDatabaseTagsEmbeddingCache.size() > 0)
		{
			for (const QString& lSearchTag : mSearchString.split(" ", Qt::SkipEmptyParts))
			{
				mSearchTagIndices.emplace_back();

				std::vector<std::pair<float, uint16_t>> lScores;
				ESTextEncoder::TextEncodedResult lSearchTagEmbedding = mTextEncoder->encode(lSearchTag);
				for (uint16_t i = 0, e = uint16_t(mDatabaseTagsEmbeddingCache.size()); i < e ; ++i)
				{
					const ESTextEncoder::TextEncodedResult& lDatabaseTagEmbedding = mDatabaseTagsEmbeddingCache[i];
					float lSimilarityScore = lSearchTagEmbedding.computeSimilarityScore(lDatabaseTagEmbedding);
					lScores.emplace_back(lSimilarityScore, i);
				}

				std::sort(lScores.begin(), lScores.end(),
					[](const std::pair<float, uint16_t>& a, const std::pair<float, uint16_t>& b)
					{
						return a.first > b.first;
					});

				for (const std::pair<float, uint16_t>& lScore : lScores)
				{
					if (lScore.first > 0.6)
					{
						mSearchTagIndices.back().insert(lScore.second);
						QString lTagLabel = ESDatabase::getInstance().getTagLabel(lScore.second);
						mSearchTags << lTagLabel;
						qDebug() << "Tag Score: " << lTagLabel << "=" << lScore.first;
						if (mSearchTagIndices.back().size() >= 3)
							break;
					}
				}
			}
		}

		mSearchTagsEmbeddings = mTextEncoder->encode(mSearchString);

#ifdef HNSWLIB_ENABLED
		updateHnswSearchResults();
#endif // HNSWLIB_ENABLED

		mDatabaseTagsEmbeddingCacheMutex.unlock();
	}
#endif // IMAGETAGGER_ENABLE
}

/********************************************************************************/

#ifdef HNSWLIB_ENABLED
void ESTagsFilter::updateHnswSearchResults()
{
	if (mHnswIndex && !mHnswIndexUpdating)
	{
		ESPerfLog lPerfLog("HNSW Search");
		constexpr size_t cMaxResults = 10000;
		mHnswSearchResults.clear();
		mHnswSearchResults.reserve(cMaxResults);
		hnswlib::EpsilonSearchStopCondition<float> lStopCondition(1.f - mMinSimilarityScore, 100, cMaxResults);
		std::vector<std::pair<float, uint64_t>> lSearchResults = mHnswIndex->searchStopConditionClosest(mSearchTagsEmbeddings.mEmbeddings.data(), lStopCondition);
		for (std::pair<float, ESStringPool::InternalId> lResult : lSearchResults)
		{
			const float lSimilarityScore = 1.f - lResult.first;
			if (lSimilarityScore >= mMinSimilarityScore) // HNSWLIB InnerProductSpace: d = 1.0 - sum(Ai*Bi)
			{
				ESFileInfoId lFileInfoId = ESFileInfoId(mHnswIndex->getExternalLabel(hnswlib::tableint(lResult.second)));
				mHnswSearchResults.insert(lFileInfoId);
				ESStringId lImagePath = ESDatabase::getInstance().getFileInfo(lFileInfoId)->mFilePath;
				ESImageCache::getInstance().getImage(lImagePath)->mCurrentSearchSimilarity = lSimilarityScore;
			}
		}
	}

}
#endif // HNSWLIB_ENABLED

/********************************************************************************/

QStringList ESTagsFilter::getTagsFound() const
{
#ifdef IMAGETAGGER_ENABLE
	return mTextEncoder ? mSearchTags : QStringList();
#else
	return QStringList();
#endif // IMAGETAGGER_ENABLE
}

/********************************************************************************/

#ifdef IMAGETAGGER_ENABLE
void ESTagsFilter::onDatabaseTagsHaveChanged()
{
	std::scoped_lock lLock(mDatabaseTagsEmbeddingCacheMutex);
	ESDatabase& lDB = ESDatabase::getInstance();

	mDatabaseTagsEmbeddingCache.clear();
	std::vector<QString> lAllTags;
	lDB.getAllTags(lAllTags);
	mDatabaseTagsEmbeddingCache.resize(lAllTags.size());
	constexpr const int cNumThreads = 4;
	QFuture<void> lThreadResult[cNumThreads];
	for (int lThreadIdx = 0; lThreadIdx < cNumThreads; ++lThreadIdx)
	{
		lThreadResult[lThreadIdx] = QtConcurrent::run([this, &lAllTags, lThreadIdx, cNumThreads]()
		{
			for (int i = lThreadIdx; i < lAllTags.size(); i += cNumThreads)
			{
				mDatabaseTagsEmbeddingCache[i] = mTextEncoder->encode(lAllTags[i]);
			}
		});
	}

	// lAllTags must not be destroyed until everything is done
	for (int lThreadIdx = 0; lThreadIdx < cNumThreads; ++lThreadIdx)
		lThreadResult[lThreadIdx].waitForFinished();
}
#endif // IMAGETAGGER_ENABLE

/********************************************************************************/

#ifdef IMAGETAGGER_ENABLE
bool ESTagsFilter::isTokenizerEnabled() const
{
	return mTokenizerEnabled;
}
#endif // IMAGETAGGER_ENABLE

/********************************************************************************/

#ifdef HNSWLIB_ENABLED

bool ESTagsFilter::loadHnswIndex()
{
	if (mHnswIndex)
	{
#if defined(Q_OS_ANDROID) && defined(EXIFSTATS_READONLY)
		QSettings lSettings;
		QString lDataBasePath = lSettings.value(ESDatabase::msReadOnlyDatabaseFolderSettingsKey).toString();

		if (lDataBasePath.isEmpty())
			return false;

		QuaZip lZip(lDataBasePath);
		if (!lZip.open(QuaZip::mdUnzip))
		{
			qWarning() << "Cannot open ExifStats archive file";
			return false;
		}
		if (!lZip.setCurrentFile("hnswIndex.esti"))
		{
			qWarning() << "File 'hnswIndex.esti' not found in ExifStats archive file";
			return false;
		}
		QuaZipFile lHNSWIndexFile(&lZip);
		if(!lHNSWIndexFile.open(QIODevice::ReadOnly))
		{
			qWarning() << "Cannot open ExifStats archive HNSW Index file";
			return false;
		}
		QString lIndexDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
		QString lIndexPath = lIndexDir + QDir::separator() + "hnswIndex.esti";
		QFile lLocalIndexFile(lIndexPath);
		if (!lLocalIndexFile.open(QIODevice::WriteOnly))
		{
			qWarning() << "Cannot create local 'hnswIndex.esti'";
			return false;
		}
		lLocalIndexFile.write(lHNSWIndexFile.readAll());
		lLocalIndexFile.close();
		lHNSWIndexFile.close();

		lSettings.setValue("HnswIndex", lIndexPath);
		mHnswIndex->loadIndex(lIndexPath.toStdString(), mHnswSpace.get());

		return true;
#else
		QSettings lSettings;
		QString lIndexPath = lSettings.value("HnswIndex").toString();
		if (!lIndexPath.isEmpty() && QFile::exists(lIndexPath))
		{
			mHnswIndex->loadIndex(lIndexPath.toStdString(), mHnswSpace.get());

			return true;
		}
#endif
	}

	return false;
}

/********************************************************************************/

void ESTagsFilter::saveHnswIndex()
{
#ifndef EXIFSTATS_READONLY
	if(mHnswIndex)
	{
		QString lIndexDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
		QString lIndexPath = lIndexDir + QDir::separator() + "hnswIndex.esti";
	
		mHnswIndex->saveIndex(lIndexPath.toStdString());

		QSettings lSettings;
		lSettings.setValue("HnswIndex", lIndexPath);
	}
#endif // EXIFSTATS_READONLY
}

#endif // HNSWLIB_ENABLED

