#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESExifFilter.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/


/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

template<typename T, class ExifStatType>
class ExifFromToFilter : public ExifFilter
{
public:
	T mFilterFrom;
	T mFilterTo;

	ExifFromToFilter()
	 : mFilterFrom(std::numeric_limits<T>::min())
	 , mFilterTo(std::numeric_limits<T>::max())
	{

	}

	virtual void reset() override
	{
		mFilterFrom = std::numeric_limits<T>::min();
		mFilterTo = std::numeric_limits<T>::max();
	}

	virtual bool isFileFilteredOut(const FileInfo& pFile) const override
	{
		auto lFileValue = ExifStatType::getFileValue(pFile);
		return		lFileValue < mFilterFrom
				||	lFileValue > mFilterTo;
	}

	virtual QJsonObject serialize() const override
	{
		QJsonObject lResult;
		lResult["From"] = toJsonValue(mFilterFrom);
		lResult["To"] = toJsonValue(mFilterTo);
		return lResult;
	}

	virtual bool deserialize(const QJsonObject& pJson) override
	{
		mFilterFrom = fromJsonValue<T>(pJson["From"]);
		mFilterTo = fromJsonValue<T>(pJson["To"]);

		return true;
	}
};