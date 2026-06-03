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
#include "ESFocalLengthIn35mmStat.h"
#include "ESApertureStat.h"
#include "ESCameraModelStat.h"
#include "ESLensModelStat.h"
#include "ESDateTimeStat.h"
#include "ESGeoLocationStat.h"
#include "ESListFilesStat.h"
#include "ESOrientationStat.h"
#include "ESFilter.h"
#include "ESFromToFilter.h"
#include "ESListFilter.h"
#include "ESGeoLocationFilter.h"
#include "ESPathFilter.h"
#include "ESTagsFilter.h"
#include "ESOrientationFilter.h"
#include "ESFileInfo.h"
#include "ESUtils.h"
#include "ESImageCache.h"
#include "ESImageTaggerManager.h"

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
		auto lValue = static_cast<decltype(pVarName)>(p##pName); \
		 \
		if(pVarName != lValue) \
		{ \
			pVarName = lValue; \
			emit property##pName##Changed(); \
			updateStats(false); \
		} \
	} \
	public:

#define B_QML_PROPERTY_GETSET(pName, pType, pGetterFct, pSetterFct) \
	Q_PROPERTY(pType pName READ get##pName WRITE set##pName NOTIFY property##pName##Changed) \
	Q_SIGNAL void property##pName##Changed(); \
	pType get##pName() const \
	{ \
		return pGetterFct(); \
	} \
	void set##pName(pType p##pName) \
	{ \
		 \
		if(pGetterFct() != p##pName) \
		{ \
			pSetterFct(p##pName); \
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

	ES_QML_PROPERTY(FullScreen, bool)

	Q_PROPERTY(QVector<QString> ProcessedFolders READ getProcessedFolders NOTIFY processedFoldersChanged)
	Q_PROPERTY(bool Processing READ getProcessing NOTIFY processingChanged)
	Q_PROPERTY(float ProcessingProgress READ getProcessingProgress NOTIFY processingProgressChanged)

	Q_PROPERTY(bool UpdatingHNSWIndex READ getUpdatingHNSWIndex NOTIFY updatingHNSWIndexChanged)
	Q_PROPERTY(float UpdatingHNSWIndexProgress READ getUpdatingHNSWIndexProgress NOTIFY updatingHNSWIndexProgressChanged)

	Q_PROPERTY(QString TimeFrom READ getTimeFrom WRITE setTimeFrom NOTIFY timeFromChanged)
	Q_PROPERTY(QString TimeTo READ getTimeTo WRITE setTimeTo NOTIFY timeToChanged)
	B_QML_PROPERTY(TimeFilterOutInvalid, mDateTimeFilter.mFilterOutInvalidValues, bool)

	Q_PROPERTY(int MinFocalLength35mm READ getMinFocalLength35mm)
	Q_PROPERTY(int MaxFocalLength35mm READ getMaxFocalLength35mm)

	Q_PROPERTY(float MinAperture READ getMinAperture)
	Q_PROPERTY(float MaxAperture READ getMaxAperture)

	Q_PROPERTY(QString MinTime READ getMinTime)
	Q_PROPERTY(QString MaxTime READ getMaxTime)

	ES_QML_PROPERTY(Tagging, bool)
	ES_QML_PROPERTY(TaggingProgress, float)

	ES_QML_PROPERTY(PauseCaching, bool, ESImageCache::getInstance().setPaused(mPauseCaching))
#if defined(IMAGETAGGER_ENABLE) && !defined(EXIFSTATS_READONLY)
	ES_QML_PROPERTY(PauseTagging, bool, ESImageTaggerManager::getInstance().setPaused(mPauseTagging))
#else
	ES_QML_PROPERTY(PauseTagging, bool)
#endif // defined(IMAGETAGGER_ENABLE) && !defined(EXIFSTATS_READONLY)

	B_QML_PROPERTY(TimelineStep, mDateTimeStat.mCountComp.mStep, double)
	B_QML_PROPERTY(TimelineStatic, mDateTimeStat.mCountComp.mAddFileCategories, bool)
	B_QML_PROPERTY(FocalLengthFrom, m35mmFilter.mFilterFrom, int)
	B_QML_PROPERTY(FocalLengthTo, m35mmFilter.mFilterTo, int)
	B_QML_PROPERTY(FocalLengthFilterOutInvalid, m35mmFilter.mFilterOutInvalidValues, bool)
	B_QML_PROPERTY(ApertureFrom, mApertureFilter.mFilterFrom, float)
	B_QML_PROPERTY(ApertureTo, mApertureFilter.mFilterTo, float)
	B_QML_PROPERTY(ApertureFilterOutInvalid, mApertureFilter.mFilterOutInvalidValues, bool)
	B_QML_PROPERTY(PathInclusiveFilters, mPathFilter.mPathInclusiveFilters, QStringList)
	B_QML_PROPERTY_GETSET(TagsSearchString, QString, mTagsFilter.getSearchString, mTagsFilter.setSearchString)
	B_QML_PROPERTY_GETSET(TagsMinSimilarityScore, float, mTagsFilter.getMinSimilarityScore, mTagsFilter.setMinSimilarityScore)
	B_QML_PROPERTY(OrientationFilterMode, mOrientationFilter.mFilterMode, int)
	B_QML_PROPERTY(GeoShapeFilter, mGeoLocationFilter.mGeoShapeFilter, QGeoShape)

	// Features
	ES_QML_PROPERTY(ImageTaggerEnabled, bool)
	ES_QML_PROPERTY(HNSWIndexEnabled, bool)
	ES_QML_PROPERTY(TokenizerEnabled, bool)

	// Models Installation
	ES_QML_PROPERTY(SearchModelsExtracting, bool)
	ES_QML_PROPERTY(SearchModelsExtractingProgress, float)
	ES_QML_PROPERTY(TaggingModelsExtracting, bool)
	ES_QML_PROPERTY(TaggingModelsExtractingProgress, float)

	// Export
	ES_QML_PROPERTY(Exporting, bool)
	ES_QML_PROPERTY(ExportingProgress, float)

	/********************************* METHODS ***********************************/

	ESQmlBinder();

	void initialize();

	QVector<QString> getProcessedFolders();
	bool getProcessing();
	float getProcessingProgress();

	// HNSW Index
	bool getUpdatingHNSWIndex();
	float getUpdatingHNSWIndexProgress();

	// General
	Q_INVOKABLE void refresh(bool pFullRefresh);
	Q_INVOKABLE void retag();
	Q_INVOKABLE void clear();
	Q_INVOKABLE bool isCtrlPressed() const;
	Q_INVOKABLE bool isMobile() const;
	Q_INVOKABLE bool hasPreviousCrash() const;
	Q_INVOKABLE QString getPreviousCrashLogs() const;
	Q_INVOKABLE void resetPreviousCrash() const;
	Q_INVOKABLE void parseFolder(const QUrl& pFolderPath, bool pClearDB);
	Q_INVOKABLE void setDatabaseArchive(const QUrl& pDatabaseArchive);
	Q_INVOKABLE void installSearchModels(const QUrl& pSearchModelFile);
	Q_INVOKABLE void installTaggingModels(const QUrl& pTaggingModelFile);
	Q_INVOKABLE void createDatabaseArchive(const QUrl& pZipPath);
	Q_INVOKABLE void themeHasChanged();

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
	Q_INVOKABLE const ESListFilesStat* getFilteredFilesList() const;

	// Orientation
	Q_INVOKABLE QVector<QString> getOrientations() const;
	Q_INVOKABLE QVector<int> getOrientationsCount() const;

	// Filter Presets
	Q_INVOKABLE void resetFilters();
	Q_INVOKABLE bool saveDefaultFilters();
	Q_INVOKABLE bool loadDefaultFilters();
	Q_INVOKABLE bool saveFilters(QString pPresetName);
	Q_INVOKABLE bool loadFilters(QString pPresetName);
	Q_INVOKABLE bool deleteFilters(QString pPresetName);
	Q_INVOKABLE QStringList getFiltersPresets() const;

	// Search tags
	Q_INVOKABLE QStringList getTagsFound() const;

	void save();

signals:
	/********************************** SIGNALS ***********************************/

	void dataHasChanged();

	void processedFoldersChanged();
	void processingChanged();
	void processingProgressChanged();
	void updatingHNSWIndexChanged();
	void updatingHNSWIndexProgressChanged();

	void timeFromChanged();
	void timeToChanged();

private:
	/******************************** ATTRIBUTES **********************************/

	ESFocalLengthIn35mmStat m35mmStat;
	ESApertureStat mApertureStat;
	ESCameraModelStat mCameraModelStat;
	ESLensModelStat mLensModelStat;
	ESDateTimeStat mDateTimeStat;
	ESGeoLocationStat mGeoLocationStat;
	ESListFilesStat mListFilesStat;
	ESOrientationStat mOrientationStat;

	ESFromToFilter<int, ESFocalLengthIn35mmStat> m35mmFilter;
	ESFromToFilter<float, ESApertureStat> mApertureFilter;
	ESListFilter<ESCameraModelStat, QString> mCameraModelFilter;
	ESListFilter<ESLensModelStat, QString> mLensModelFilter;
	ESFromToFilter<uint64_t, ESDateTimeStat>  mDateTimeFilter;
	ESGeoLocationFilter mGeoLocationFilter;
	ESPathFilter mPathFilter;
	ESTagsFilter mTagsFilter;
	ESOrientationFilter mOrientationFilter;

	std::vector<ESStat*> mStats;
	std::vector<ESFilter*> mFilters;

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
	void onTaggingProgress(int pLoadedCount, int pLoadingCount);
	bool extractZip(const std::vector<QString>& pSplittedZipFiles, const QString& pOutputDir, std::function<void(float)> pProgressCallback);

	template<typename K, typename V>
	static QVariantMap toQVariantMap(const QMap<K, V>& pMap)
	{
		QVariantMap lResult;
		for (auto it = pMap.begin(); it != pMap.end(); ++it)
		{
			lResult[it.key()] = it.value();
		}

		return lResult;
	}

	template<typename K, typename V>
	static QMap<K, V> toQMap(const QVariantMap& pMap)
	{
		QMap<K, V> lResult;
		for (auto it = pMap.begin(); it != pMap.end(); ++it)
		{
			lResult[it.key()] = qvariant_cast<V>(it.value());
		}

		return lResult;
	}
};

