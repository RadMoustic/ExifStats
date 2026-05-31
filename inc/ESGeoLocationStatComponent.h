#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESStatComponent.h"


/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

struct ESFileInfo;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESGeoLocationStatComponent : public ESStatComponent
{
public:
	QVector<QPointF> mGeoLocations;
	QVector<const ESFileInfo*> mGeoLocationsFiles;

	virtual void addFile(const ESFileInfo& pFile) override
	{
		if(pFile.mExif.mGeoLocation.mLatitude == 0 && pFile.mExif.mGeoLocation.mLongitude == 0)
			return;
		mGeoLocations.push_back(QPointF(pFile.mExif.mGeoLocation.mLatitude, pFile.mExif.mGeoLocation.mLongitude));
		mGeoLocationsFiles.push_back(&pFile);
	}

	virtual void reset() override
	{
		mGeoLocations.clear();
		mGeoLocationsFiles.clear();
	}
};