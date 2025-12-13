#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// Qt
#include <QGeoShape>
#include <QGeoRectangle>

// ES
#include "ESExifFilter.h"
#include "ESFileInfo.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/


/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ExifGeoLocationFilter : public ExifFilter
{
public:
	QGeoShape mGeoShapeFilter;

	virtual void reset() override
	{
		mGeoShapeFilter = QGeoShape();
	}

	virtual bool isFileFilteredOut(const FileInfo& pFile) const override
	{
		return		mGeoShapeFilter.isValid()
				&&	(	(	pFile.mExif.mGeoLococation.mLatitude == 0
						&&	pFile.mExif.mGeoLococation.mLongitude == 0)
					||	!mGeoShapeFilter.contains(QGeoCoordinate(pFile.mExif.mGeoLococation.mLatitude, pFile.mExif.mGeoLococation.mLongitude)));
	}

	QJsonObject serializeGeoCoordinates(const QGeoCoordinate& aGeoCoordinate) const
	{
		QJsonObject lResult;
		lResult["Latitude"] = aGeoCoordinate.latitude();
		lResult["Longitude"] = aGeoCoordinate.longitude();
		return lResult;
	}

	bool deserializeGeoCoordinates(const QJsonObject& pJson, QGeoCoordinate& pDest) const
	{
		double lLatitude;
		double lLongitude;
		VALIDATE_JSONVALUE(pJson, "Latitude", lLatitude);
		VALIDATE_JSONVALUE(pJson, "Longitude", lLongitude);
		pDest = QGeoCoordinate(lLatitude, lLongitude);

		return true;
	}

	virtual QJsonObject serialize() const override
	{
		QJsonObject lResult;
		/*
		
		 Doesn't make any sense now that we can only restrict to the view 
		
		if(mGeoShapeFilter.isValid())
		{
			if(mGeoShapeFilter.type() == QGeoShape::RectangleType)
			{
				QGeoRectangle lGeoRectFilter(mGeoShapeFilter);
				lResult["Type"] = "Rectangle";
				lResult["TopLeft"] = serializeGeoCoordinates(lGeoRectFilter.topLeft());
				lResult["BottomRight"] = serializeGeoCoordinates(lGeoRectFilter.bottomRight());
			}
			else
			{
				assert(false); // Not implemented
			}
		}
		*/
		return lResult;
	}

	virtual bool deserialize(const QJsonObject& /*pJson*/) override
	{
		mGeoShapeFilter = QGeoShape();
		/*
		if(!pJson.contains("Type"))
			return true;

		if (pJson["Type"] == "Rectangle")
		{
			QGeoCoordinate topLeft;
			if(!deserializeGeoCoordinates(pJson["TopLeft"].toObject(), topLeft))
				return false;

			QGeoCoordinate bottomRight;
			if (!deserializeGeoCoordinates(pJson["BottomRight"].toObject(), bottomRight))
				return false;

			mGeoShapeFilter = QGeoRectangle(topLeft, bottomRight);

			return true;
		}
		else
		{
			assert(false); // Not implemented
			return false;
		}
		*/

		return true;
	}
};