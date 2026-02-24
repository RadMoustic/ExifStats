#pragma once

#ifdef IMAGETAGGER_ENABLE

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

	QVector<uint16_t> generateImageTags(const QImage& pImage);
	QStringList getTagsLabels(const QVector<uint16_t>& pTags);

	void updateDatabaseMissingTags();
    
private:
	/******************************** ATTRIBUTES **********************************/

	bool mEnabled;
	QString mTaggerDirectoryPath;
	std::vector<std::shared_ptr<ESImageTagger>> mTaggers;
	QStringList mAllTagLabels;
	std::unordered_map<ESImageTagger*, QVector<uint16_t>> mTaggerTagIndexesInAllLabels;
	int mMaxSizeOfAllTaggerInputs;

	/********************************* METHODS ***********************************/

	void loadTaggersFromDirectory(const QString& pDirectoryPath);
	void addTagger(std::shared_ptr<ESImageTagger> pTagger);
	void addTagger(const QString& pTaggerFilePath);
	void onImageCacheLoadingProgress(int pCachedCount, int pCachingCount);
	void onImageCacheUpdateFinished();
	void updateAllTagLabels();
	void convertTagsToAllTagIndexes(QVector<uint16_t>& pTaggerTags, ESImageTagger* pTagger);
	virtual void internalLoadImage(const std::shared_ptr<ESImage>& pImage, std::atomic_int32_t& pNumAsyncTaskStarted) override;
};

#endif // IMAGETAGGER_ENABLE