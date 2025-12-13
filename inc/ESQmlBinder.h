#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// Qt
#include <QObject>
#include <QVector2D>
#include <QGeoShape>
#include <QDateTime>

// Stl
#include <functional>
#include <unordered_set>

// ES
#include "ESExifStatCountFocalLengthIn35mm.h"
#include "ESExifStatCountAperture.h"
#include "ESExifStatCountCameraModel.h"
#include "ESExifStatCountLensModel.h"
#include "ESExifStatCountDateTime.h"
#include "ESExifStatGeoLocation.h"
#include "ESExifStatListFiles.h"
#include "ESExifFilter.h"
#include "ESExifFromToFilter.h"
#include "ESExifListFilter.h"
#include "ESExifGeoLocationFilter.h"
#include "ESExifPathFilter.h"
#include "ESFileInfo.h"

// External
#include "exif.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESDatabase;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

#define B_QML_PROPERTY(pName, pVarName, pType) \
	Q_PROPERTY(pType pName READ get##pName WRITE set##pName NOTIFY property##pName##Changed) \
	Q_SIGNAL void property##pName##Changed(); \
	pType get##pName() const \
	{ \
		return pVarName; \
	} \
	void set##pName(pType p##pName) \
	{ \
		if(pVarName != p##pName) \
		{ \
			pVarName = p##pName; \
			emit property##pName##Changed(); \
			updateStats(false); \
		} \
	} \
	public:

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESQmlBinder : public QObject
{
	Q_OBJECT
public:
	/******************************** ATTRIBUTES **********************************/

	Q_PROPERTY(QVector<QString> ProcessedFolders READ getProcessedFolders NOTIFY processedFoldersChanged)
	Q_PROPERTY(bool Processing READ getProcessing NOTIFY processingChanged)
	Q_PROPERTY(float ProcessingProgress READ getProcessingProgress NOTIFY processingProgressChanged)

	Q_PROPERTY(QString TimeFrom READ getTimeFrom WRITE setTimeFrom NOTIFY timeFromChanged)
	Q_PROPERTY(QString TimeTo READ getTimeTo WRITE setTimeTo NOTIFY timeToChanged)

	Q_PROPERTY(int MinFocalLength35mm READ getMinFocalLength35mm)
	Q_PROPERTY(int MaxFocalLength35mm READ getMaxFocalLength35mm)

	Q_PROPERTY(float MinAperture READ getMinAperture)
	Q_PROPERTY(float MaxAperture READ getMaxAperture)

	Q_PROPERTY(QString MinTime READ getMinTime)
	Q_PROPERTY(QString MaxTime READ getMaxTime)

	B_QML_PROPERTY(TimelineStep, mDateTimeStat.mCountComp.mStep, double)
	B_QML_PROPERTY(FocalLengthFrom, m35mmFilter.mFilterFrom, int)
	B_QML_PROPERTY(FocalLengthTo, m35mmFilter.mFilterTo, int)
	B_QML_PROPERTY(ApertureFrom, mApertureFilter.mFilterFrom, float)
	B_QML_PROPERTY(ApertureTo, mApertureFilter.mFilterTo, float)
	B_QML_PROPERTY(PathInclusiveFilters, mPathFilter.mPathInclusiveFilters, QStringList)

	/********************************* METHODS ***********************************/

	ESQmlBinder();

	QVector<QString> getProcessedFolders();
	bool getProcessing();
	float getProcessingProgress();

	// General
	Q_INVOKABLE void refresh(bool pFullRefresh);
	Q_INVOKABLE void clear();
	Q_INVOKABLE bool isCtrlPressed() const;
	Q_INVOKABLE void parseFolder(const QUrl& pFolderPath, bool pClearDB);

	// Lens Model
	Q_INVOKABLE QVector<QString> getLensModels() const;
	Q_INVOKABLE QVector<int> getLensModelsCount() const;
	Q_INVOKABLE QVariantMap getLensModelsFilter() const;
	Q_INVOKABLE void setLensModelsFilter(const QVariantMap& pSelectedLens);

	// Camera Model
	Q_INVOKABLE QVector<QString> getCameraModels() const;
	Q_INVOKABLE QVector<int> getCameraModelsCount() const;
	Q_INVOKABLE QVariantMap getCameraModelsFilter() const;
	Q_INVOKABLE void setCameraModelsFilter(const QVariantMap& pSelectedCameras);

	// Geo Location
	Q_INVOKABLE QVector<QPointF> getAllGeoLocations() const;
	Q_INVOKABLE QVector<QString> getFilesAtLocation(const QPointF& pGeoLoc, float pMaxDist) const;
	Q_INVOKABLE void setGeoShapeFilter(QGeoShape aGeoShape);

	// Timeline
	Q_INVOKABLE QString getTimeFrom() const;
	Q_INVOKABLE QString getTimeTo() const;
	Q_INVOKABLE void setTimeFrom(QString pFrom);
	Q_INVOKABLE void setTimeTo(QString pTo);
	Q_INVOKABLE QVector<int> getTimeCounts() const;
	Q_INVOKABLE QVector<QString> getTimeLabels() const;
	Q_INVOKABLE QString getMinTime() const;
	Q_INVOKABLE QString getMaxTime() const;

	// 35mm Focal Length
	Q_INVOKABLE QVector<int> getFocalLengthIn35mmCounts() const;
	Q_INVOKABLE QVector<QString> getFocalLengthIn35mmLabels() const;
	Q_INVOKABLE int getMinFocalLength35mm() const;
	Q_INVOKABLE int getMaxFocalLength35mm() const;
	Q_INVOKABLE void setCameraModelTo35mmFocalLengthFactor(QString pCameraModel, float pFactor);
	Q_INVOKABLE float getCameraModelTo35mmFocalLengthFactor(QString pCameraModel) const;

	// Aperture
	Q_INVOKABLE QVector<int> getApertureCounts() const;
	Q_INVOKABLE QVector<QString> getApertureLabels() const;
	Q_INVOKABLE float getMinAperture() const;
	Q_INVOKABLE float getMaxAperture() const;

	// List Files
	Q_INVOKABLE const ESExifStatListFiles* getFilteredFilesList() const;

	// Filter Presets
	Q_INVOKABLE void resetFilters();
	Q_INVOKABLE bool saveFilters(QString pPresetName);
	Q_INVOKABLE bool loadFilters(QString pPresetName);
	Q_INVOKABLE bool deleteFilters(QString pPresetName);
	Q_INVOKABLE QStringList getFiltersPresets() const;

signals:
	/********************************** SIGNALS ***********************************/

	void dataHasChanged();

	void processedFoldersChanged();
	void processingChanged();
	void processingProgressChanged();

	void timeFromChanged();
	void timeToChanged();

private:
	/******************************** ATTRIBUTES **********************************/

	ExifStatCountFocalLengthIn35mm m35mmStat;
	ESExifStatCountAperture mApertureStat;
	ExifStatCountCameraModel mCameraModelStat;
	ExifStatCountLensModel mLensModelStat;
	ExifStatCountDateTime mDateTimeStat;
	ExifStatGeoLocation mGeoLocationStat;
	ESExifStatListFiles mListFilesStat;

	ExifFromToFilter<int, ExifStatCountFocalLengthIn35mm> m35mmFilter;
	ExifFromToFilter<float, ESExifStatCountAperture> mApertureFilter;
	ExifListFilter<ExifStatCountCameraModel, QString> mCameraModelFilter;
	ExifListFilter<ExifStatCountLensModel, QString> mLensModelFilter;
	ExifFromToFilter<uint64_t, ExifStatCountDateTime>  mDateTimeFilter;
	ExifGeoLocationFilter mGeoLocationFilter;
	ESExifPathFilter mPathFilter;

	std::vector<ExifStat*> mStats;
	std::vector<ExifFilter*> mFilters;

	float mApertureMin;
	float mApertureMax;

	int m35mmMin;
	int m35mmMax;

	uint64_t mDateTimeMin;
	uint64_t mDateTimeMax;

	/********************************* METHODS ***********************************/

	void updateStats(bool pIgnoreFilters);
	void updateFiltersFromData();
	QString getPresetsFolderPath() const;
	QString getPresetFilePathPath(const QString& pPresetName) const;

	template<typename K, typename V>
	static QVariantMap toQVariantMap(const QMap<typename K, typename V>& pMap)
	{
		QVariantMap result;
		for (auto it = pMap.begin(); it != pMap.end(); ++it)
		{
			result[it.key()] = it.value();
		}

		return result;
	}

	template<typename K, typename V>
	static QMap<K, V> toQMap(const QVariantMap& pMap)
	{
		QMap<K, V> result;
		for (auto it = pMap.begin(); it != pMap.end(); ++it)
		{
			result[it.key()] = qvariant_cast<V>(it.value());
		}

		return result;
	}
};

