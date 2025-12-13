#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESExifStatComponent.h"


/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

struct FileInfo;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ExifStatGeoLocationComponent : public ExifStatComponent
{
public:
	QVector<QPointF> mGeoLocations;
	QVector<const FileInfo*> mGeoLocationsFiles;

	virtual void addFile(const FileInfo& pFile) override
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