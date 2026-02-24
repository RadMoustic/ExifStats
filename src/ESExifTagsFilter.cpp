#include "ESExifTagsFilter.h"

// ES
#include "ESDatabase.h"

// Qt
#include <QtConcurrent>

/********************************************************************************/

ESExifTagsFilter::ESExifTagsFilter()
	: mTokenizerEnabled(false)
{
	mTokenizerDirectoryPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/Tokenizer");

#ifdef IMAGETAGGER_ENABLE
	if(QDir(mTokenizerDirectoryPath).exists())
	{
		mTokenizerEnabled = true;
		QtConcurrent::run([this]()
		{
			mEngine.reset(new ESImageTagsSearchEngine(mTokenizerDirectoryPath + "/model.onnx", mTokenizerDirectoryPath + "/tokenizer.json"));
			onDatabaseTagsHaveChanged();
		});

		(void)connect(&ESDatabase::getInstance(), &ESDatabase::tagsChanged, this, &ESExifTagsFilter::onDatabaseTagsHaveChanged);
	}
#endif // IMAGETAGGER_ENABLE
}

/********************************************************************************/

/*virtual*/ void ESExifTagsFilter::reset() /*override*/
{
	mTagsInclusiveFilters.clear();
}

/********************************************************************************/

/*virtual*/ bool ESExifTagsFilter::isFileFilteredOut(const FileInfo& pFile) const /*override*/
{
	if (mTagsInclusiveFilters.isEmpty())
		return false;

#ifdef IMAGETAGGER_ENABLE
	if(mEngine)
	{
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
		for (const QString& lPathPart : mTagsInclusiveFilters)
		{
			if (lTags.contains(lPathPart, Qt::CaseInsensitive))
				return false;
		}

		return true;
	}
}

/********************************************************************************/

/*virtual*/ QJsonObject ESExifTagsFilter::serialize() const /*override*/
{
	QJsonObject lResult;
	lResult["TagsInclusiveFilters"] = QJsonArray::fromStringList(mTagsInclusiveFilters);
	return lResult;
}

/********************************************************************************/

/*virtual*/ bool ESExifTagsFilter::deserialize(const QJsonObject& pJson) /*override*/
{
	VALIDATE_JSONVALUE(pJson, "TagsInclusiveFilters", mTagsInclusiveFilters);

	return true;
}

/********************************************************************************/

QStringList ESExifTagsFilter::getTagsInclusiveFilters() const
{
	return mTagsInclusiveFilters;
}

/********************************************************************************/

void ESExifTagsFilter::setTagsInclusiveFilters(const QStringList& pTagsInclusiveFilters)
{
#ifdef IMAGETAGGER_ENABLE
	if(!mDatabaseTagsEmbeddingCacheMutex.tryLock())
		return;
#endif // IMAGETAGGER_ENABLE

	mTagsInclusiveFilters = pTagsInclusiveFilters;

#ifdef IMAGETAGGER_ENABLE
	if (mEngine)
	{
		mSearchTagIndices.clear();
		mSearchTags.clear();

		for (const QString& lSearchTag : mTagsInclusiveFilters)
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
		mDatabaseTagsEmbeddingCacheMutex.unlock();
	}
#endif // IMAGETAGGER_ENABLE
}

/********************************************************************************/

QStringList ESExifTagsFilter::getActualSearchedTags() const
{
#ifdef IMAGETAGGER_ENABLE
	return mEngine ? mSearchTags : mTagsInclusiveFilters;
#else
	return mTagsInclusiveFilters;
#endif // IMAGETAGGER_ENABLE
}

/********************************************************************************/

#ifdef IMAGETAGGER_ENABLE
void ESExifTagsFilter::onDatabaseTagsHaveChanged()
{
	if(!mEngine)
		return;
	std::scoped_lock lock(mDatabaseTagsEmbeddingCacheMutex);
	ESDatabase& lDB = ESDatabase::getInstance();

	mDatabaseTagsEmbeddingCache.clear();
	QStringList lAllTags;
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

bool ESExifTagsFilter::isTokenizerEnabled() const
{
#ifdef IMAGETAGGER_ENABLE
	return mTokenizerEnabled;
#else
	return false;
#endif // IMAGETAGGER_ENABLE
}