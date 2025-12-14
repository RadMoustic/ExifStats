#include "ESStringPool.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

/*static*/ ESStringPool* ESStringPool::msInstance = nullptr;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

ESStringPool::ESStringPool()
{
	assert(!msInstance);
	msInstance = this;
	mNullString = "";
}
/********************************************************************************/

ESStringPool::~ESStringPool()
{
}

/********************************************************************************/

ESStringPool::InternalId ESStringPool::getStringId(const QString& aString)
{
	if(aString.isEmpty())
		return 0;
	std::unique_ptr<QString>* lStringData = nullptr;

	mStringsMutex.lockForRead();
	auto itFound = mStrings.find(aString);
	if (itFound == mStrings.end())
	{
		mStringsMutex.unlock();
		mStringsMutex.lockForWrite();
		lStringData = &mStrings[aString];
		if(!lStringData->get()) // Can be written by another thread between the unlock and the lockforwrite
			lStringData->reset(new QString(aString));
		mStringsMutex.unlock();
	}
	else
	{
		lStringData = &itFound->second;
		mStringsMutex.unlock();
	}

	static_assert(sizeof(QString*) == sizeof(uint64_t));

	return reinterpret_cast<uint64_t>(lStringData->get());
}

/********************************************************************************/

StringId::StringId()
	: mId(0)
{

}

/********************************************************************************/

StringId::StringId(const StringId& aString)
	: mId(aString.mId)
{

}

/********************************************************************************/

StringId::StringId(const QString& aString)
{
	mId = ESStringPool::msInstance->getStringId(aString);
}

/********************************************************************************/

StringId::StringId(const std::string& aString)
{
	mId = ESStringPool::msInstance->getStringId(aString.c_str());
}

/********************************************************************************/

const QString& StringId::getString() const
{
	return mId == 0 ? ESStringPool::msInstance->mNullString : *reinterpret_cast<QString*>(mId);
}

/********************************************************************************/

ESStringPool::InternalId StringId::getId() const
{
	return mId;
}

/********************************************************************************/

bool StringId::isValid() const
{
	return mId != 0;
}

/********************************************************************************/

StringId::operator QString() const
{
	return getString();
}
