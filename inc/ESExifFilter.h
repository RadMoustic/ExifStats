#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESFileInfo.h"

// Qt
#include <QJsonObject>
#include <QJsonArray>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

#define VALIDATE_JSONVALUE(pJson, pKey, pDest) \
	{ \
		QJsonValue lJsonValue = pJson[pKey]; \
		if (lJsonValue.isUndefined()) \
		{ \
			qWarning("Key '%s' not found in Json", qUtf8Printable(pKey)); \
			return false; \
		} \
			pDest = lJsonValue.toVariant().template value<decltype(pDest)>(); \
	}

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ExifFilter
{
public:
	bool mKeepCategory = false;
	QString mName;

	virtual void reset() = 0;
	virtual bool isFileFilteredOut(const FileInfo& pFile) const = 0;
	virtual QJsonObject serialize() const = 0;
	virtual bool deserialize(const QJsonObject& pJson) = 0;

	template<typename T>
	QJsonValue toJsonValue(T pValue) const
	{
		return QJsonValue(pValue);
	}

	template<>
	QJsonValue toJsonValue(uint64_t pValue) const
	{
		return QJsonValue(QString::number(pValue));
	}

	template<typename T>
	QJsonArray toJsonArray(QList<T> pValues) const
	{
		QJsonArray lResult;
		for (const T& value : pValues)
		{
			lResult.append(toJsonValue(value));
		}
		return lResult;
	}

	template<typename T>
	T fromJsonValue(QJsonValue pJson) const
	{
		return pJson.toVariant().template value<T>();
	}

	template<>
	uint64_t fromJsonValue(QJsonValue pJson) const
	{
		return pJson.toString().toULongLong();
	}
};