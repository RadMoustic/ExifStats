#include "ESQmlBinder.h"

// ES
#include "ESDatabase.h"
#include "ESPerfLog.h"
#include "ESImageTaggerManager.h"
#include "ESMaterialPalette.h"
#include "ESCrashHandler.h"

// Qt
#include <QApplication>
#include <QUrl>
#include <QMessageBox>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>

// Quazip
#ifdef Q_OS_ANDROID
#include <quazip/JlCompress.h>
#endif

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

	mCameraModelFilter.mKeepCategory = true;
	mLensModelFilter.mKeepCategory = true;
	mDateTimeFilter.mFilterFrom = mDateTimeStat.mMinMaxComp.mValidMinValue;
	mDateTimeFilter.mFilterTo = mDateTimeStat.mMinMaxComp.mValidMaxValue;

	m35mmFilter.mName = "35mm";
	mApertureFilter.mName = "Aperture";
	mCameraModelFilter.mName = "CameraModel";
	mLensModelFilter.mName = "LensModel";
	mDateTimeFilter.mName = "DateTime";
	mGeoLocationFilter.mName = "GeoLocation";
	mPathFilter.mName = "Path";
	mTagsFilter.mName = "Tags";
	mOrientationFilter.mName = "Orientation";

	mFilters.push_back(&m35mmFilter);
	mFilters.push_back(&mApertureFilter);
	mFilters.push_back(&mCameraModelFilter);
	mFilters.push_back(&mLensModelFilter);
	mFilters.push_back(&mDateTimeFilter);
	mFilters.push_back(&mGeoLocationFilter);
	mFilters.push_back(&mPathFilter);
	mFilters.push_back(&mTagsFilter);
	mFilters.push_back(&mOrientationFilter);

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
	ESDatabase::getInstance().loadDatabase();
	(void)QtConcurrent::run([]()
		{
			ESImageCache::getInstance().initializeFromDatabase();
		});
#else // EXIFSTATS_READONLY
	(void)pDatabaseArchive;
#endif // EXIFSTATS_READONLY
}

/********************************************************************************/

#if defined(IMAGETAGGER_ENABLE) && defined(Q_OS_ANDROID)
bool ESQmlBinder::extractZip(const QUrl& pZipUrl, const QString& pOutputDir, std::function<void(float)> pProgressCallback)
{
	QFile lZipFile(pZipUrl.toString());
	if (!lZipFile.open(QIODevice::ReadOnly))
	{
		return false;
	}

	QuaZip lZip(&lZipFile);
	if (!lZip.open(QuaZip::mdUnzip))
	{
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
				return false;
			}
			continue;
		}

		QFileInfo lFileInfo(lAbsolutePath);
		if (!lBaseDir.mkpath(lFileInfo.path()))
		{
			return false;
		}

		QuaZipFile lOutFile(&lZip);
		if (!lOutFile.open(QIODevice::ReadOnly))
		{
			return false;
		}

		QFile lLocalFile(lAbsolutePath);
		if (!lLocalFile.open(QIODevice::WriteOnly))
		{
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

#endif // defined(IMAGETAGGER_ENABLE) && defined(Q_OS_ANDROID)

/********************************************************************************/

void ESQmlBinder::installSearchModels(const QUrl& pSearchModelFile)
{
#if defined(IMAGETAGGER_ENABLE) && defined(Q_OS_ANDROID)
	(void)QtConcurrent::run([this, pSearchModelFile]()
	{
		QString lDestPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/Tokenizer";
		QDir lDestDir(lDestPath);
		if (lDestDir.exists())
			lDestDir.removeRecursively();
		lDestDir.mkpath(".");

		setSearchModelsExtracting(true);

		if (extractZip(pSearchModelFile, lDestPath, [this](float pProgress)
			{
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
			qWarning() << "Failed to extract Search Model files from" << pSearchModelFile.toLocalFile();
		}

		setSearchModelsExtracting(false);

		/*
		
		QDir lDestDir(lDestPath);

		if (lDestDir.exists())
			lDestDir.removeRecursively();

		lDestDir.mkpath(".");

		QDir lSourceDir(pSearchModelFile.toString());

		// TODO: add a progress bar for that
		for (const QFileInfo& lFile : lSourceDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot))
			QFile::copy(lFile.absoluteFilePath(), lDestPath + "/" + lFile.fileName());

		mTagsFilter.loadTokenizerAndHNSW([this](bool pTokenizerEnabled, bool pHNSWEnabled)
			{
				setTokenizerEnabled(pTokenizerEnabled);
				setHNSWIndexEnabled(pHNSWEnabled);
			});
		*/
	});
#else
	(void)pSearchModelFile;
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

	mDateTimeFilter.mFilterFrom = mDateTimeStat.mMinMaxComp.getMinValue();
	mDateTimeMin = mDateTimeFilter.mFilterFrom;
	emit timeFromChanged();

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

void ESQmlBinder::setGeoShapeFilter(QGeoShape pGeoShape)
{
	mGeoLocationFilter.mGeoShapeFilter = pGeoShape;
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
	static uint64_t lsMaxTo = QDateTime::currentDateTime().toSecsSinceEpoch();
	uint64_t lOldTimeEnd = mDateTimeFilter.mFilterTo;
	QDateTime lToDate = QDateTime::fromString(pTo, "yyyy/MM/dd");
	if (lToDate.isValid())
	{
		qint64 lSecs = lToDate.toSecsSinceEpoch();
		mDateTimeFilter.mFilterTo = lSecs > 0 ? lSecs : 0;
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

void ESQmlBinder::resetFilters()
{
	for (ESFilter* lFilter : mFilters)
		lFilter->reset();
	updateStats(true);
	updateFiltersFromData();

	emit propertyPathInclusiveFiltersChanged();
	emit propertyTagsSearchStringChanged();
	emit propertyOrientationFilterModeChanged();
	emit propertyTagsMinSimilarityScoreChanged();
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
		QJsonObject lStatJson;
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
	emit propertyOrientationFilterModeChanged();

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
