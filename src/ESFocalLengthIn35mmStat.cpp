#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESFocalLengthIn35mmStat.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

/*static*/ std::map<uint8_t, float> ESFocalLengthIn35mmStat::msCameraModelsTo35mmFocalFactors;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

/*static*/ int ESFocalLengthIn35mmStat::get35mmFocalLength(const ESFileInfo& pFile)
{
	if (pFile.mExif.mFocalLengthIn35mm > 0)
		return pFile.mExif.mFocalLengthIn35mm;
	auto&& lItFound = msCameraModelsTo35mmFocalFactors.find(pFile.mCameraModelIdx);
	float lTo35mmFactor = lItFound != msCameraModelsTo35mmFocalFactors.end() ? lItFound->second : 1.f;
	return pFile.mExif.mFocalLengthIn35mm > 0 ? pFile.mExif.mFocalLengthIn35mm : int(round(pFile.mExif.mFocalLength * lTo35mmFactor));
}
