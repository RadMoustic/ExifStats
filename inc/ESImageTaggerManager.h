#pragma once

#if defined(IMAGETAGGER_ENABLE) && !defined(ES_READONLY)

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESImageLoader.h"

// Qt
#include <QImage>
#include <QString>

// Stl
#include <memory>
#include <vector>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESImageTagger;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESImageTaggerManager : public ESImageLoader
{
	Q_OBJECT
public:
	/********************************* METHODS ***********************************/

	static ESImageTaggerManager& getInstance();

	ESImageTaggerManager();
	virtual ~ESImageTaggerManager();

	bool isEnabled() const;

	void initialize();
	void retag();

	void processImage(const QImage& pImage, std::vector<uint16_t>& pTagsOut, ESEmbeddings& pEmbeddingsOut);
	QStringList getTagsLabels(const QVector<uint16_t>& pTags);

	void updateDatabaseMissingTags();
	
private:
	/******************************** ATTRIBUTES **********************************/

	bool mEnabled;
	QString mTaggerDirectoryPath;
	std::vector<std::shared_ptr<ESImageTagger>> mTaggers;
	std::vector<QString> mAllTagLabels;
	std::unordered_map<ESImageTagger*, std::vector<uint16_t>> mTaggerTagIndexesInAllLabels;
	int mMaxSizeOfAllTaggerInputs;

	/********************************* METHODS ***********************************/

	void loadTaggersFromDirectory(const QString& pDirectoryPath);
	void addTagger(std::shared_ptr<ESImageTagger> pTagger);
	void addTagger(const QString& pTaggerFilePath);
	void onImageCacheLoadingProgress(int pCachedCount, int pCachingCount);
	void onImageCacheUpdateFinished();
	void onDatabaseProcessingChanged();
	void updateAllTagLabels();
	void convertTagsToAllTagIndexes(std::vector<uint16_t>& pTaggerTags, ESImageTagger* pTagger);
	virtual void internalLoadImage(const std::shared_ptr<ESImage>& pImage, std::atomic_int32_t& pNumAsyncTaskStarted) override;
};

#endif // defined(IMAGETAGGER_ENABLE) && !defined(ES_READONLY)
