#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESExifStatCountComponent.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

struct FileInfo;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

template<typename T, class Derived>
class ExifStatCountStepComponent : public ExifStatCountComponent<T, Derived>
{
public:
	typedef ExifStatCountComponent<T, Derived> Super;

	T mStep;

	virtual void addFileCategory(const FileInfo& pFile) override
	{
		T lFileValue = Derived::getFileValue(pFile);

		T lRoundedToStepFileValue = (lFileValue / mStep) * mStep;
		Super::mValueCounters[lRoundedToStepFileValue] += 0;
	}

	virtual void addFile(const FileInfo& pFile) override
	{
		T lFileValue = Derived::getFileValue(pFile);

		T lRoundedToStepFileValue = (lFileValue / mStep) * mStep;
		Super::mValueCounters[lRoundedToStepFileValue] += 1;
	}
};