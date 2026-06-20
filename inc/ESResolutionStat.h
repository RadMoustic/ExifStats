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

class ESResolutionStat : public ESStat
{
public:
	struct ESWidthStat
	{
		static unsigned short getFileValue(const ESFileInfo& pFile) { return pFile.mExif.mWidth; }
	};

	struct ESHeightStat
	{
		static unsigned short getFileValue(const ESFileInfo& pFile) { return pFile.mExif.mHeight; }
	};

	ESCountStatComponent<ESStringId, ESResolutionStat> mCountComp;
	ESMinMaxStatComponent<unsigned short, ESWidthStat> mMinMaxWidthComp;
	ESMinMaxStatComponent<unsigned short, ESHeightStat> mMinMaxHeightComp;

	ESResolutionStat()
	{
		addComponent(&mCountComp);
		addComponent(&mMinMaxWidthComp);
		addComponent(&mMinMaxHeightComp);
	}

	static ESStringId getFileValue(const ESFileInfo& pFile) { return pFile.mResolutionStr; }
	static QString getValueLabel(const ESStringId& pValue) { return pValue.getString(); }
};