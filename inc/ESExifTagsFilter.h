#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// Qt
#include <QGeoShape>
#include <QGeoRectangle>

// ES
#include "ESExifFilter.h"
#include "ESFileInfo.h"
#include "ESImageTagsSearchEngine.h"

// Stl
#include <unordered_set>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESExifTagsFilter : public QObject, public ExifFilter
{
	Q_OBJECT;

public:
	/********************************* METHODS ***********************************/

	ESExifTagsFilter();

	virtual void reset() override;
	virtual bool isFileFilteredOut(const FileInfo& pFile) const override;
	virtual QJsonObject serialize() const override;
	virtual bool deserialize(const QJsonObject& pJson) override;

	QStringList getActualSearchedTags() const;
	QStringList getTagsInclusiveFilters() const;
	void setTagsInclusiveFilters(const QStringList& pTagsInclusiveFilters);

#ifdef IMAGETAGGER_ENABLE
	bool isTokenizerEnabled() const;
#endif // IMAGETAGGER_ENABLE

private:
	/******************************** ATTRIBUTES **********************************/

	QStringList mTagsInclusiveFilters;

#ifdef IMAGETAGGER_ENABLE
	QString mTokenizerDirectoryPath;
	bool mTokenizerEnabled;
	std::unique_ptr<ESImageTagsSearchEngine> mEngine;

	std::vector<ESImageTagsSearchEngine::TextEncodedResult> mDatabaseTagsEmbeddingCache;
	std::vector<std::unordered_set<uint16_t>> mSearchTagIndices;
	QStringList mSearchTags;
	QMutex mDatabaseTagsEmbeddingCacheMutex;
#endif // IMAGETAGGER_ENABLE

	/********************************* METHODS ***********************************/

#ifdef IMAGETAGGER_ENABLE
	void onDatabaseTagsHaveChanged();
#endif // IMAGETAGGER_ENABLE
};