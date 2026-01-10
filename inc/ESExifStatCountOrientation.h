#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESExifStat.h"
#include "ESExifStatCountComponent.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

struct FileInfo;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// Bool: True => Portrait, False => Landscape
class ESExifStatCountOrientation : public ExifStat
{
public:
	ExifStatCountComponent<bool, ESExifStatCountOrientation> mCountComp;

	ESExifStatCountOrientation()
	{
		addComponent(&mCountComp);
	}

	static bool getFileValue(const FileInfo& pFile) { return pFile.mExif.mOrientation == UpperRight || pFile.mExif.mOrientation == LowerLeft; }
	static QString getValueLabel(bool aValue) { return aValue ? "Portrait" : "Landscape"; }
};