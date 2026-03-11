#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESStat.h"
#include "ESMinMaxStatComponent.h"
#include "ESCountIntAllValuesStatComponent.h"

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
	ESCountIntAllValuesStatComponent<ESFocalLengthIn35mmStat> mCountComp;

	ESFocalLengthIn35mmStat()
	{
		mCountComp.mMinMaxComponent = &mMinMaxComp;

		addComponent(&mMinMaxComp);
		addComponent(&mCountComp);
	}

	static int getFileValue(const ESFileInfo& pFile) { return get35mmFocalLength(pFile); }
	static QString getValueLabel(int aValue) { return QString::number(aValue); }

	static std::map<uint8_t, float> msCameraModelsTo35mmFocalFactors;
	static int get35mmFocalLength(const ESFileInfo& aFile);
};