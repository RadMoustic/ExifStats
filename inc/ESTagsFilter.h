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

class ESTagsFilter : public QObject, public ESFilter
{
	Q_OBJECT;

public:
	/******************************** ATTRIBUTES **********************************/

	float mMinSimilarityScore;

	/********************************* METHODS ***********************************/

	ESTagsFilter();

	virtual void reset() override;
	virtual bool isFileFilteredOut(const ESFileInfo& pFile) const override;
	virtual QJsonObject serialize() const override;
	virtual bool deserialize(const QJsonObject& pJson) override;

	QStringList getTagsFound() const;
	QString getSearchString() const;
	void setSearchString(const QString& pSearchString);

#ifdef IMAGETAGGER_ENABLE
	bool isTokenizerEnabled() const;
#endif // IMAGETAGGER_ENABLE

private:
	/******************************** ATTRIBUTES **********************************/

	QString mSearchString;

#ifdef IMAGETAGGER_ENABLE
	QString mTokenizerDirectoryPath;
	bool mTokenizerEnabled;
	std::unique_ptr<ESImageTagsSearchEngine> mEngine;

	std::vector<ESImageTagsSearchEngine::TextEncodedResult> mDatabaseTagsEmbeddingCache;
	std::vector<std::unordered_set<uint16_t>> mSearchTagIndices;
	QStringList mSearchTags;
	ESImageTagsSearchEngine::TextEncodedResult mSearchTagsEmbeddings;
	QMutex mDatabaseTagsEmbeddingCacheMutex;
#endif // IMAGETAGGER_ENABLE

	/********************************* METHODS ***********************************/

#ifdef IMAGETAGGER_ENABLE
	void onDatabaseTagsHaveChanged();
#endif // IMAGETAGGER_ENABLE
};