#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESStat.h"
#include "ESCountStatComponent.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

struct ESFileInfo;
class ESDatabase;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESLensModelStat : public ESStat
{
public:
	ESCountStatComponent<uint8_t, ESLensModelStat> mCountComp;

	ESLensModelStat()
	{
		addComponent(&mCountComp);
	}

	static uint8_t getFileValue(const ESFileInfo& pFile) { return pFile.mLensModelIdx; }
	static uint8_t getFileValueIndex(const ESFileInfo& pFile) { return pFile.mLensModelIdx; }
	static QString getValueLabel(uint8_t pValue);
};