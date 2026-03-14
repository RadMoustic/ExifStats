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

	InternalId getStringId(const QString& pString);
};

/********************************************************************************/

class ESStringId
{
public:
	/********************************* METHODS ***********************************/

	ESStringId();
	ESStringId(ESStringPool::InternalId pId);
	ESStringId(const ESStringId& pString);
	ESStringId(const QString& pString);
	ESStringId(const std::string& pString);
	const QString& getString() const;
	ESStringPool::InternalId getId() const;
	auto operator<=>(const ESStringId& pString) const = default;
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
	std::size_t operator()(const ESStringId& pStringId) const
	{
		return std::hash<ESStringPool::InternalId>()(pStringId.getId());
	}
};
