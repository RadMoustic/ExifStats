#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESFilter.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

template<class ExifStatType, typename T>
class ESListFilter: public ESFilter
{
public:
	virtual bool isEnabled() const override
	{
		return !mVectorFilters.empty();
	}

	virtual void reset() override
	{
		resetFilters();
	}

	const QMap<T, bool>& getFilters() const { return mFilters; }

	void resetFilters() { setFilters(QMap<T, bool>(), QVector<T>()); }

	void setFilters(const QMap<T, bool>& pFilters, const QVector<T>& pAllValues)
	{
		mFilters = pFilters;
		mVectorFilters.clear();
		mAllValues = pAllValues;
		mVectorFilters.resize(pAllValues.size());
		bool lAllTrue = true;
		for(bool lFilter: pFilters)
		{
			if(!lFilter)
			{
				lAllTrue = false;
				break;
			}
		}
		for(int i = 0 ; i < pAllValues.size() ; ++i)
		{
			auto lItFound = mFilters.find(pAllValues[i]);
			mVectorFilters[i] = lItFound != mFilters.end() ? *lItFound : lAllTrue;
		}
	}

	virtual bool isFileFilteredOut(const ESFileInfo& pFile) const override
	{
		if (mVectorFilters.empty())
			return false;
		auto lValueIndex = ExifStatType::getFileValueIndex(pFile);
		return !mVectorFilters[lValueIndex];
	}

	virtual QJsonObject serialize() const override
	{
		QJsonObject lResult;

		lResult["Keys"] = toJsonArray(mFilters.keys());
		lResult["Values"] = toJsonArray(mFilters.values());
		
		return lResult;
	}

	virtual bool deserialize(const QJsonObject& pJson) override
	{
		QVector<T> lKeys;
		QVariantList lValues;
		VALIDATE_JSONVALUE(pJson, "Keys", lKeys);
		VALIDATE_JSONVALUE(pJson, "Values", lValues);
		if (lKeys.size() != lValues.size())
		{
			qWarning("List Filter Key/Values mismatch in Json"); \
			return false;
		}
		
		QMap<T, bool> lFilters;
		for (int i = 0; i < lKeys.size(); ++i)
		{
			lFilters.insert(lKeys[i], lValues[i].toBool());
		}

		setFilters(lFilters, mAllValues);

		return true;
	}

private:
	QMap<T, bool> mFilters;
	std::vector<bool> mVectorFilters;
	QVector<T> mAllValues;
};