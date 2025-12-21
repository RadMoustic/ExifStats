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

class ESExifPathFilter : public ExifFilter
{
public:
	QStringList mPathInclusiveFilters;

	virtual void reset() override
	{
		mPathInclusiveFilters.clear();
	}

	virtual bool isFileFilteredOut(const FileInfo& pFile) const override
	{
		if(mPathInclusiveFilters.isEmpty())
			return false;
		for(const QString& lPathPart : mPathInclusiveFilters)
		{
			if(pFile.mFilePath.getString().contains(lPathPart, Qt::CaseInsensitive))
				return false;
		}
		return true;
	}

	virtual QJsonObject serialize() const override
	{
		QJsonObject lResult;
		lResult["PathInclusiveFilters"] = QJsonArray::fromStringList(mPathInclusiveFilters);
		return lResult;
	}

	virtual bool deserialize(const QJsonObject& pJson) override
	{
		VALIDATE_JSONVALUE(pJson, "PathInclusiveFilters", mPathInclusiveFilters);

		return true;
	}
};