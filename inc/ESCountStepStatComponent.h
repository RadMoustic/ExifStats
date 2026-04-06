#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESCountStatComponent.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

struct ESFileInfo;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

template<typename T, class Derived>
class ESCountStepStatComponent : public ESCountStatComponent<T, Derived>
{
public:
	typedef ESCountStatComponent<T, Derived> Super;

	T mStep;
	bool mAddFileCategories = false;
	bool mFillEmptySteps = false;
    T mMinFillValue = T();
    T mMaxFillValue = T();

	virtual void addFileCategory(const ESFileInfo& pFile) override
	{
		if(mAddFileCategories)
		{
			T lFileValue = Derived::getFileValue(pFile);

			T lRoundedToStepFileValue = (lFileValue / mStep) * mStep;
			Super::mValueCounters[lRoundedToStepFileValue] += 0;
		}
	}

	virtual void addFile(const ESFileInfo& pFile) override
	{
		T lFileValue = Derived::getFileValue(pFile);

		T lRoundedToStepFileValue = (lFileValue / mStep) * mStep;
		Super::mValueCounters[lRoundedToStepFileValue] += 1;
	}

	virtual void onAllFilesAdded() override
	{
		if(!Super::mValueCounters.empty() && mFillEmptySteps)
		{
			T lFirstValue = Super::mValueCounters.begin()->first;
			T lLastValue = Super::mValueCounters.rbegin()->first;
			if(mMinFillValue != mMaxFillValue)
			{
				lFirstValue = std::max(mMinFillValue, lFirstValue);
				lLastValue = std::min(mMaxFillValue, lLastValue);
			}
			assert((lLastValue - lFirstValue) / mStep < 1000000 && "Set Min/MaxFillValue");
			for(T lValue = lFirstValue; lValue < lLastValue; lValue += mStep)
			{
				Super::mValueCounters[lValue] += 0;
			}
		}

		Super::onAllFilesAdded();
	}
};
