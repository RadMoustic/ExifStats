#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// Qt
#include <QGeoShape>
#include <QGeoRectangle>
#include <QtConcurrent>

// ES
#include "ESFilter.h"
#include "ESFileInfo.h"
#include "ESTextEncoder.h"

// Stl
#include <unordered_set>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

namespace hnswlib
{
	template<typename dist_t>
	class HierarchicalNSW;

	class InnerProductSpace;
}

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESTagsFilter : public QObject, public ESFilter
{
	Q_OBJECT;

public:
	/******************************** ATTRIBUTES **********************************/

#if defined(IMAGETAGGER_ENABLE) && defined(HNSWLIB_ENABLED)
	ES_QML_PROPERTY(UpdatingHNSWIndex, bool)
	ES_QML_PROPERTY(UpdatingHNSWIndexProgress, float)
#endif //defined(IMAGETAGGER_ENABLE) && defined(HNSWLIB_ENABLED)

	static const char* msTokenizerFolderSettingsKey;

	/********************************* METHODS ***********************************/

	ESTagsFilter();
	virtual ~ESTagsFilter();

#ifdef IMAGETAGGER_ENABLE
	void loadTokenizerAndHNSW(std::function<void(bool, bool)> pDoneCallback);
#endif // IMAGETAGGER_ENABLE
	virtual bool isEnabled() const override;
	virtual void reset() override;
	virtual bool isFileFilteredOut(const ESFileInfo& pFile) const override;
	virtual QJsonObject serialize() const override;
	virtual bool deserialize(const QJsonObject& pJson) override;

	QStringList getTagsFound() const;
	QString getSearchString() const;
	void setSearchString(const QString& pSearchString);

	float getMinSimilarityScore() const;
	void setMinSimilarityScore(float pMinSimilarityScore);

#ifdef IMAGETAGGER_ENABLE
	bool isTokenizerEnabled() const;
#ifdef HNSWLIB_ENABLED
	void saveHnswIndex();
#endif // HNSWLIB_ENABLED
#endif // IMAGETAGGER_ENABLE

private:
	/******************************** ATTRIBUTES **********************************/

	QString mSearchString;
	float mMinSimilarityScore;

#ifdef IMAGETAGGER_ENABLE
	QString mTokenizerDirectoryPath;
	bool mTokenizerEnabled;
	std::unique_ptr<ESTextEncoder> mTextEncoder;

	std::vector<ESTextEncoder::TextEncodedResult> mDatabaseTagsEmbeddingCache;
	std::vector<std::unordered_set<uint16_t>> mSearchTagIndices;
	QStringList mSearchTags;
	ESTextEncoder::TextEncodedResult mSearchTagsEmbeddings;
	QMutex mDatabaseTagsEmbeddingCacheMutex;
#ifdef HNSWLIB_ENABLED
	std::unique_ptr<hnswlib::InnerProductSpace> mHnswSpace;
	std::unique_ptr<hnswlib::HierarchicalNSW<float>> mHnswIndex;
	std::unordered_set<ESFileInfoId> mHnswSearchResults;
	QFuture<void> mHnswIndexUpdateFuture;
	std::atomic_bool mHnswIndexUpdating;
	std::atomic_bool mHnswIndexUpdatingAbortRequested;
#endif // HNSWLIB_ENABLED
#endif // IMAGETAGGER_ENABLE

	/********************************* METHODS ***********************************/

#ifdef IMAGETAGGER_ENABLE
	void onDatabaseTagsHaveChanged();
#ifdef HNSWLIB_ENABLED
	bool loadHnswIndex();
	void onImageTaggerManagerLoadingProgress(int pLoadedCount, int pLoadingCount);
	void updateHnswSearchResults();
#endif // HNSWLIB_ENABLED
#endif // IMAGETAGGER_ENABLE
};