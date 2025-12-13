#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESExifStat.h"
#include "ESExifStatCountComponent.h"
#include "ESStringPool.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

struct FileInfo;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ExifStatCountCameraModel : public ExifStat
{
public:
	ExifStatCountComponent<StringId, ExifStatCountCameraModel> mCountComp;

	ExifStatCountCameraModel()
	{
		addComponent(&mCountComp);
	}

	static StringId getFileValue(const FileInfo& pFile) { return pFile.mExif.mCameraModel; }
	static uint8_t getFileValueIndex(const FileInfo& pFile) { return pFile.mCameraModelIdx; }
	static QString getValueLabel(StringId aValue) { return aValue.getString(); }
};