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

	virtual void addFileCategory(const ESFileInfo& pFile) override
	{
		T lFileValue = Derived::getFileValue(pFile);

		T lRoundedToStepFileValue = (lFileValue / mStep) * mStep;
		Super::mValueCounters[lRoundedToStepFileValue] += 0;
	}

	virtual void addFile(const ESFileInfo& pFile) override
	{
		T lFileValue = Derived::getFileValue(pFile);

		T lRoundedToStepFileValue = (lFileValue / mStep) * mStep;
		Super::mValueCounters[lRoundedToStepFileValue] += 1;
	}
};