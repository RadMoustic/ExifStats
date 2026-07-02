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

class ESShutterSpeedStat : public ESStat
{
public:
	ESMinMaxStatComponent<float, ESShutterSpeedStat> mMinMaxComp;
	ESCountStatComponent<float, ESShutterSpeedStat, std::unordered_map<float, int>> mCountComp;

	ESShutterSpeedStat()
	{
		addComponent(&mMinMaxComp);
		addComponent(&mCountComp);
	}

	static float getFileValue(const ESFileInfo& pFile) { return pFile.mExif.mShutterSpeedValue; }
	static QString getValueLabel(float pValue) { return pValue > 1.f ? QString("1/%1").arg(pValue) : QString::number(1.f / pValue, 'f', 1); }
};