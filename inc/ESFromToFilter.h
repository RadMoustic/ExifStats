#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESFilter.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/


/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

template<typename T, class ExifStatType>
class ESFromToFilter : public ESFilter
{
public:
	T mFilterFrom;
	T mFilterTo;

	bool mFilterOutInvalidValues;
	T mMinValue = std::numeric_limits<T>::min();
	T mMaxValue = std::numeric_limits<T>::max();

	ESFromToFilter()
	 : mFilterFrom(std::numeric_limits<T>::min())
	 , mFilterTo(std::numeric_limits<T>::max())
	 , mInvalidValue(std::numeric_limits<T>::max())
	 , mInvalidValueSet(false)
	 , mFilterOutInvalidValues(false)
	{

	}

	void setInvalidValue(T pValue)
	{
		mInvalidValue = pValue;
		mInvalidValueSet = true;
	}

	virtual bool isEnabled() const override
	{
		return mFilterFrom != mMinValue || mFilterTo != mMaxValue || (mInvalidValueSet && mFilterOutInvalidValues);
	}

	virtual void reset() override
	{
		mFilterFrom = mMinValue;
		mFilterTo = mMaxValue;
		mFilterOutInvalidValues = false;
	}

	virtual bool isFileFilteredOut(const ESFileInfo& pFile) const override
	{
		auto lFileValue = ExifStatType::getFileValue(pFile);
		bool lIsInvalidValue = mInvalidValueSet && lFileValue == mInvalidValue;
		return		(lIsInvalidValue && mFilterOutInvalidValues)
				||	(	!lIsInvalidValue
					&&	(	lFileValue < mFilterFrom
						||	lFileValue > mFilterTo)
					);
	}

	virtual QJsonObject serialize() const override
	{
		QJsonObject lResult;
		lResult["From"] = toJsonValue(mFilterFrom);
		lResult["To"] = toJsonValue(mFilterTo);
		lResult["FilterOutInvalidValues"] = mFilterOutInvalidValues;
		return lResult;
	}

	virtual bool deserialize(const QJsonObject& pJson) override
	{
		mFilterFrom = fromJsonValue<T>(pJson["From"]);
		mFilterTo = fromJsonValue<T>(pJson["To"]);
		mFilterOutInvalidValues = fromJsonValue<bool>(pJson["FilterOutInvalidValues"]);
		return true;
	}

private:
	T mInvalidValue;
	bool mInvalidValueSet;
};