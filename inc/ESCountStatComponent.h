#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESExifStatCounterComponentInterface.h"

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

	virtual void addFileCategory(const ESFileInfo& pFile) override
	{
		T lFileValue = Derived::getFileValue(pFile);
		mValueCounters[lFileValue] += 0;
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
	}

	virtual void onAllFilesAdded() override
	{
		mCounters.resize(mValueCounters.size());
		mCounterLabels.resize(mValueCounters.size());

		int i = 0;
		for (const auto& valueCount : mValueCounters)
		{
			mCounters[i] = valueCount.second;
			mCounterLabels[i] = Derived::getValueLabel(valueCount.first);
			++i;
		}

	}

	virtual const QVector<int>& getCounters() const override { return mCounters; }
	virtual const QVector<QString>& getLabels() const override { return mCounterLabels; }

protected:

	std::map<T, int> mValueCounters;
	QVector<QString> mCounterLabels;
	QVector<int> mCounters;
};