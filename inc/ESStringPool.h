#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

#include <QString>
#include <QMutex>
#include <QReadWriteLock>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESStringPool
{
	friend class ESStringId;

public:
	/********************************** TYPES *************************************/

	typedef uint64_t InternalId;

	/********************************* METHODS ***********************************/

	ESStringPool();
	~ESStringPool();
private:
	/******************************** ATTRIBUTES **********************************/

	static ESStringPool* msInstance;
	QReadWriteLock mStringsMutex;
	std::unordered_map<QString, std::unique_ptr<QString>> mStrings;
	QString mNullString;

	/********************************* METHODS ***********************************/

	InternalId getStringId(const QString& aString);
};

/********************************************************************************/

class ESStringId
{
public:
	/********************************* METHODS ***********************************/

	ESStringId();
	ESStringId(const ESStringId& aString);
	ESStringId(const QString& aString);
	ESStringId(const std::string& aString);
	const QString& getString() const;
	ESStringPool::InternalId getId() const;
	auto operator<=>(const ESStringId& aString) const = default;
	operator QString() const;

	bool isValid() const;

private:
	/******************************** ATTRIBUTES **********************************/

	ESStringPool::InternalId mId;
};

/********************************************************************************/

template <>
struct std::hash<ESStringId>
{
	std::size_t operator()(const ESStringId& aStringId) const
	{
		return std::hash<ESStringPool::InternalId>()(aStringId.getId());
	}
};
