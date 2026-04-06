#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESStat.h"
#include "ESMinMaxStatComponent.h"
#include "ESCountStepStatComponent.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/


/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESFocalLengthIn35mmStat : public ESStat
{
public:
	
	ESMinMaxStatComponent<int, ESFocalLengthIn35mmStat> mMinMaxComp;
	ESCountStepStatComponent<int, ESFocalLengthIn35mmStat> mCountComp;

	ESFocalLengthIn35mmStat()
	{
		mCountComp.mFillEmptySteps = true;
		mCountComp.mStep = 1;

		addComponent(&mMinMaxComp);
		addComponent(&mCountComp);
	}

	static int getFileValue(const ESFileInfo& pFile) { return get35mmFocalLength(pFile); }
	static QString getValueLabel(int pValue) { return QString::number(pValue); }

	static std::map<uint8_t, float> msCameraModelsTo35mmFocalFactors;
	static int get35mmFocalLength(const ESFileInfo& pFile);
};