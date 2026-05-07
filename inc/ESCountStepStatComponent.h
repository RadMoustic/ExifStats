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
			bool lFirstValueSet = false;
			bool lLastValueSet = false;
			T lFirstValue = T();
			T lLastValue = T();
			for(const auto& lValueCount : Super::mValueCounters)
			{
				if(!lFirstValueSet)
				{
					lFirstValue = lValueCount.first;
					lFirstValueSet = true;
				}
				else
				{
					lFirstValue = std::min(lFirstValue, lValueCount.first);
				}
				if(!lLastValueSet)
				{
					lLastValue = lValueCount.first;
					lLastValueSet = true;
				}
				else
				{
					lLastValue = std::max(lLastValue, lValueCount.first);
				}
			}
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
