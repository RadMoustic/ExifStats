#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

template<bool READ>
class ESSerializer
{
public:
	const bool mIsReading = READ;

	ESSerializer(QIODevice* pDevice)
		: mDataStream(pDevice)
	{

	}
	
	template<typename T>
	void Serialize(T& pValue)
	{
		if constexpr (READ)
		{
			mDataStream >> pValue;
		}
		else
		{
			mDataStream << pValue;
		}
	}

	template<>
	void Serialize(StringId& pValue)
	{
		if constexpr (READ)
		{
			QString lValue;
			mDataStream >> lValue;
			pValue = lValue;
		}
		else
		{
			mDataStream << pValue.getString();
		}
	}

	template<typename T>
	void Serialize(QVector<T>& pList)
	{
		if constexpr (READ)
		{
			qsizetype lNbItem = 0;
			mDataStream >> lNbItem;
			pList.clear();
			pList.resize(lNbItem);
			for (T& lItem : pList)
				mDataStream >> lItem;
		}
		else
		{
			mDataStream << pList.size();
			for (const T& lItem : pList)
				mDataStream << lItem;
		}
	}

	template<typename KEY, typename VALUE>
	void Serialize(std::map<KEY, VALUE>& pMap)
	{
		if constexpr (READ)
		{
			size_t lNbPair = 0;
			mDataStream >> lNbPair;
			for (size_t i = 0; i < lNbPair; ++i)
			{
				KEY lKey;
				mDataStream >> lKey;
				VALUE lValue;
				mDataStream >> lValue;
				pMap[lKey] = lValue;
			}
		}
		else
		{
			mDataStream << pMap.size();
			for (const auto& lPair : pMap)
			{
				mDataStream << lPair.first;
				mDataStream << lPair.second;
			}
		}
	}

	template <typename T>
	struct identity
	{
		typedef T type;
	};

	template<typename KEY, typename VALUE>
	void SerializeCustom(std::map<KEY,VALUE>& pMap, typename identity<std::function<void(KEY&, VALUE&)>>::type pSerializeFct)
	{
		if constexpr (READ)
		{
			size_t lNbPair = 0;
			mDataStream >> lNbPair;
			for (size_t i = 0; i < lNbPair; ++i)
			{
				KEY lKey;
				VALUE lValue;
				pSerializeFct(lKey, lValue);
				pMap[lKey] = std::move(lValue);
			}
		}
		else
		{
			mDataStream << pMap.size();
			for (auto& lPair : pMap)
			{
				KEY lKey = lPair.first;
				pSerializeFct(lKey, lPair.second);
			}
		}
	}

	template<typename T, class COMP>
	bool SerializeCheck(const T& pValue, COMP pComparer)
	{
		T lReadValue;
		return SerializeCheck(pValue, pComparer, lReadValue);
	}

	template<typename T, class COMP>
	bool SerializeCheck(const T& pValue, COMP pComparer, T& pReadValue)
	{
		if constexpr (READ)
		{
			T lValue;
			mDataStream >> lValue;
			return pComparer(lValue, pValue);
		}
		else
		{
			Q_UNUSED(pComparer);
			Q_UNUSED(pReadValue);
			mDataStream << pValue;
			return true;
		}
	}

	const QDataStream& getDataStream() const
	{
		return mDataStream;
	}

private:
	QDataStream mDataStream;
};
