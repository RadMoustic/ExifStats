#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESExifStat.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

struct FileInfo;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ExifStatCountLensModel : public ExifStat
{
public:
	ExifStatCountComponent<StringId, ExifStatCountLensModel> mCountComp;

	ExifStatCountLensModel()
	{
		addComponent(&mCountComp);
	}

	static StringId getFileValue(const FileInfo& pFile) { return pFile.mExif.mLensModel; }
	static uint8_t getFileValueIndex(const FileInfo& pFile) { return pFile.mLensModelIdx; }
	static QString getValueLabel(StringId aValue) { return aValue.getString(); }
};