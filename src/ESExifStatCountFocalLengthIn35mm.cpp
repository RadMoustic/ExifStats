#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESExifStatCountFocalLengthIn35mm.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

/*static*/ std::map<uint8_t, float> ESFocalLengthIn35mmStat::msCameraModelsTo35mmFocalFactors;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

/*static*/ int ESFocalLengthIn35mmStat::get35mmFocalLength(const ESFileInfo& aFile)
{
	if (aFile.mExif.mFocalLengthIn35mm > 0)
		return aFile.mExif.mFocalLengthIn35mm;
	auto&& itFound = msCameraModelsTo35mmFocalFactors.find(aFile.mCameraModelIdx);
	float lTo35mmFactor = itFound != msCameraModelsTo35mmFocalFactors.end() ? itFound->second : 1.f;
	return aFile.mExif.mFocalLengthIn35mm > 0 ? aFile.mExif.mFocalLengthIn35mm : int(round(aFile.mExif.mFocalLength * lTo35mmFactor));
}
