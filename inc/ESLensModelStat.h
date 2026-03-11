#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESStat.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

struct ESFileInfo;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESLensModelStat : public ESStat
{
public:
	ESCountStatComponent<ESStringId, ESLensModelStat> mCountComp;

	ESLensModelStat()
	{
		addComponent(&mCountComp);
	}

	static ESStringId getFileValue(const ESFileInfo& pFile) { return pFile.mExif.mLensModel; }
	static uint8_t getFileValueIndex(const ESFileInfo& pFile) { return pFile.mLensModelIdx; }
	static QString getValueLabel(ESStringId aValue) { return aValue.getString(); }
};