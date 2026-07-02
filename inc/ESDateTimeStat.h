#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESStat.h"
#include "ESMinMaxStatComponent.h"
#include "ESCountStepStatComponent.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/


/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESDateTimeStat : public ESStat
{
public:
	static const char* msTimeFormat;

	ESMinMaxStatComponent<uint64_t, ESDateTimeStat> mMinMaxComp;
	ESCountStepStatComponent<uint64_t, ESDateTimeStat, std::unordered_map<uint64_t, int>> mCountComp;

	ESDateTimeStat()
	{
		addComponent(&mMinMaxComp);
		addComponent(&mCountComp);

		mCountComp.mStep = 3600 * 24 * 30;
		mCountComp.mFillEmptySteps = true;
		mCountComp.mMinFillValue = 0;
		mCountComp.mMaxFillValue = QDateTime::currentSecsSinceEpoch();
		mMinMaxComp.mValidMinValue = 1;
		mMinMaxComp.mValidMaxValue = 0xfffffffffffff000 - 1;
	}

	static uint64_t getFileValue(const ESFileInfo& pFile) { return pFile.mExif.mDateTime; }
	static QString getValueLabel(uint64_t pValue) { return QDateTime::fromSecsSinceEpoch(pValue).toString(msTimeFormat); }
};