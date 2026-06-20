#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESFilter.h"
#include "ESDatabase.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

template<class ExifStatType, typename T>
class ESListFilter: public ESFilter
{
public:
	std::function<const QVector<T>&()> mDeserializeGetAllValuesCallback;

	virtual bool isEnabled() const override
	{
		return !mFilters.empty();
	}

	virtual void reset() override
	{
		resetFilters();
	}

	const QMap<T, bool>& getFilters() const { return mFilters; }

	void resetFilters() { setFilters(QMap<T, bool>(), QVector<T>()); }

	void setFilters(const QMap<T, bool>& pFilters, const QVector<T>& pAllValues)
	{
		bool lAllTrue = true;
		for(bool lFilter: pFilters)
		{
			if(!lFilter)
			{
				lAllTrue = false;
				break;
			}
		}
		setFiltersInternal(pFilters, pAllValues, lAllTrue);
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

		QVector<T> lEnabledFilters;
		for(typename QMap<T, bool>::const_iterator it = mFilters.begin() ; it != mFilters.end() ; ++it)
			if(it.value())
				lEnabledFilters.push_back(it.key());

		lResult["EnabledFilters"] = toJsonArray(lEnabledFilters);
		
		return lResult;
	}

	virtual bool deserialize(const QJsonObject& pJson) override
	{
		QVector<T> lEnabledFilters;
		VALIDATE_JSONVALUE(pJson, "EnabledFilters", lEnabledFilters);

		QMap<T, bool> lFilters;
		for (int i = 0; i < lEnabledFilters.size(); ++i)
		{
			lFilters.insert(lEnabledFilters[i], true);
		}

		setFiltersInternal(lFilters, mDeserializeGetAllValuesCallback(), false);

		return true;
	}

private:
	QMap<T, bool> mFilters;
	std::vector<bool> mVectorFilters;
	QVector<T> mAllValues;

	void setFiltersInternal(const QMap<T, bool>& pFilters, const QVector<T>& pAllValues, bool pAllTrue)
	{
		mFilters = pFilters;
		mVectorFilters.clear();
		mAllValues = pAllValues;
		mVectorFilters.resize(pAllValues.size());
		for (int i = 0; i < pAllValues.size(); ++i)
		{
			auto lItFound = mFilters.find(pAllValues[i]);
			mVectorFilters[i] = lItFound != mFilters.end() ? *lItFound : pAllTrue;
		}
	}
};