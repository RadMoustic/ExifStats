#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESStat.h"
#include "ESMinMaxStatComponent.h"
#include "ESCountStatComponent.h"

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
	ESCountStatComponent<float, ESApertureStat, std::unordered_map<float, int>> mCountComp;

	ESApertureStat()
	{
		addComponent(&mMinMaxComp);
		addComponent(&mCountComp);
	}

	static float getFileValue(const ESFileInfo& pFile) { return pFile.mExif.mFNumber; }
	static QString getValueLabel(float pValue) { return QString::number(pValue); }
};