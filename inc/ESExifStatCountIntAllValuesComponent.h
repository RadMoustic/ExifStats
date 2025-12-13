#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESExifStatCountComponent.h"
#include "ESExifStatMinMaxComponent.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/


/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

template<class Derived>
class ExifStatCountIntAllValuesComponent : public ExifStatCountComponent<int, Derived>
{
public:
	typedef ExifStatCountComponent<int, Derived> Super;

	const ExifStatMinMaxComponent<int, Derived>* mMinMaxComponent;

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