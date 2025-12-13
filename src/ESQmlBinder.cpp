#include "ESQmlBinder.h"

// ES
#include "ESDatabase.h"

// Qt
#include <QApplication>
#include <QUrl>
#include <QMessageBox>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

static const char* cPresetExtension = "espreset";

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

ESQmlBinder::ESQmlBinder()
{
	(void)connect(&ESDatabase::getInstance(), &ESDatabase::foldersChanged, this, 
	[this]()
	{
		updateStats(true);
		updateFiltersFromData();
		emit processedFoldersChanged();
	});
	(void)connect(&ESDatabase::getInstance(), &ESDatabase::propertyProcessingChanged, this, &ESQmlBinder::processingChanged);
	(void)connect(&ESDatabase::getInstance(), &ESDatabase::propertyProcessingProgressChanged, this, &ESQmlBinder::processingProgressChanged);

	mStats.push_back(&m35mmStat);
	mStats.push_back(&mApertureStat);
	mStats.push_back(&mCameraModelStat);
	mStats.push_back(&mLensModelStat);
	mStats.push_back(&mDateTimeStat);
	mStats.push_back(&mGeoLocationStat);
	mStats.push_back(&mListFilesStat);

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

	mFilters.push_back(&m35mmFilter);
	mFilters.push_back(&mApertureFilter);
	mFilters.push_back(&mCameraModelFilter);
	mFilters.push_back(&mLensModelFilter);
	mFilters.push_back(&mDateTimeFilter);
	mFilters.push_back(&mGeoLocationFilter);
	mFilters.push_back(&mPathFilter);
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

void ESQmlBinder::clear()
{
	if (QMessageBox::question(nullptr, tr("Clear Database"), tr("Are you sure you want to clear the database ?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
	{
		ESDatabase::getInstance().clear();
		ExifStatCountFocalLengthIn35mm::msCameraModelsTo35mmFocalFactors.clear();

		for (ExifStat* lStat : mStats)
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

void ESQmlBinder::parseFolder(const QUrl& pFolderPath, bool pClearDB)
{
	ESDatabase::getInstance().addFolders(QStringList(pFolderPath.toLocalFile()), pClearDB, false);
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
	for (ExifStat* lStat : mStats)
		lStat->reset();
	for (ExifStat* lStat : mStats)
	{
		for (auto&& lProcessedFile : ESDatabase::getInstance().getFiles())
		{
			if(lProcessedFile.second.mReadResult != eSuccess)
				continue;
			
			bool lAddFile = true;
			bool lKeepCategory = false;
			if(!pIgnoreFilters)
			{
				for (const ExifFilter* lFilter : mFilters)
				{
					if (lFilter->isFileFilteredOut(lProcessedFile.second))
					{
						lAddFile = false;
						lKeepCategory = lFilter->mKeepCategory;
						if(!lKeepCategory)
							break;
					}
				}
			}
			if(lAddFile)
				lStat->addFile(lProcessedFile.second);
			else if (lKeepCategory)
				lStat->addFileCategory(lProcessedFile.second);
		}
	}
	for (auto lStat : mStats)
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

void ESQmlBinder::setGeoShapeFilter(QGeoShape aGeoShape)
{
	mGeoLocationFilter.mGeoShapeFilter = aGeoShape;
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
	uint8_t lCameraModelIndex = ESDatabase::getInstance().getAllCameraModels().indexOf(pCameraModel);
	if (lCameraModelIndex < ExifStatCountFocalLengthIn35mm::msCameraModelsTo35mmFocalFactors.size())
	{
		ExifStatCountFocalLengthIn35mm::msCameraModelsTo35mmFocalFactors[lCameraModelIndex] = pFactor;
		updateStats(false);
	}
	else
	{
		qWarning() << __FUNCTION__"() camera model not found.";
	}
}

/********************************************************************************/

float ESQmlBinder::getCameraModelTo35mmFocalLengthFactor(QString pCameraModel) const
{
	uint8_t lCameraModelIndex = ESDatabase::getInstance().getAllCameraModels().indexOf(pCameraModel);
	if (lCameraModelIndex < ExifStatCountFocalLengthIn35mm::msCameraModelsTo35mmFocalFactors.size())
	{
		return ExifStatCountFocalLengthIn35mm::msCameraModelsTo35mmFocalFactors[lCameraModelIndex];
	}
	else
	{
		qWarning() << __FUNCTION__"() camera model not found.";
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
	static uint64_t sMaxTo = QDateTime::currentDateTime().toSecsSinceEpoch();
	uint64_t lOldTimeEnd = mDateTimeFilter.mFilterTo;
	QDateTime lToDate = QDateTime::fromString(pTo, "yyyy/MM/dd");
	if (lToDate.isValid())
	{
		qint64 lSecs = lToDate.toSecsSinceEpoch();
		mDateTimeFilter.mFilterTo = lSecs > 0 ? lSecs : 0;
		if (mDateTimeFilter.mFilterTo > sMaxTo)
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
	return QDateTime::fromSecsSinceEpoch(mDateTimeMin).toString(ExifStatCountDateTime::msTimeFormat);
}

/********************************************************************************/

QString ESQmlBinder::getMaxTime() const
{
	return QDateTime::fromSecsSinceEpoch(mDateTimeMax).toString(ExifStatCountDateTime::msTimeFormat);
}

/********************************************************************************/

bool ESQmlBinder::isCtrlPressed() const
{
	return qApp->keyboardModifiers().testFlag(Qt::ControlModifier);
}

/********************************************************************************/

const ESExifStatListFiles* ESQmlBinder::getFilteredFilesList() const
{
	return &mListFilesStat;
}

/********************************************************************************/

void ESQmlBinder::resetFilters()
{
	for (ExifFilter* lFilter : mFilters)
		lFilter->reset();
	updateStats(true);
	updateFiltersFromData();

	emit propertyPathInclusiveFiltersChanged();
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

	static const QRegularExpression lsInvalidChars(R"([\\\/:\*\?"<>\|])");
	if (pPresetName.contains(lsInvalidChars) || pPresetName.endsWith(' ') || pPresetName.endsWith('.'))
	{
		qWarning("Preset name contains invalid characters: %s", qUtf8Printable(pPresetName));
		return false;
	}

	QJsonObject lPresetJson;
	for(const ExifFilter* lFilter: mFilters)
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
	for (ExifFilter* lFilter : mFilters)
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
	emit propertyTimelineStepChanged();
	emit timeFromChanged();
	emit timeToChanged();
	emit propertyPathInclusiveFiltersChanged();

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
	QStringList lPresetFiles = lDir.entryList(QStringList() << QString("*.%1").arg(cPresetExtension), QDir::Files);
	for (const QString& lPresetFile : lPresetFiles)
	{
		lResult.push_back(lPresetFile.left(lPresetFile.length() - constExprStringLength(cPresetExtension) - 1));
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
	return getPresetsFolderPath() + QDir::separator() + pPresetName + "." + cPresetExtension;
}
