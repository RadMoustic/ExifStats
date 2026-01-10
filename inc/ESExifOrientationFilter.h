#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESExifFilter.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESExifOrientationFilter : public ExifFilter
{
public:
	enum FilterMode : int
	{
		eNoFilter = 0,
		eFilterOutTrue,
		eFilterOutFalse,
		eFilterOutBoth,
	};

	FilterMode mFilterMode = eNoFilter;

	virtual void reset() override
	{
		mFilterMode = eNoFilter;
	}

	virtual bool isFileFilteredOut(const FileInfo& pFile) const override
	{
		if(mFilterMode == eNoFilter)
			return false;

		if(mFilterMode == eFilterOutBoth)
			return true;
		
		bool lIsPortrait = pFile.mExif.mOrientation == UpperRight || pFile.mExif.mOrientation == LowerLeft;
		
		return (mFilterMode == eFilterOutTrue && lIsPortrait) || (mFilterMode == eFilterOutFalse && !lIsPortrait);
	}

	virtual QJsonObject serialize() const override
	{
		QJsonObject lResult;
		lResult["FilterMode"] = mFilterMode;
		return lResult;
	}

	virtual bool deserialize(const QJsonObject& pJson) override
	{
		VALIDATE_JSONVALUE(pJson, "FilterMode", mFilterMode);

		return true;
	}
};