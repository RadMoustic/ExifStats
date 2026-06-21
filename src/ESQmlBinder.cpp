#include "ESQmlBinder.h"

// ES
#include "ESDatabase.h"
#include "ESPerfLog.h"
#include "ESImageTaggerManager.h"
#include "ESMaterialPalette.h"
#include "ESCrashHandler.h"
#include "ESSplitZipFileDevice.h"

// Qt
#include <QApplication>
#include <QUrl>
#include <QMessageBox>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QtLogging>

// Quazip
#include <quazip/JlCompress.h>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

static const char* scPresetExtension = "espreset";
static const char* scDefaultPresetName = "__Default__";

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

ESQmlBinder::ESQmlBinder()
	: mFullScreen(false)
	, mTagging(false)
	, mTaggingProgress(1.f)
	, mPauseCaching(false)
	, mPauseTagging(false)
	, mImageTaggerEnabled(false)
	, mHNSWIndexEnabled(false)
	, mTokenizerEnabled(false)
	, mSearchModelsExtracting(false)
	, mSearchModelsExtractingProgress(0.f)
	, mTaggingModelsExtracting(false)
	, mTaggingModelsExtractingProgress(0.f)
	, mExporting(false)
	, mExportingProgress(0.f)
{
	(void)connect(&ESDatabase::getInstance(), &ESDatabase::dataChanged, this, 
	[this]()
	{
		updateStats(true);
		updateFiltersFromData();
		emit processedFoldersChanged();
	}, Qt::QueuedConnection);
	(void)connect(&ESDatabase::getInstance(), &ESDatabase::propertyProcessingChanged, this, &ESQmlBinder::processingChanged);
	(void)connect(&ESDatabase::getInstance(), &ESDatabase::propertyProcessingProgressChanged, this, &ESQmlBinder::processingProgressChanged);
#if defined(IMAGETAGGER_ENABLE) && !defined(EXIFSTATS_READONLY)
#ifdef HNSWLIB_ENABLED
	(void)connect(&mTagsFilter, &ESTagsFilter::propertyUpdatingHNSWIndexChanged, this, &ESQmlBinder::updatingHNSWIndexChanged);
	(void)connect(&mTagsFilter, &ESTagsFilter::propertyUpdatingHNSWIndexProgressChanged, this, &ESQmlBinder::updatingHNSWIndexProgressChanged);
#endif // HNSWLIB_ENABLED
	(void)connect(&ESImageTaggerManager::getInstance(), &ESImageTaggerManager::imageLoadingProgress, this, &ESQmlBinder::onTaggingProgress, Qt::QueuedConnection);
#endif // defined(IMAGETAGGER_ENABLE) && !defined(EXIFSTATS_READONLY)

	mStats.push_back(&m35mmStat);
	mStats.push_back(&mApertureStat);
	mStats.push_back(&mCameraModelStat);
	mStats.push_back(&mLensModelStat);
	mStats.push_back(&mDateTimeStat);
	mStats.push_back(&mGeoLocationStat);
	mStats.push_back(&mListFilesStat);
	mStats.push_back(&mOrientationStat);
	mStats.push_back(&mISOSpeedStat);
	mStats.push_back(&mShutterSpeedStat);
	mStats.push_back(&mResolutionStat);

	mCameraModelStat.mCountComp.mIgnoreEmptyCategories = false;
	mLensModelStat.mCountComp.mIgnoreEmptyCategories = false;
	mResolutionStat.mCountComp.mMinCountCategory = 2;

	mCameraModelFilter.mKeepCategory = true;
	mCameraModelFilter.mDeserializeGetAllValuesCallback = []() -> const QVector<QString>& { return ESDatabase::getInstance().getAllCameraModels(); };
	mLensModelFilter.mKeepCategory = true;
	mLensModelFilter.mDeserializeGetAllValuesCallback = []() -> const QVector<QString>& { return ESDatabase::getInstance().getAllLensModels(); };
	mDateTimeFilter.mFilterFrom = mDateTimeStat.mMinMaxComp.mValidMinValue;
	mDateTimeFilter.mFilterTo = mDateTimeStat.mMinMaxComp.mValidMaxValue;
	
	mISOSpeedFilter.setInvalidValue(0);
	mShutterSpeedFilter.setInvalidValue(0);
	m35mmFilter.setInvalidValue(0);
	mApertureFilter.setInvalidValue(0.f);
	//mDateTimeFilter.setInvalidValue(18446744073709548016);
	mDateTimeFilter.setInvalidValue(0);

	m35mmFilter.mName = "35mm";
	mApertureFilter.mName = "Aperture";
	mCameraModelFilter.mName = "CameraModel";
	mLensModelFilter.mName = "LensModel";
	mDateTimeFilter.mName = "DateTime";
	mGeoLocationFilter.mName = "GeoLocation";
	mPathFilter.mName = "Path";
	mTagsFilter.mName = "Tags";
	mOrientationFilter.mName = "Orientation";
	mISOSpeedFilter.mName = "ISOSpeed";
	mShutterSpeedFilter.mName = "ShutterSpeed";
	mWidthFilter.mName = "Width";
	mHeightFilter.mName = "Height";

	m35mmFilter.mShouldBeSerializedCallback = [&]() { return m35mmFilter.mFilterFrom != getMinFocalLength35mm() || m35mmFilter.mFilterTo != getMaxFocalLength35mm(); };
	mApertureFilter.mShouldBeSerializedCallback = [&]() { return mApertureFilter.mFilterFrom != getMinAperture() || mApertureFilter.mFilterTo != getMaxAperture(); };
	mISOSpeedFilter.mShouldBeSerializedCallback = [&]() { return mISOSpeedFilter.mFilterFrom != getMinISOSpeed() || mISOSpeedFilter.mFilterTo != getMaxISOSpeed(); };
	mShutterSpeedFilter.mShouldBeSerializedCallback = [&]() { return mShutterSpeedFilter.mFilterFrom != getMinShutterSpeed() || mShutterSpeedFilter.mFilterTo != getMaxShutterSpeed(); };
	mWidthFilter.mShouldBeSerializedCallback = [&]() { return mWidthFilter.mFilterFrom != getMinWidth() || mWidthFilter.mFilterTo != getMaxWidth(); };
	mHeightFilter.mShouldBeSerializedCallback = [&]() { return mHeightFilter.mFilterFrom != getMinHeight() || mHeightFilter.mFilterTo != getMaxHeight(); };

	mFilters.push_back(&m35mmFilter);
	mFilters.push_back(&mApertureFilter);
	mFilters.push_back(&mCameraModelFilter);
	mFilters.push_back(&mLensModelFilter);
	mFilters.push_back(&mDateTimeFilter);
	mFilters.push_back(&mGeoLocationFilter);
	mFilters.push_back(&mPathFilter);
	mFilters.push_back(&mTagsFilter);
	mFilters.push_back(&mOrientationFilter);
	mFilters.push_back(&mISOSpeedFilter);
	mFilters.push_back(&mShutterSpeedFilter);
	mFilters.push_back(&mWidthFilter);
	mFilters.push_back(&mHeightFilter);

#ifdef IMAGETAGGER_ENABLE
	setImageTaggerEnabled(true);
#endif // IMAGETAGGER_ENABLE
}

/********************************************************************************/

void ESQmlBinder::initialize()
{
#ifdef IMAGETAGGER_ENABLE
	mTagsFilter.loadTokenizerAndHNSW([this](bool pTokenizerEnabled, bool pHNSWEnabled)
		{
			setTokenizerEnabled(pTokenizerEnabled);
			setHNSWIndexEnabled(pHNSWEnabled);
		});
#endif // IMAGETAGGER_ENABLE
}

/********************************************************************************/

void ESQmlBinder::refresh(bool pFullRefresh)
{
	if (!pFullRefresh || QMessageBox::question(nullptr, tr("Refresh Database"), tr("Are you sure you want to reparse all files in the selected directories ?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
	{
		ESDatabase::getInstance().refresh(pFullRefresh);
	}
}

/********************************************************************************/

void ESQmlBinder::retag()
{
#if defined(IMAGETAGGER_ENABLE) && !defined(EXIFSTATS_READONLY)
	if (QMessageBox::question(nullptr, tr("Retag Database"), tr("Are you sure you want to retag all files in the database ?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
	{
		ESImageTaggerManager::getInstance().retag();
	}
#endif // defined(IMAGETAGGER_ENABLE) && !defined(EXIFSTATS_READONLY)
}

/********************************************************************************/

void ESQmlBinder::clear()
{
	if (QMessageBox::question(nullptr, tr("Clear Database"), tr("Are you sure you want to clear the database ?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
	{
#if defined(IMAGETAGGER_ENABLE) && !defined(EXIFSTATS_READONLY)
		ESImageTaggerManager::getInstance().stopAndCancelAllLoadings();
#endif // defined(IMAGETAGGER_ENABLE) && !defined(EXIFSTATS_READONLY)
		ESImageCache::getInstance().stopAndCancelAllLoadings();

		ESDatabase::getInstance().clear();
		ESFocalLengthIn35mmStat::msCameraModelsTo35mmFocalFactors.clear();

		for (ESStat* lStat : mStats)
			lStat->reset();

		emit dataHasChanged();
		emit processedFoldersChanged();
	}
}

/********************************************************************************/

QVector<QString> ESQmlBinder::getProcessedFolders()
{
	return ESDatabase::getInstance().getFolders();
}

/********************************************************************************/

bool ESQmlBinder::getProcessing()
{
	return ESDatabase::getInstance().getProcessing();
}

/********************************************************************************/

float ESQmlBinder::getProcessingProgress()
{
	return ESDatabase::getInstance().getProcessingProgress();
}

/********************************************************************************/

bool ESQmlBinder::getUpdatingHNSWIndex()
{
#if defined(IMAGETAGGER_ENABLE) && defined(HNSWLIB_ENABLED)
	return mTagsFilter.getUpdatingHNSWIndex();
#else
	return	false;
#endif // defined(IMAGETAGGER_ENABLE) && defined(HNSWLIB_ENABLED)
}

/********************************************************************************/

float ESQmlBinder::getUpdatingHNSWIndexProgress()
{
#if defined(IMAGETAGGER_ENABLE) && defined(HNSWLIB_ENABLED)
	return mTagsFilter.getUpdatingHNSWIndexProgress();
#else
	return 1.f;
#endif // defined(IMAGETAGGER_ENABLE) && defined(HNSWLIB_ENABLED)
}

/********************************************************************************/

void ESQmlBinder::onTaggingProgress(int pLoadedCount, int pLoadingCount)
{
	if (pLoadedCount == pLoadingCount)
	{
		setTaggingProgress(1.f);
		setTagging(false);
	}
	else
	{
		setTagging(true);
		float lLoadingProgress = static_cast<float>(pLoadedCount) / pLoadingCount;
		if (abs(lLoadingProgress - getTaggingProgress()) >= 0.001)
			setTaggingProgress(lLoadingProgress);
	}
}

/********************************************************************************/

void ESQmlBinder::parseFolder(const QUrl& pFolderPath, bool pClearDB)
{
	ESDatabase::getInstance().updateDatabase(QStringList(pFolderPath.toLocalFile()), pClearDB, false);
}

/********************************************************************************/

void ESQmlBinder::setDatabaseArchive(const QUrl& pDatabaseArchive)
{
#if defined(EXIFSTATS_READONLY) && defined(Q_OS_ANDROID)
	QSettings lSettings;
	QString lFolderPathStr = pDatabaseArchive.toString();
	lSettings.setValue(ESDatabase::msReadOnlyDatabaseFolderSettingsKey, lFolderPathStr);
	ESImage::closeExifStatsArchive();
	ESDatabase::getInstance().loadDatabase();
	mTagsFilter.reloadHNSW();
	(void)QtConcurrent::run([]()
		{
			ESImageCache::getInstance().initializeFromDatabase();
		});
#else // EXIFSTATS_READONLY
	(void)pDatabaseArchive;
#endif // EXIFSTATS_READONLY
}

/********************************************************************************/

bool ESQmlBinder::extractZip(const std::vector<QString>& pSplittedZipFiles, const QString& pOutputDir, std::function<void(float)> pProgressCallback)
{
	ESSplitZipFileDevice lZipDevice(pSplittedZipFiles);
	if (!lZipDevice.open(QIODevice::ReadOnly))
	{
		qWarning() << "Failed to open split zip files for extraction.";
		return false;
	}

	QuaZip lZip(&lZipDevice);
	if (!lZip.open(QuaZip::mdUnzip))
	{
		qWarning() << "Failed to open zip archive for extraction.";
		return false;
	}

	qint64 lTotalUncompressedSize = 0;
	for (bool lMore = lZip.goToFirstFile(); lMore; lMore = lZip.goToNextFile())
	{
		QuaZipFileInfo64 lInfo;
		if (lZip.getCurrentFileInfo(&lInfo))
		{
			lTotalUncompressedSize += lInfo.uncompressedSize;
		}
	}

	if (lTotalUncompressedSize == 0)
	{
		qWarning() << "No files to extract in the zip archive.";
		return false;
	}

	lZip.goToFirstFile();
	QDir lBaseDir(pOutputDir);
	qint64 lCurrentExtractedSize = 0;

	for (bool lMore = lZip.goToFirstFile(); lMore; lMore = lZip.goToNextFile())
	{
		QString lFilePath = lZip.getCurrentFileName();
		QString lAbsolutePath = lBaseDir.absoluteFilePath(lFilePath);

		if (lFilePath.endsWith('/'))
		{
			if (!lBaseDir.mkpath(lFilePath))
			{
				qWarning() << "Failed to create directory:" << lFilePath;
				return false;
			}
			continue;
		}

		QFileInfo lFileInfo(lAbsolutePath);
		if (!lBaseDir.mkpath(lFileInfo.path()))
		{
			qWarning() << "Failed to create directory:" << lFileInfo.path();
			return false;
		}

		QuaZipFile lOutFile(&lZip);
		if (!lOutFile.open(QIODevice::ReadOnly))
		{
			qWarning() << "Failed to open file in zip archive:" << lFilePath;
			return false;
		}

		QFile lLocalFile(lAbsolutePath);
		if (!lLocalFile.open(QIODevice::WriteOnly))
		{
			qWarning() << "Failed to open local file for writing:" << lAbsolutePath;
			return false;
		}

		char lBuffer[4096];
		qint64 lReadLen;
		while ((lReadLen = lOutFile.read(lBuffer, sizeof(lBuffer))) > 0)
		{
			lCurrentExtractedSize += lReadLen;
			float lProgress = static_cast<float>(lCurrentExtractedSize) / lTotalUncompressedSize;
			pProgressCallback(lProgress);
			lLocalFile.write(lBuffer, lReadLen);
		}
	}

	pProgressCallback(1.f);

	return true;
}

/********************************************************************************/

void ESQmlBinder::createDatabaseArchive(const QUrl& pZipPath)
{
	if(getProcessing())
	{
		QMessageBox::warning(nullptr, tr("Export Database"), tr("Cannot export database while processing."));
		return;
	}

	if(getTagging())
	{
		QMessageBox::warning(nullptr, tr("Export Database"), tr("Cannot export database while tagging."));
		return;
	}

	if(ESImageCache::getInstance().isLoading())
	{
		QMessageBox::warning(nullptr, tr("Export Database"), tr("Cannot export database while image cache is working."));
		return;
	}

#ifdef HNSWLIB_ENABLED
	if(getUpdatingHNSWIndex())
	{
		QMessageBox::warning(nullptr, tr("Export Database"), tr("Cannot export database while the search index is being updated."));
		return;
	}
#endif // HNSWLIB_ENABLED

	QtConcurrent::run([this, pZipPath]()
		{
			const ESDatabase& lDB = ESDatabase::getInstance();
			ESImageCache& lImageCache = ESImageCache::getInstance();

			lDB.saveDatabase();
#ifdef HNSWLIB_ENABLED
			mTagsFilter.saveHnswIndex();
#endif // HNSWLIB_ENABLED

			QuaZip lZip(pZipPath.toLocalFile());
			if (!lZip.open(QuaZip::mdCreate))
			{
				QMessageBox::warning(nullptr, tr("Export Database"), tr("Failed to create zip archive."));
				return;
			}

			setExporting(true);

			std::vector<std::pair<QString,QString>> lFiles;

			lFiles.emplace_back(lDB.getDatabaseFilePath(), "");
#ifdef HNSWLIB_ENABLED
			lFiles.emplace_back(mTagsFilter.getHNSWIndexFilePath(), "");
#endif // HNSWLIB_ENABLED
			for(const auto& [lFileInfoId, lFileInfo] : lDB.getFiles())
				if (std::shared_ptr<ESImage> lImage = lImageCache.getImage(lFileInfo.mFilePath))
					if(lImage->hasCacheFile())
						lFiles.emplace_back(lImage->getImageCachePath(), "ImageCache");

			setExportingProgress(0.f);
			int lFileIndex = 0;
			for (const auto& [lCurrentPath, lTargetDir] : lFiles)
			{
				QFile lSourceFile(lCurrentPath);
				if (!lSourceFile.open(QIODevice::ReadOnly))
				{
					continue;
				}

				QFileInfo lFileInfo(lCurrentPath);

				QString lInternalPath = lTargetDir;
				if (!lInternalPath.isEmpty() && !lInternalPath.endsWith('/'))
					lInternalPath += "/";
				lInternalPath += lFileInfo.fileName();

				QuaZipNewInfo lNewInfo(lInternalPath);

				QuaZipFile lTargetFile(&lZip);
				if (!lTargetFile.open(QIODevice::WriteOnly, lNewInfo, nullptr, 0, 0))
				{
					lSourceFile.close();
					break;
				}

				char lBuffer[8192];
				qint64 lBytesRead;
				while ((lBytesRead = lSourceFile.read(lBuffer, sizeof(lBuffer))) > 0)
				{
					lTargetFile.write(lBuffer, lBytesRead);
				}

				lTargetFile.close();
				lSourceFile.close();

				setExportingProgress(static_cast<float>(++lFileIndex) / lFiles.size());
			}

			lZip.close();

			setExporting(false);
		});
}

/********************************************************************************/

std::vector<QString> getEssmSequence(const QUrl& pFolderUrl)
{
	std::vector<QString> lResult;
	QString lFolderPath = pFolderUrl.isLocalFile() ? pFolderUrl.toLocalFile() : pFolderUrl.toString();
	QDir lDir(lFolderPath);
	QStringList lFilters;
	lFilters << "*.essm*";
	QFileInfoList lFiles = lDir.entryInfoList(lFilters, QDir::Files, QDir::Name);
	QFileInfo lFirstFile;
	bool lFound = false;
	for (const QFileInfo& lInfo : lFiles)
	{
		if (lInfo.fileName().endsWith(".essm"))
		{
			lFirstFile = lInfo;
			lFound = true;
			break;
		}
	}
	if (!lFound)
	{
		return lResult;
	}
	lResult.push_back(lFirstFile.absoluteFilePath());
	QString lBaseName = lFirstFile.fileName().left(lFirstFile.fileName().lastIndexOf(".essm"));
	int lIndex = 1;
	while (true)
	{
		QString lNextFileName = lBaseName + ".essm" + QString::number(lIndex);
		bool lNextFound = false;
		for (const QFileInfo& lInfo : lFiles)
		{
			if (lInfo.fileName() == lNextFileName)
			{
				lResult.push_back(lInfo.absoluteFilePath());
				lNextFound = true;
				break;
			}
		}
		if (lNextFound)
		{
			lIndex++;
		}
		else
		{
			break;
		}
	}
	return lResult;
}

/********************************************************************************/

void ESQmlBinder::installSearchModels(const QUrl& pSearchModelFile)
{
#if defined(IMAGETAGGER_ENABLE)
	(void)QtConcurrent::run([this, pSearchModelFile]()
	{
		QString lDestPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/Tokenizer";
		QDir lDestDir(lDestPath);
		if (lDestDir.exists())
			lDestDir.removeRecursively();
		lDestDir.mkpath(".");

		setTokenizerEnabled(false);
		setSearchModelsExtracting(true);

		std::vector<QString> lSearchModelFiles;
#ifdef Q_OS_ANDROID
		lSearchModelFiles = getEssmSequence(pSearchModelFile);
		qInfo() << "Search model files found:";
		qInfo() << lSearchModelFiles;
#else
		lSearchModelFiles.push_back(pSearchModelFile.toLocalFile());

		while(true)
		{
			QUrl lNextPart = pSearchModelFile;
			lNextPart.setPath(lNextPart.path() + QString("%1").arg(lSearchModelFiles.size()));
			if(QFile::exists(lNextPart.toLocalFile()))
				lSearchModelFiles.push_back(lNextPart.toLocalFile());
			else
				break;
		}
#endif // Q_OS_ANDROID

		if (extractZip(lSearchModelFiles, lDestPath, [this](float pProgress)
			{
				if (abs(pProgress - getSearchModelsExtractingProgress()) >= 0.001)
					setSearchModelsExtractingProgress(pProgress);
			}))
		{
			mTagsFilter.loadTokenizerAndHNSW([this](bool pTokenizerEnabled, bool pHNSWEnabled)
				{
					setTokenizerEnabled(pTokenizerEnabled);
					setHNSWIndexEnabled(pHNSWEnabled);
				});
		}
		else
		{
			qWarning() << "Failed to extract Search Model files from" << pSearchModelFile.toString();
		}

		setSearchModelsExtracting(false);
	});
#else
	(void)pSearchModelFile;
#endif // IMAGETAGGER_ENABLE
}

/********************************************************************************/

void ESQmlBinder::installTaggingModels(const QUrl& pTaggingModelFile)
{
#if defined(IMAGETAGGER_ENABLE) && !defined(EXIFSTATS_READONLY)
	(void)QtConcurrent::run([this, pTaggingModelFile]()
	{
		QString lDestPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/ImageTaggers";
		QDir lDestDir(lDestPath);
		if (lDestDir.exists())
			lDestDir.removeRecursively();
		lDestDir.mkpath(".");

		setImageTaggerEnabled(false);
		ESImageTaggerManager::getInstance().unloadTaggers();
		setTaggingModelsExtracting(true);

		std::vector<QString> lTaggingModelFiles;
		lTaggingModelFiles.push_back(pTaggingModelFile.toLocalFile());

		while(true)
		{
			QUrl lNextPart = pTaggingModelFile;
			lNextPart.setPath(lNextPart.path() + QString("%1").arg(lTaggingModelFiles.size()));
			if(QFile::exists(lNextPart.toLocalFile()))
				lTaggingModelFiles.push_back(lNextPart.toLocalFile());
			else
				break;
		}

		if (extractZip(lTaggingModelFiles, lDestPath, [this](float pProgress)
			{
				if (abs(pProgress - getTaggingModelsExtractingProgress()) >= 0.001)
					setTaggingModelsExtractingProgress(pProgress);
			}))
		{
			ESImageTaggerManager::getInstance().loadTaggers();
			setImageTaggerEnabled(ESImageTaggerManager::getInstance().isEnabled());
		}
		else
		{
			qWarning() << "Failed to extract Tagging Model files from" << pTaggingModelFile.toLocalFile();
		}

		setTaggingModelsExtracting(false);
	});
#else
	(void)pTaggingModelFile;
#endif // IMAGETAGGER_ENABLE
}

/********************************************************************************/

void ESQmlBinder::themeHasChanged()
{
	ESMaterialPalette::clearCache();
}

/********************************************************************************/

void ESQmlBinder::updateFiltersFromData()
{
	m35mmFilter.mFilterFrom = m35mmStat.mMinMaxComp.getMinValue();
	m35mmMin = m35mmFilter.mFilterFrom;
	emit propertyFocalLengthFromChanged();

	m35mmFilter.mFilterTo = m35mmStat.mMinMaxComp.getMaxValue();
	m35mmMax = m35mmFilter.mFilterTo;
	emit propertyFocalLengthToChanged();

	mApertureFilter.mFilterFrom = mApertureStat.mMinMaxComp.getMinValue();
	mApertureMin = mApertureFilter.mFilterFrom;
	emit propertyApertureFromChanged();

	mApertureFilter.mFilterTo = mApertureStat.mMinMaxComp.getMaxValue();
	mApertureMax = mApertureFilter.mFilterTo;
	emit propertyApertureToChanged();

	mISOSpeedFilter.mFilterFrom = mISOSpeedStat.mMinMaxComp.getMinValue();
	mISOSpeedMin = mISOSpeedFilter.mFilterFrom;
	emit propertyISOSpeedFromChanged();

	mISOSpeedFilter.mFilterTo = mISOSpeedStat.mMinMaxComp.getMaxValue();
	mISOSpeedMax = mISOSpeedFilter.mFilterTo;
	emit propertyISOSpeedToChanged();

	mShutterSpeedFilter.mFilterFrom = mShutterSpeedStat.mMinMaxComp.getMinValue();
	mShutterSpeedMin = mShutterSpeedFilter.mFilterFrom;
	emit propertyShutterSpeedFromChanged();

	mShutterSpeedFilter.mFilterTo = mShutterSpeedStat.mMinMaxComp.getMaxValue();
	mShutterSpeedMax = mShutterSpeedFilter.mFilterTo;
	emit propertyShutterSpeedToChanged();

	mWidthFilter.mFilterFrom = mResolutionStat.mMinMaxWidthComp.getMinValue();
	mWidthMin = mWidthFilter.mFilterFrom;
	emit propertyWidthFromChanged();

	mWidthFilter.mFilterTo = mResolutionStat.mMinMaxWidthComp.getMaxValue();
	mWidthMax = mWidthFilter.mFilterTo;
	emit propertyWidthToChanged();

	mHeightFilter.mFilterFrom = mResolutionStat.mMinMaxHeightComp.getMinValue();
	mHeightMin = mHeightFilter.mFilterFrom;
	emit propertyHeightFromChanged();

	mHeightFilter.mFilterTo = mResolutionStat.mMinMaxHeightComp.getMaxValue();
	mHeightMax = mHeightFilter.mFilterTo;
	emit propertyHeightToChanged();

	mDateTimeFilter.mFilterTo = mDateTimeStat.mMinMaxComp.getMaxValue();
	mDateTimeMax = mDateTimeFilter.mFilterTo;
	emit timeToChanged();
}

/********************************************************************************/

void ESQmlBinder::updateStats(bool pIgnoreFilters)
{
	ESPerfLog lPerfLog(__FUNCTION__);

	const ESDatabase& lDB = ESDatabase::getInstance();
	std::shared_lock lLock(lDB.getFilesMutex());

	std::vector<const ESFilter*> lActiveFilters;
	for(const ESFilter* lFilter: mFilters)
	{
		if(lFilter->isEnabled())
			lActiveFilters.push_back(lFilter);
	}

	for (ESStat* lStat : mStats)
		lStat->reset();
	for (ESStat* lStat : mStats)
	{
		for (const auto& [lFileInfoId, lFileInfo] : lDB.getFiles())
		{
			if(lFileInfo.mReadResult != eSuccess)
				continue;
			
			bool lAddFile = true;
			bool lKeepCategory = false;
			if(!pIgnoreFilters)
			{
				for (const ESFilter* lFilter : lActiveFilters)
				{
					if (lFilter->isFileFilteredOut(lFileInfo))
					{
						lAddFile = false;
						lKeepCategory = lFilter->mKeepCategory;
						if(!lKeepCategory)
							break;
					}
				}
			}
			if(lAddFile)
				lStat->addFile(lFileInfo);
			else if (lKeepCategory)
				lStat->addFileCategory(lFileInfo);
		}
	}
	for (ESStat * lStat : mStats)
		lStat->onAllFilesAdded();

	emit dataHasChanged();
}

/********************************************************************************/

QVector<int> ESQmlBinder::getFocalLengthIn35mmCounts() const
{
	return m35mmStat.mCountComp.getCounters();
}

/********************************************************************************/

QVector<QString> ESQmlBinder::getFocalLengthIn35mmLabels() const
{
	return m35mmStat.mCountComp.getLabels();
}

/********************************************************************************/

QVector<QString> ESQmlBinder::getLensModels() const
{
	return mLensModelStat.mCountComp.getLabels();
}

/********************************************************************************/

QVector<int> ESQmlBinder::getLensModelsCount() const
{
	return mLensModelStat.mCountComp.getCounters();
}

/********************************************************************************/

QVariantMap ESQmlBinder::getLensModelsFilter() const
{
	return toQVariantMap(mLensModelFilter.getFilters());
}

/********************************************************************************/

void ESQmlBinder::setLensModelsFilter(const QVariantMap& pSelectedLens)
{
	mLensModelFilter.setFilters(toQMap<QString, bool>(pSelectedLens), ESDatabase::getInstance().getAllLensModels());
	updateStats(false);
}

/********************************************************************************/

QVector<QString> ESQmlBinder::getCameraModels() const
{
	return mCameraModelStat.mCountComp.getLabels();
}

/********************************************************************************/

QVector<int> ESQmlBinder::getCameraModelsCount() const
{
	return mCameraModelStat.mCountComp.getCounters();
}

/********************************************************************************/

QVariantMap ESQmlBinder::getCameraModelsFilter() const
{
	return toQVariantMap(mCameraModelFilter.getFilters());
}

/********************************************************************************/

void ESQmlBinder::setCameraModelsFilter(const QVariantMap& pSelectedCameras)
{
	mCameraModelFilter.setFilters(toQMap<QString, bool>(pSelectedCameras), ESDatabase::getInstance().getAllCameraModels());
	mLensModelFilter.resetFilters();
	updateStats(false);
}

/********************************************************************************/

void ESQmlBinder::setCameraModelTo35mmFocalLengthFactor(QString pCameraModel, float pFactor)
{
	int lCameraModelIndex = ESDatabase::getInstance().getAllCameraModels().indexOf(pCameraModel);
	if (lCameraModelIndex >= 0)
	{
		ESFocalLengthIn35mmStat::msCameraModelsTo35mmFocalFactors[lCameraModelIndex] = pFactor;
		updateStats(false);
	}
	else
	{
		qWarning() << __FUNCTION__ << "() camera model not found.";
	}
}

/********************************************************************************/

float ESQmlBinder::getCameraModelTo35mmFocalLengthFactor(QString pCameraModel) const
{
	int lCameraModelIndex = ESDatabase::getInstance().getAllCameraModels().indexOf(pCameraModel);
	if (lCameraModelIndex >= 0)
	{
		return ESFocalLengthIn35mmStat::msCameraModelsTo35mmFocalFactors[lCameraModelIndex];
	}
	else
	{
		qWarning() << __FUNCTION__ << "() camera model not found.";
	}

	return 1.0f;
}

/********************************************************************************/

QVector<int> ESQmlBinder::getApertureCounts() const
{
	return mApertureStat.mCountComp.getCounters();
}

/********************************************************************************/

QVector<QString> ESQmlBinder::getApertureLabels() const
{
	return mApertureStat.mCountComp.getLabels();
}

/********************************************************************************/

float ESQmlBinder::getMinAperture() const
{
	return mApertureMin;
}

/********************************************************************************/

float ESQmlBinder::getMaxAperture() const
{
	return mApertureMax;
}

/********************************************************************************/

QVector<int> ESQmlBinder::getISOSpeedCounts() const
{
	return mISOSpeedStat.mCountComp.getCounters();
}

/********************************************************************************/

QVector<QString> ESQmlBinder::getISOSpeedLabels() const
{
	return mISOSpeedStat.mCountComp.getLabels();
}

/********************************************************************************/

int ESQmlBinder::getMinISOSpeed() const
{
	return mISOSpeedMin;
}

/********************************************************************************/

int ESQmlBinder::getMaxISOSpeed() const
{
	return mISOSpeedMax;
}

/********************************************************************************/

QVector<int> ESQmlBinder::getShutterSpeedCounts() const
{
	return mShutterSpeedStat.mCountComp.getCounters();
}

/********************************************************************************/

QVector<QString> ESQmlBinder::getShutterSpeedLabels() const
{
	return mShutterSpeedStat.mCountComp.getLabels();
}

/********************************************************************************/

QVector<float> ESQmlBinder::getShutterSpeedValues() const
{
	return mShutterSpeedStat.mCountComp.getValues();
}

/********************************************************************************/

float ESQmlBinder::getMinShutterSpeed() const
{
	return mShutterSpeedMin;
}

/********************************************************************************/

float ESQmlBinder::getMaxShutterSpeed() const
{
	return mShutterSpeedMax;
}

/********************************************************************************/

int ESQmlBinder::getMinWidth() const
{
	return mWidthMin;
}

/********************************************************************************/

int ESQmlBinder::getMaxWidth() const
{
	return mWidthMax;
}

/********************************************************************************/

int ESQmlBinder::getMinHeight() const
{
	return mHeightMin;
}

/********************************************************************************/

int ESQmlBinder::getMaxHeight() const
{
	return mHeightMax;
}

/********************************************************************************/

QVector<QPointF> ESQmlBinder::getAllGeoLocations() const
{
	return mGeoLocationStat.mGeoLocComp.mGeoLocations;
}

/********************************************************************************/

QVector<QString> ESQmlBinder::getFilesAtLocation(const QPointF& pGeoLoc, float pMaxDist) const
{
	QVector<QString> lResult;
	QGeoCoordinate lGeoCoord(pGeoLoc.x(), pGeoLoc.y());
	for (int i = 0, e = mGeoLocationStat.mGeoLocComp.mGeoLocations.count(); i < e; ++i)
	{
		QGeoCoordinate lOneGeoLoc(mGeoLocationStat.mGeoLocComp.mGeoLocations[i].x(), mGeoLocationStat.mGeoLocComp.mGeoLocations[i].y());
		if (lGeoCoord.distanceTo(lOneGeoLoc) < pMaxDist)
		{
			lResult.push_back(mGeoLocationStat.mGeoLocComp.mGeoLocationsFiles[i]->mFilePath);
		}
	}

	return lResult;
}

/********************************************************************************/

QString ESQmlBinder::getTimeFrom() const
{
	return QDateTime::fromSecsSinceEpoch(mDateTimeFilter.mFilterFrom).toString("yyyy/MM/dd");
}

/********************************************************************************/

QString ESQmlBinder::getTimeTo() const
{
	return QDateTime::fromSecsSinceEpoch(std::min<uint64_t>( std::numeric_limits<qint64>::max() >> 10, mDateTimeFilter.mFilterTo)).toString("yyyy/MM/dd");
}

/********************************************************************************/

void ESQmlBinder::setTimeFrom(QString pFrom)
{
	uint64_t lOldTimeBegin = mDateTimeFilter.mFilterFrom;
	QDateTime lFromDate = QDateTime::fromString(pFrom, "yyyy/MM/dd");
	if(lFromDate.isValid())
	{
		qint64 lSecs = lFromDate.toSecsSinceEpoch();
		mDateTimeFilter.mFilterFrom = lSecs > 0 ? lSecs : 0;
	}
	else
	{
		mDateTimeFilter.mFilterFrom = 0;
	}
	if(lOldTimeBegin != mDateTimeFilter.mFilterFrom)
	{
		emit timeFromChanged();
		updateStats(false);
	}
}

/********************************************************************************/

void ESQmlBinder::setTimeTo(QString pTo)
{
	constexpr uint64_t c24Hours = 24*60*60-1;
	static uint64_t lsMaxTo = QDateTime::currentDateTime().toSecsSinceEpoch();
	uint64_t lOldTimeEnd = mDateTimeFilter.mFilterTo;
	QDateTime lToDate = QDateTime::fromString(pTo, "yyyy/MM/dd");
	if (lToDate.isValid())
	{
		qint64 lSecs = lToDate.toSecsSinceEpoch();
		mDateTimeFilter.mFilterTo = lSecs > 0 ? lSecs + c24Hours : 0;
		if (mDateTimeFilter.mFilterTo > lsMaxTo)
			mDateTimeFilter.mFilterTo = std::numeric_limits<uint64_t>::max();
	}
	else
	{
		mDateTimeFilter.mFilterTo = std::numeric_limits<uint64_t>::max();
	}
	if(lOldTimeEnd != mDateTimeFilter.mFilterTo)
	{
		emit timeToChanged();
		updateStats(false);
	}
}

/********************************************************************************/

QVector<int> ESQmlBinder::getTimeCounts() const
{
	return mDateTimeStat.mCountComp.getCounters();
}

/********************************************************************************/

QVector<QString> ESQmlBinder::getTimeLabels() const
{
	return mDateTimeStat.mCountComp.getLabels();
}

/********************************************************************************/

int ESQmlBinder::getMinFocalLength35mm() const
{
	return m35mmMin;
}

/********************************************************************************/

int ESQmlBinder::getMaxFocalLength35mm() const
{
	return m35mmMax;
}

/********************************************************************************/

QString ESQmlBinder::getMinTime() const
{
	return QDateTime::fromSecsSinceEpoch(mDateTimeMin).toString(ESDateTimeStat::msTimeFormat);
}

/********************************************************************************/

QString ESQmlBinder::getMaxTime() const
{
	return QDateTime::fromSecsSinceEpoch(mDateTimeMax).toString(ESDateTimeStat::msTimeFormat);
}

/********************************************************************************/

bool ESQmlBinder::isCtrlPressed() const
{
	return qApp->keyboardModifiers().testFlag(Qt::ControlModifier);
}

/********************************************************************************/

bool ESQmlBinder::isMobile() const
{
#if defined(Q_OS_ANDROID) || defined(EXIFSTATS_READONLY)
	return true;
#else
	return false;
#endif
}

/********************************************************************************/

bool ESQmlBinder::hasPreviousCrash() const
{
	return ESCrashHandler::hasPreviousCrash();
}

/********************************************************************************/

QString ESQmlBinder::getPreviousCrashLogs() const
{
	return ESCrashHandler::getPreviousCrashLogs();
}

/********************************************************************************/

void ESQmlBinder::resetPreviousCrash() const
{
	ESCrashHandler::resetPreviousCrash();
}

/********************************************************************************/

const ESListFilesStat* ESQmlBinder::getFilteredFilesList() const
{
	return &mListFilesStat;
}

/********************************************************************************/

QVector<QString> ESQmlBinder::getOrientations() const
{
	return mOrientationStat.mCountComp.getLabels();
}

/********************************************************************************/

QVector<int> ESQmlBinder::getOrientationsCount() const
{
	return mOrientationStat.mCountComp.getCounters();
}

/********************************************************************************/

QVector<QString> ESQmlBinder::getResolutions() const
{
	return mResolutionStat.mCountComp.getLabels();
}

/********************************************************************************/

QVector<int> ESQmlBinder::getResolutionsCount() const
{
	return mResolutionStat.mCountComp.getCounters();
}

/********************************************************************************/

void ESQmlBinder::resetFilters()
{
	for (ESFilter* lFilter : mFilters)
		lFilter->reset();
	updateStats(true);
	updateFiltersFromData();

	emit propertyPathInclusiveFiltersChanged();
	emit propertyTagsSearchStringChanged();
	emit propertyTagsSearchSimilarImageChanged();
	emit propertyOrientationFilterModeChanged();
	emit propertyTagsMinSimilarityScoreChanged();
	emit propertyFocalLengthFilterOutInvalidChanged();
	emit propertyApertureFilterOutInvalidChanged();
	emit propertyShutterSpeedFilterOutInvalidChanged();
	emit propertyISOSpeedFilterOutInvalidChanged();
	emit propertyTimeFilterOutInvalidChanged();
	emit propertyGeoShapeFilterChanged();
}

/********************************************************************************/

bool ESQmlBinder::saveDefaultFilters()
{
	deleteFilters(scDefaultPresetName);
	return saveFilters(scDefaultPresetName);
}

/********************************************************************************/

bool ESQmlBinder::loadDefaultFilters()
{
	return loadFilters(scDefaultPresetName);
}

/********************************************************************************/

bool ESQmlBinder::saveFilters(QString pPresetName)
{
	if (pPresetName.isEmpty())
	{
		qWarning("Preset name empty: %s", qUtf8Printable(pPresetName));
		return false;
	}

	if(pPresetName.size() > 255)
	{
		qWarning("Preset name too long (max 255 characters): %s", qUtf8Printable(pPresetName));
		return false;
	}

	static const QRegularExpression scInvalidChars(R"([\\\/:\*\?"<>\|])");
	if (pPresetName.contains(scInvalidChars) || pPresetName.endsWith(' ') || pPresetName.endsWith('.'))
	{
		qWarning("Preset name contains invalid characters: %s", qUtf8Printable(pPresetName));
		return false;
	}

	QJsonObject lPresetJson;
	for(const ESFilter* lFilter: mFilters)
	{
		assert(!lFilter->mName.isEmpty());
		if(lFilter->isEnabled() && lFilter->mShouldBeSerializedCallback())
			lPresetJson[lFilter->mName] = lFilter->serialize();
	}

	QString lPresetFilePath = getPresetFilePathPath(pPresetName);
	QDir lDir;
	lDir.mkpath(getPresetsFolderPath());
	QFile lPresetFile(lPresetFilePath);

	if (!lPresetFile.open(QIODevice::WriteOnly))
	{
		qWarning("Couldn't open preset file: %s", qUtf8Printable(lPresetFilePath));
		return false;
	}

	lPresetFile.write(QJsonDocument(lPresetJson).toJson());

	return true;
}

/********************************************************************************/

bool ESQmlBinder::loadFilters(QString pPresetName)
{
	if (pPresetName.isEmpty())
	{
		qWarning("Preset name empty: %s", qUtf8Printable(pPresetName));
		return false;
	}
	QString lPresetFilePath = getPresetFilePathPath(pPresetName);
	QFile lPresetFile(lPresetFilePath);
	if (!lPresetFile.open(QIODevice::ReadOnly))
	{
		qWarning("Couldn't open preset file: %s", qUtf8Printable(lPresetFilePath));
		return false;
	}
	QByteArray lPresetData = lPresetFile.readAll();
	QJsonDocument lPresetDoc = QJsonDocument::fromJson(lPresetData);
	if (lPresetDoc.isNull() || !lPresetDoc.isObject())
	{
		qWarning("Couldn't parse preset file: %s", qUtf8Printable(lPresetFilePath));
		return false;
	}
	
	for (ESFilter* lFilter : mFilters)
		lFilter->reset();
	
	QJsonObject lPresetJson = lPresetDoc.object();
	for (ESFilter* lFilter : mFilters)
	{
		assert(!lFilter->mName.isEmpty());
		if (lPresetJson.contains(lFilter->mName))
		{
			QJsonObject lStatJson = lPresetJson[lFilter->mName].toObject();
			lFilter->deserialize(lStatJson);
		}
	}
	updateStats(false);

	emit propertyApertureFromChanged();
	emit propertyApertureToChanged();
	emit propertyFocalLengthFromChanged();
	emit propertyFocalLengthToChanged();
	emit timeFromChanged();
	emit timeToChanged();

	emit propertyPathInclusiveFiltersChanged();
	emit propertyTagsSearchStringChanged();
	emit propertyTagsSearchSimilarImageChanged();
	emit propertyOrientationFilterModeChanged();
	emit propertyTagsMinSimilarityScoreChanged();
	emit propertyFocalLengthFilterOutInvalidChanged();
	emit propertyApertureFilterOutInvalidChanged();
	emit propertyShutterSpeedFilterOutInvalidChanged();
	emit propertyISOSpeedFilterOutInvalidChanged();
	emit propertyTimeFilterOutInvalidChanged();
	emit propertyGeoShapeFilterChanged();

	return true;
}

/********************************************************************************/

bool ESQmlBinder::deleteFilters(QString pPresetName)
{
	if (pPresetName.isEmpty())
	{
		qWarning("Preset name empty: %s", qUtf8Printable(pPresetName));
		return false;
	}
	QString lPresetFilePath = getPresetFilePathPath(pPresetName);
	QFile lPresetFile(lPresetFilePath);
	if (!lPresetFile.exists())
	{
		qWarning("Couldn't find preset file to delete: %s", qUtf8Printable(lPresetFilePath));
		return false;
	}
	if (!lPresetFile.remove())
	{
		qWarning("Couldn't delete preset file: %s", qUtf8Printable(lPresetFilePath));
		return false;
	}
	return true;
}

/********************************************************************************/

QStringList ESQmlBinder::getFiltersPresets() const
{
	QStringList lResult;
	QDir lDir(getPresetsFolderPath());
	if (!lDir.exists())
		return lResult;
	QStringList lPresetFiles = lDir.entryList(QStringList() << QString("*.%1").arg(scPresetExtension), QDir::Files);
	for (const QString& lPresetFile : lPresetFiles)
	{
		QString lPresetName = lPresetFile.left(lPresetFile.size() - QString(scPresetExtension).size() - 1);
		if (lPresetName == scDefaultPresetName)
			continue;
		lResult.push_back(lPresetName);
	}
	return lResult;
}

/********************************************************************************/

QString ESQmlBinder::getPresetsFolderPath() const
{
	return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QDir::separator() + "Presets";
}

/********************************************************************************/

QString ESQmlBinder::getPresetFilePathPath(const QString& pPresetName) const
{
	return getPresetsFolderPath() + QDir::separator() + pPresetName + "." + scPresetExtension;
}

/********************************************************************************/

QStringList ESQmlBinder::getTagsFound() const
{
	return mTagsFilter.getTagsFound();
}

/********************************************************************************/

void ESQmlBinder::save()
{
#if defined(IMAGETAGGER_ENABLE) && defined(HNSWLIB_ENABLED)
	mTagsFilter.saveHnswIndex();
#endif // defined(IMAGETAGGER_ENABLE) && defined(HNSWLIB_ENABLED)
}
