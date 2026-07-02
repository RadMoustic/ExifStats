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

template<typename T, class Derived, class CounterType = std::vector<int>>
class ESCountStepStatComponent : public ESCountStatComponent<T, Derived, CounterType>
{
public:
	typedef ESCountStatComponent<T, Derived, CounterType> Super;

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
			if constexpr (!requires { typename CounterType::mapped_type; })
				if(size_t(lRoundedToStepFileValue) >= Super::mValueCounters.size())
					Super::mValueCounters.resize(lRoundedToStepFileValue+1);
			Super::mValueCounters[lRoundedToStepFileValue] += 0;
		}
	}

	virtual void addFile(const ESFileInfo& pFile) override
	{
		T lFileValue = Derived::getFileValue(pFile);
		T lRoundedToStepFileValue = (lFileValue / mStep) * mStep;		
		if constexpr (!requires { typename CounterType::mapped_type; })
			if(size_t(lRoundedToStepFileValue) >= Super::mValueCounters.size())
				Super::mValueCounters.resize(lRoundedToStepFileValue+1);
		Super::mValueCounters[lRoundedToStepFileValue] += 1;
	}

	virtual void onAllFilesAdded() override
	{
		if(Super::mValueCounters.size() > 0 && mFillEmptySteps)
		{
			bool lFirstValueSet = false;
			bool lLastValueSet = false;
			T lFirstValue = T();
			T lLastValue = T();
			if constexpr (requires { typename CounterType::mapped_type; })
			{
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
			}
			else
			{
				for (T i = 0,e = T(Super::mValueCounters.size()) ; i < e ; ++i)
				{
					if(!lFirstValueSet)
					{
						lFirstValue = i;
						lFirstValueSet = true;
					}
					else
					{
						lFirstValue = std::min(lFirstValue, i);
					}
					if(!lLastValueSet)
					{
						lLastValue = i;
						lLastValueSet = true;
					}
					else
					{
						lLastValue = std::max(lLastValue, i);
					}
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
