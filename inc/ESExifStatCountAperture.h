#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESExifStat.h"
#include "ESExifStatMinMaxComponent.h"
#include "ESExifStatCountComponent.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/


/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESExifStatCountAperture : public ExifStat
{
public:
	ExifStatMinMaxComponent<float, ESExifStatCountAperture> mMinMaxComp;
	ExifStatCountComponent<float, ESExifStatCountAperture> mCountComp;

	ESExifStatCountAperture()
	{
		addComponent(&mMinMaxComp);
		addComponent(&mCountComp);
	}

	static float getFileValue(const FileInfo& pFile) { return pFile.mExif.mFNumber; }
	static QString getValueLabel(float aValue) { return QString::number(aValue); }
};