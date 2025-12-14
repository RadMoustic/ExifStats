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
#include "ESExifStatCountCameraModel.h"
#include "ESExifStatCountLensModel.h"
#include "ESExifStatCountDateTime.h"
#include "ESExifStatGeoLocation.h"
#include "ESExifStat.h"
#include "ESExifFilter.h"
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

	const FileInfo* getFileInfo(StringId pFile) const;
	const std::map<StringId, FileInfo>& getFiles() const;
	const QVector<QString>& getAllLensModels() const;
	const QVector<QString>& getAllCameraModels() const;

signals:
	/********************************** SIGNALS ***********************************/

	void foldersChanged();

private:
	/******************************** ATTRIBUTES **********************************/

	QVector<QString> mFolders;

	std::map<StringId, FileInfo> mFiles;
	std::atomic_int mProcessedFilesCounter;
	QMutex mProgressMutex;

	QVector<QString> mAllLensModels;
	QVector<QString> mAllCameraModels;

	/********************************* METHODS ***********************************/

	ESDatabase();

	ReadExifFileResult readFileExif(const QString& pFilePath, easyexif::EXIFInfo& pOutExif);
	UsefullExif convertToUsefullExif(const easyexif::EXIFInfo& aFullExif);

	template<class SERIALIZER>
	bool Serialize(SERIALIZER& pSerializer, const QString& pFilePath);
};

