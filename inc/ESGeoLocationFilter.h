#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// Qt
#include <QGeoShape>
#include <QGeoRectangle>

// ES
#include "ESFilter.h"
#include "ESFileInfo.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/


/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESGeoLocationFilter : public ESFilter
{
public:
	QGeoShape mGeoShapeFilter;
	bool mShowGuessedLocation = false;

	virtual void reset() override
	{
		mGeoShapeFilter = QGeoShape();
		mShowGuessedLocation = false;
	}

	virtual bool isEnabled() const override
	{
		return mGeoShapeFilter.isValid();
	}

	virtual bool isFileFilteredOut(const ESFileInfo& pFile) const override
	{
		return		mGeoShapeFilter.isValid()
				&&	(	(	pFile.mExif.mGeoLocation.mLatitude == 0
						&&	pFile.mExif.mGeoLocation.mLongitude == 0)
					||	(	!mShowGuessedLocation
						&& pFile.mExif.mGeoLocationGuessed)
					||	!mGeoShapeFilter.contains(QGeoCoordinate(pFile.mExif.mGeoLocation.mLatitude, pFile.mExif.mGeoLocation.mLongitude)));
	}

	QJsonObject serializeGeoCoordinates(const QGeoCoordinate& pGeoCoordinate) const
	{
		QJsonObject lResult;
		lResult["Latitude"] = pGeoCoordinate.latitude();
		lResult["Longitude"] = pGeoCoordinate.longitude();
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