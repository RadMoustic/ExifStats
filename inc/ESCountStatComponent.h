#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESCounterStatComponentInterface.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

struct ESFileInfo;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

template<typename T, class Derived, class CounterType = std::vector<int>>
class ESCountStatComponent : public ESCounterStatComponentInterface
{
public:
	bool mIgnoreEmptyCategories = true;
	int mMinCountCategory = -1;

	virtual void addFileCategory(const ESFileInfo& pFile) override
	{
		if(!mIgnoreEmptyCategories)
		{
			T lFileValue = Derived::getFileValue(pFile);
			if constexpr (!requires { typename CounterType::mapped_type; })
				if(size_t(lFileValue) >= mValueCounters.size())
					mValueCounters.resize(lFileValue+1);
			mValueCounters[lFileValue] += 0;
		}
	}

	virtual void addFile(const ESFileInfo& pFile) override
	{
		T lFileValue = Derived::getFileValue(pFile);
		if constexpr (!requires { typename CounterType::mapped_type; })
			if(size_t(lFileValue) >= mValueCounters.size())
				mValueCounters.resize(lFileValue+1);
		mValueCounters[lFileValue] += 1;
	}

	virtual void reset() override
	{
		mValueCounters.clear();
		mCounterLabels.clear();
		mCounters.clear();
		mValues.clear();
	}

	virtual void onAllFilesAdded() override
	{
		std::vector<std::tuple<T, int, QString>> lSortedValues;
		lSortedValues.reserve(mValueCounters.size());

		if constexpr (requires { typename CounterType::mapped_type; })
		{
			for (const auto& lValueCount : mValueCounters)
				if(mMinCountCategory < 0 || lValueCount.second >= mMinCountCategory)
					lSortedValues.emplace_back(lValueCount.first, lValueCount.second, Derived::getValueLabel(lValueCount.first));
		}
		else	
		{
			for (int i = 0,e = int(mValueCounters.size()) ; i < e ; ++i)
			{
				const auto& lValueCount = mValueCounters[i];
				if(mMinCountCategory < 0 || lValueCount >= mMinCountCategory)
					lSortedValues.emplace_back(T(i), lValueCount, Derived::getValueLabel(T(i)));
			}
		}

		std::sort(lSortedValues.begin(), lSortedValues.end(), [](const auto& a, const auto& b) { return std::get<0>(a) < std::get<0>(b); });

		mValues.resize(lSortedValues.size());
		mCounters.resize(lSortedValues.size());
		mCounterLabels.resize(lSortedValues.size());

		int i = 0;
		for (const auto& lValueCount : lSortedValues)
		{
			mValues[i] = std::get<0>(lValueCount);
			mCounters[i] = std::get<1>(lValueCount);
			mCounterLabels[i] = std::get<2>(lValueCount);
			++i;
		}

	}

	virtual const QVector<int>& getCounters() const override { return mCounters; }
	virtual const QVector<QString>& getLabels() const override { return mCounterLabels; }
	virtual const QVector<T>& getValues() const { return mValues; }

protected:

	CounterType mValueCounters;
	QVector<QString> mCounterLabels;
	QVector<int> mCounters;
	QVector<T> mValues;
};