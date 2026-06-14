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

template<typename T, class Derived>
class ESCountStatComponent : public ESCounterStatComponentInterface
{
public:
	bool mIgnoreEmptyCategories = true;

	virtual void addFileCategory(const ESFileInfo& pFile) override
	{
		if(!mIgnoreEmptyCategories)
		{
			T lFileValue = Derived::getFileValue(pFile);
			mValueCounters[lFileValue] += 0;
		}
	}

	virtual void addFile(const ESFileInfo& pFile) override
	{
		T lFileValue = Derived::getFileValue(pFile);
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
		mValues.resize(mValueCounters.size());
		mCounters.resize(mValueCounters.size());
		mCounterLabels.resize(mValueCounters.size());

		std::vector<std::tuple<T, int, QString>> lSortedValues;

		for (const auto& lValueCount : mValueCounters)
			lSortedValues.emplace_back(lValueCount.first, lValueCount.second, Derived::getValueLabel(lValueCount.first));

		std::sort(lSortedValues.begin(), lSortedValues.end(), [](const auto& a, const auto& b) { return std::get<0>(a) < std::get<0>(b); });

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

	std::unordered_map<T, int> mValueCounters;
	QVector<QString> mCounterLabels;
	QVector<int> mCounters;
	QVector<T> mValues;
};