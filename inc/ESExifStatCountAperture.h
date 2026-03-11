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

class ESApertureStat : public ESStat
{
public:
	ESMinMaxStatComponent<float, ESApertureStat> mMinMaxComp;
	ESCountStatComponent<float, ESApertureStat> mCountComp;

	ESApertureStat()
	{
		addComponent(&mMinMaxComp);
		addComponent(&mCountComp);
	}

	static float getFileValue(const ESFileInfo& pFile) { return pFile.mExif.mFNumber; }
	static QString getValueLabel(float aValue) { return QString::number(aValue); }
};