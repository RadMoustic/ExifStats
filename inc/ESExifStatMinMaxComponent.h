#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESExifStatComponent.h"
#include "ESFileInfo.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/


/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

template<typename T, class Derived>
class ExifStatMinMaxComponent : public ExifStatComponent
{
public:
	T mValidMinValue = std::numeric_limits<T>::min();
	T mValidMaxValue = std::numeric_limits<T>::max();

	virtual void addFile(const FileInfo& pFile) override
	{
		T lFileValue = Derived::getFileValue(pFile);
		if(lFileValue >= mValidMinValue && lFileValue <= mValidMaxValue)
		{
			mMinValue = std::min(mMinValue, lFileValue);
			mMaxValue = std::max(mMaxValue, lFileValue);
			mIsValid = true;
		}
	}

	virtual void reset() override
	{
		mMinValue = std::numeric_limits<T>::max();
		mMaxValue = std::numeric_limits<T>::min();
		mIsValid = false;
	}

	T getMinValue() const { return mMinValue; }
	T getMaxValue() const { return mMaxValue; }

	bool isValid() const { return mIsValid; }

protected:
	T mMinValue = 0;
	T mMaxValue = 0;
	bool mIsValid = false;
};