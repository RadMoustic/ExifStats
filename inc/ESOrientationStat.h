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

struct ESFileInfo;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// Bool: True => Portrait, False => Landscape
class ESOrientationStat : public ESStat
{
public:
	ESCountStatComponent<bool, ESOrientationStat> mCountComp;

	ESOrientationStat()
	{
		addComponent(&mCountComp);
	}

	static bool getFileValue(const ESFileInfo& pFile) { return pFile.mExif.mOrientation == UpperRight || pFile.mExif.mOrientation == LowerLeft; }
	static QString getValueLabel(bool aValue) { return aValue ? "Portrait" : "Landscape"; }
};