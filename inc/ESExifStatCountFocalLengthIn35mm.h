#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESExifStat.h"
#include "ESExifStatMinMaxComponent.h"
#include "ESExifStatCountIntAllValuesComponent.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/


/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ExifStatCountFocalLengthIn35mm : public ExifStat
{
public:
	
	ExifStatMinMaxComponent<int, ExifStatCountFocalLengthIn35mm> mMinMaxComp;
	ExifStatCountIntAllValuesComponent<ExifStatCountFocalLengthIn35mm> mCountComp;

	ExifStatCountFocalLengthIn35mm()
	{
		mCountComp.mMinMaxComponent = &mMinMaxComp;

		addComponent(&mMinMaxComp);
		addComponent(&mCountComp);
	}

	static int getFileValue(const FileInfo& pFile) { return get35mmFocalLength(pFile); }
	static QString getValueLabel(int aValue) { return QString::number(aValue); }

	static std::map<uint8_t, float> msCameraModelsTo35mmFocalFactors;
	static int get35mmFocalLength(const FileInfo& aFile);
};