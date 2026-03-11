#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESExifStatComponent.h"


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
		if(pFile.mExif.mGeoLococation.mLatitude == 0 && pFile.mExif.mGeoLococation.mLongitude == 0)
			return;
		mGeoLocations.push_back(QPointF(pFile.mExif.mGeoLococation.mLatitude, pFile.mExif.mGeoLococation.mLongitude));
		mGeoLocationsFiles.push_back(&pFile);
	}

	virtual void reset() override
	{
		mGeoLocations.clear();
		mGeoLocationsFiles.clear();
	}
};