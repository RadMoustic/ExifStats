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

class ESCameraModelStat : public ESStat
{
public:
	ESCountStatComponent<uint8_t, ESCameraModelStat> mCountComp;

	ESCameraModelStat()
	{
		addComponent(&mCountComp);
	}

	static uint8_t getFileValue(const ESFileInfo& pFile) { return pFile.mCameraModelIdx; }
	static uint8_t getFileValueIndex(const ESFileInfo& pFile) { return pFile.mCameraModelIdx; }
	static QString getValueLabel(uint8_t pValue);
};