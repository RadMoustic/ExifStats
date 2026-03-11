#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESStat.h"
#include "ESCountStatComponent.h"
#include "ESStringPool.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

struct ESFileInfo;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESCameraModelStat : public ESStat
{
public:
	ESCountStatComponent<ESStringId, ESCameraModelStat> mCountComp;

	ESCameraModelStat()
	{
		addComponent(&mCountComp);
	}

	static ESStringId getFileValue(const ESFileInfo& pFile) { return pFile.mExif.mCameraModel; }
	static uint8_t getFileValueIndex(const ESFileInfo& pFile) { return pFile.mCameraModelIdx; }
	static QString getValueLabel(ESStringId aValue) { return aValue.getString(); }
};