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
#include <shared_mutex>

// ES
#include "ESFocalLengthIn35mmStat.h"
#include "ESCameraModelStat.h"
#include "ESLensModelStat.h"
#include "ESDateTimeStat.h"
#include "ESGeoLocationStat.h"
#include "ESStat.h"
#include "ESFilter.h"
#include "ESFileInfo.h"
#include "ESUtils.h"

// External
#include "exif.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESDatabase : public QObject
{
	Q_OBJECT

#ifdef IMAGETAGGER_ENABLE
	friend class ESImageTaggerManager;
#endif // IMAGETAGGER_ENABLE

public:	
	/******************************** ATTRIBUTES **********************************/

	ES_QML_PROPERTY(Processing, bool)
	ES_QML_PROPERTY(ProcessingProgress, float)

	/********************************* METHODS ***********************************/

	static ESDatabase& getInstance();

	void refresh(bool pFullRefresh);
	void clear();

	void addFolder(const QUrl& pFolderPath, bool pClearDB);
	void addFolders(const QStringList& pFolders, bool pClearDB, bool pNewFilesOnly);

	void saveDatabase();
	void loadDatabase();

	const QVector<QString>& getFolders() const;

	ESFileInfo* getFileInfo(ESStringId pFile);
	const ESFileInfo* getFileInfo(ESStringId pFile) const;
	ESFileInfo* getFileInfo(ESFileInfoId pFile);
	const ESFileInfo* getFileInfo(ESFileInfoId pFile) const;
	std::shared_mutex& getFilesMutex() const;
	bool isUnlockDatabaseRequested() const;
	const std::map<ESFileInfoId, ESFileInfo>& getFiles() const;
	const QVector<QString>& getAllLensModels() const;
	const QVector<QString>& getAllCameraModels() const;

	void setAllTags(const std::vector<QString>& pAllTags);
	void getAllTags(std::vector<QString>& pOutput);
	QString getTagLabel(uint16_t pTagIndex) const;
	QStringList getTagsLabels(const std::vector<uint16_t>& pTags);

	int getEmbeddingsDimension() const;
	void setEmbeddingsDimension(int pEmbeddingsDimension);

signals:
	/********************************** SIGNALS ***********************************/

	void dataChanged();
	void tagsChanged();

private:
	/******************************** ATTRIBUTES **********************************/

	QVector<QString> mFolders;

	std::map<ESFileInfoId, ESFileInfo> mFiles;
	std::map<ESStringId, ESFileInfoId> mFilesPathToId;
	std::atomic_int mProcessedFilesCounter;
	QMutex mProgressMutex;
	mutable std::shared_mutex mFilesMutex;

	QVector<QString> mAllLensModels;
	QVector<QString> mAllCameraModels;
	std::vector<QString> mAllTags;
	int mEmbeddingsDimension;
	std::atomic_bool mUnlockDatabaseRequested;

	ESFileInfoId mLastAssignedId;

	/********************************* METHODS ***********************************/

	ESDatabase();

	ESReadExifFileResult readFileExif(const QString& pFilePath, easyexif::EXIFInfo& pOutExif);
	ESUsefullExif convertToUsefullExif(const easyexif::EXIFInfo& aFullExif);

	template<class SERIALIZER>
	bool Serialize(SERIALIZER& pSerializer, const QString& pFilePath);
};

