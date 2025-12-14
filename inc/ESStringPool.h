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
	friend class StringId;

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

class StringId
{
public:
	/********************************* METHODS ***********************************/

	StringId();
	StringId(const StringId& aString);
	StringId(const QString& aString);
	StringId(const std::string& aString);
	const QString& getString() const;
	ESStringPool::InternalId getId() const;
	auto operator<=>(const StringId& aString) const = default;
	operator QString() const;

	bool isValid() const;

private:
	/******************************** ATTRIBUTES **********************************/

	ESStringPool::InternalId mId;
};

/********************************************************************************/

template <>
struct std::hash<StringId>
{
	std::size_t operator()(const StringId& aStringId) const
	{
		return std::hash<ESStringPool::InternalId>()(aStringId.getId());
	}
};
