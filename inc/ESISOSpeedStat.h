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

class ESISOSpeedStat : public ESStat
{
public:
	ESMinMaxStatComponent<unsigned short, ESISOSpeedStat> mMinMaxComp;
	ESCountStatComponent<unsigned short, ESISOSpeedStat, std::unordered_map<unsigned short, int>> mCountComp;

	ESISOSpeedStat()
	{
		addComponent(&mMinMaxComp);
		addComponent(&mCountComp);
	}

	static unsigned short getFileValue(const ESFileInfo& pFile) { return pFile.mExif.mISOSpeedRatings; }
	static QString getValueLabel(unsigned short pValue) { return QString::number(pValue); }
};