#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESExifStat.h"
#include "ESExifStatMinMaxComponent.h"
#include "ESExifStatCountStepComponent.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/


/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ExifStatCountDateTime : public ExifStat
{
public:
	static const char* msTimeFormat;

	ExifStatMinMaxComponent<uint64_t, ExifStatCountDateTime> mMinMaxComp;
	ExifStatCountStepComponent<uint64_t, ExifStatCountDateTime> mCountComp;

	ExifStatCountDateTime()
	{
		addComponent(&mMinMaxComp);
		addComponent(&mCountComp);

		mCountComp.mStep = 3600 * 24 * 30;
		mMinMaxComp.mValidMinValue = 1;
		mMinMaxComp.mValidMaxValue = 0xfffffffffffff000 - 1;
	}

	static uint64_t getFileValue(const FileInfo& pFile) { return pFile.mExif.mDateTime; }
	static QString getValueLabel(uint64_t aValue) { return QDateTime::fromSecsSinceEpoch(aValue).toString(msTimeFormat); }
};