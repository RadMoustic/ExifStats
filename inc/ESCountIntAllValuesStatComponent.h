#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESCountStatComponent.h"
#include "ESMinMaxStatComponent.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/


/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

template<class Derived>
class ESCountIntAllValuesStatComponent : public ESCountStatComponent<int, Derived>
{
public:
	typedef ESCountStatComponent<int, Derived> Super;

	const ESMinMaxStatComponent<int, Derived>* mMinMaxComponent;

	virtual void onAllFilesAdded() override
	{
		if (!mMinMaxComponent->isValid())
		{
			Super::mCounters.resize(0);
			Super::mCounterLabels.resize(0);
			return;
		}
		int lMaxValue = mMinMaxComponent->getMaxValue();
		Super::mCounters.resize(lMaxValue+1);
		Super::mCounterLabels.resize(lMaxValue+1);

		for (int i = 0; i <= lMaxValue; ++i)
			Super::mCounterLabels[i] = QString::number(i);

		for (const auto& valueCount : Super::mValueCounters)
			if(valueCount.second > 0)
				Super::mCounters[valueCount.first] = valueCount.second;
	}
};