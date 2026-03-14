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

ESStringPool::InternalId ESStringPool::getStringId(const QString& pString)
{
	if(pString.isEmpty())
		return 0;
	std::unique_ptr<QString>* lStringData = nullptr;

	mStringsMutex.lockForRead();
	auto itFound = mStrings.find(pString);
	if (itFound == mStrings.end())
	{
		mStringsMutex.unlock();
		mStringsMutex.lockForWrite();
		lStringData = &mStrings[pString];
		if(!lStringData->get()) // Can be written by another thread between the unlock and the lockforwrite
			lStringData->reset(new QString(pString));
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

ESStringId::ESStringId()
	: mId(0)
{

}

/********************************************************************************/

ESStringId::ESStringId(ESStringPool::InternalId pId)
	: mId(pId)
{
}

/********************************************************************************/

ESStringId::ESStringId(const ESStringId& pString)
	: mId(pString.mId)
{

}

/********************************************************************************/

ESStringId::ESStringId(const QString& pString)
{
	mId = ESStringPool::msInstance->getStringId(pString);
}

/********************************************************************************/

ESStringId::ESStringId(const std::string& pString)
{
	mId = ESStringPool::msInstance->getStringId(pString.c_str());
}

/********************************************************************************/

const QString& ESStringId::getString() const
{
	return mId == 0 ? ESStringPool::msInstance->mNullString : *reinterpret_cast<QString*>(mId);
}

/********************************************************************************/

ESStringPool::InternalId ESStringId::getId() const
{
	return mId;
}

/********************************************************************************/

bool ESStringId::isValid() const
{
	return mId != 0;
}

/********************************************************************************/

ESStringId::operator QString() const
{
	return getString();
}
