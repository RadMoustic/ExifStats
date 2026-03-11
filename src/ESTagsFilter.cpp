#include "ESExifTagsFilter.h"

// ES
#include "ESDatabase.h"
#include "ESImageCache.h"

// Qt
#include <QtConcurrent>

/********************************************************************************/

ESTagsFilter::ESTagsFilter()
	: mTokenizerEnabled(false)
	, mMinSimilarityScore(0.3f)
{
	mTokenizerDirectoryPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/Tokenizer");

#ifdef IMAGETAGGER_ENABLE
	if(QDir(mTokenizerDirectoryPath).exists())
	{
		mTokenizerEnabled = true;
		QtConcurrent::run([this]()
		{
			mEngine.reset(new ESImageTagsSearchEngine(mTokenizerDirectoryPath + "/model.onnx", mTokenizerDirectoryPath + "/tokenizer.json", mTokenizerDirectoryPath + "/config.json"));
			onDatabaseTagsHaveChanged();
			(void)connect(&ESDatabase::getInstance(), &ESDatabase::tagsChanged, this, &ESTagsFilter::onDatabaseTagsHaveChanged, Qt::QueuedConnection);
		});			
	}
#endif // IMAGETAGGER_ENABLE
}

/********************************************************************************/

/*virtual*/ void ESTagsFilter::reset() /*override*/
{
	mSearchString = "";
	mMinSimilarityScore = 0.3f;
	ESImageCache::getInstance().resetSearchSimilarityScores();
}

/********************************************************************************/

/*virtual*/ bool ESTagsFilter::isFileFilteredOut(const ESFileInfo& pFile) const /*override*/
{
	if (mSearchString.isEmpty())
		return false;

#ifdef IMAGETAGGER_ENABLE
	if(mEngine)
	{
		if(pFile.mEmbeddings.size() == mSearchTagsEmbeddings.mEmbedding.size())
		{
			float lSimilarityScore = mSearchTagsEmbeddings.computeSimilarityScore(pFile.mEmbeddings);
			ESImageCache::getInstance().getImage(pFile.mFilePath)->mCurrentSearchSimilarity = lSimilarityScore;
			return lSimilarityScore < mMinSimilarityScore;
		}

		if(mSearchTagsEmbeddings.mEmbedding.size() > 0)
			return true;

		for (const std::unordered_set<uint16_t>& lSearchTags : mSearchTagIndices)
		{
			bool lHasAtLeastOneSearchTag = false;
			for (uint16_t lTagIndex : pFile.mTagIndexes)
			{
				if (lSearchTags.contains(lTagIndex))
				{
					lHasAtLeastOneSearchTag = true;
					break;
				}
			}
			if (!lHasAtLeastOneSearchTag)
				return true;
		}

		return false;
	}
	else
#endif // IMAGETAGGER_ENABLE
	{
		QString lTags = ESDatabase::getInstance().getTagsLabels(pFile.mTagIndexes).join(" ");
		for (const QString& lPathPart : mSearchString.split(" ", Qt::SkipEmptyParts))
		{
			if (lTags.contains(lPathPart, Qt::CaseInsensitive))
				return false;
		}

		return true;
	}
}

/********************************************************************************/

/*virtual*/ QJsonObject ESTagsFilter::serialize() const /*override*/
{
	QJsonObject lResult;
	lResult["SearchString"] = mSearchString;
	return lResult;
}

/********************************************************************************/

/*virtual*/ bool ESTagsFilter::deserialize(const QJsonObject& pJson) /*override*/
{
	VALIDATE_JSONVALUE(pJson, "SearchString", mSearchString);

	return true;
}

/********************************************************************************/

QString ESTagsFilter::getSearchString() const
{
	return mSearchString;
}

/********************************************************************************/

void ESTagsFilter::setSearchString(const QString& pSearchString)
{
#ifdef IMAGETAGGER_ENABLE
	if(!mDatabaseTagsEmbeddingCacheMutex.tryLock())
		return;
#endif // IMAGETAGGER_ENABLE

	mSearchString = pSearchString;

#ifdef IMAGETAGGER_ENABLE
	if (mEngine)
	{
		mSearchTagIndices.clear();
		mSearchTags.clear();
		ESImageCache::getInstance().resetSearchSimilarityScores();

		if(mDatabaseTagsEmbeddingCache.size() > 0)
		{
			for (const QString& lSearchTag : mSearchString.split(" ", Qt::SkipEmptyParts))
			{
				mSearchTagIndices.emplace_back();

				std::vector<std::pair<float, uint16_t>> lScores;
				ESImageTagsSearchEngine::TextEncodedResult lSearchTagEmbedding = mEngine->encode(lSearchTag);
				for (uint16_t i = 0, e = uint16_t(mDatabaseTagsEmbeddingCache.size()); i < e ; ++i)
				{
					const ESImageTagsSearchEngine::TextEncodedResult& lDatabaseTagEmbedding = mDatabaseTagsEmbeddingCache[i];
					float lSimilarityScore = lSearchTagEmbedding.computeSimilarityScore(lDatabaseTagEmbedding);
					lScores.emplace_back(lSimilarityScore, i);
				}

				std::sort(lScores.begin(), lScores.end(),
					[](const std::pair<float, uint16_t>& a, const std::pair<float, uint16_t>& b)
					{
						return a.first > b.first;
					});

				for (const std::pair<float, uint16_t>& lScore : lScores)
				{
					if (lScore.first > 0.6)
					{
						mSearchTagIndices.back().insert(lScore.second);
						QString lTagLabel = ESDatabase::getInstance().getTagLabel(lScore.second);
						mSearchTags << lTagLabel;
						qDebug() << "Tag Score: " << lTagLabel << "=" << lScore.first;
						if (mSearchTagIndices.back().size() >= 3)
							break;
					}
				}
			}
		}

		mSearchTagsEmbeddings = mEngine->encode(mSearchString);

		mDatabaseTagsEmbeddingCacheMutex.unlock();
	}
#endif // IMAGETAGGER_ENABLE
}

/********************************************************************************/

QStringList ESTagsFilter::getTagsFound() const
{
#ifdef IMAGETAGGER_ENABLE
	return mEngine ? mSearchTags : QStringList();
#else
	return mTagsInclusiveFilters;
#endif // IMAGETAGGER_ENABLE
}

/********************************************************************************/

#ifdef IMAGETAGGER_ENABLE
void ESTagsFilter::onDatabaseTagsHaveChanged()
{
	std::scoped_lock lock(mDatabaseTagsEmbeddingCacheMutex);
	ESDatabase& lDB = ESDatabase::getInstance();

	mDatabaseTagsEmbeddingCache.clear();
	std::vector<QString> lAllTags;
	lDB.getAllTags(lAllTags);
	mDatabaseTagsEmbeddingCache.resize(lAllTags.size());
	constexpr const int cNumThreads = 4;
	QFuture<void> lThreadResult[cNumThreads];
	for (int lThreadIdx = 0; lThreadIdx < cNumThreads; ++lThreadIdx)
	{
		lThreadResult[lThreadIdx] = QtConcurrent::run([this, &lAllTags, lThreadIdx, cNumThreads]()
		{
			for (int i = lThreadIdx; i < lAllTags.size(); i += cNumThreads)
			{
				mDatabaseTagsEmbeddingCache[i] = mEngine->encode(lAllTags[i]);
			}
		});
	}

	// lAllTags must not be destroyed until everything is done
	for (int lThreadIdx = 0; lThreadIdx < cNumThreads; ++lThreadIdx)
		lThreadResult[lThreadIdx].waitForFinished();
}
#endif // IMAGETAGGER_ENABLE

/********************************************************************************/

bool ESTagsFilter::isTokenizerEnabled() const
{
#ifdef IMAGETAGGER_ENABLE
	return mTokenizerEnabled;
#else
	return false;
#endif // IMAGETAGGER_ENABLE
}
