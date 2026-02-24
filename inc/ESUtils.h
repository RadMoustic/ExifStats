#pragma once

// Qt
#include <QString>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

#define CONCAT(s) s
#define _TOSTR(s) #s
#define TOSTR(s) _TOSTR(s)

/********************************************************************************/

// Multi param macros
#define MACRO_CONCAT_(a, b) a##b
#define MACRO_CONCAT(a, b) MACRO_CONCAT_(a, b)
#define MACRO_EMPTY()

#define MACRO_CONCAT_2 MACRO_CONCAT
#define MACRO_CONCAT_3(a, b, c) MACRO_CONCAT(a, MACRO_CONCAT(b, c))
#define MACRO_CONCAT_4(a, b, c, d) MACRO_CONCAT(a, MACRO_CONCAT_3(b, c, d))

#define VARGS_(_10, _9, _8, _7, _6, _5, _4, _3, _2, _1, N, ...) N
#ifdef _MSC_VER
#define VARGS(...) MACRO_CONCAT(VARGS_(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0),)
#define MULTI_PARAM_MACRO_(pPrefix, ...) MACRO_CONCAT(pPrefix, VARGS(__VA_ARGS__))
#define MULTI_PARAM_MACRO(pPrefix, ...) MACRO_CONCAT(MULTI_PARAM_MACRO_(pPrefix, __VA_ARGS__)(__VA_ARGS__), MACRO_EMPTY())
#else
#define VARGS(...) VARGS_(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define MULTI_PARAM_MACRO(pPrefix, ...) MACRO_CONCAT(pPrefix, VARGS(__VA_ARGS__))(__VA_ARGS__)
#endif

/********************************************************************************/

#define ES_QML_PROPERTY_IMPL(pName, pType, pWriteCode, pCustomCode) \
	Q_PROPERTY(pType m##pName READ get##pName pWriteCode NOTIFY property##pName##Changed) \
	Q_SIGNAL void property##pName##Changed(); \
	pType get##pName() const \
	{ \
		return m##pName; \
	} \
	void set##pName(pType p##pName) \
	{ \
		m##pName = p##pName; \
		pCustomCode; \
		emit property##pName##Changed(); \
	} \
	private: \
		pType m##pName; \
	public:

/********************************************************************************/

#define ES_QML_PROPERTY_3(pName, pType, pCustomCode) \
	ES_QML_PROPERTY_IMPL(pName, pType, WRITE set##pName, pCustomCode)

#define ES_QML_PROPERTY_2(pName, pType) \
	ES_QML_PROPERTY_IMPL(pName, pType, WRITE set##pName, )

#define ES_QML_PROPERTY(...) MULTI_PARAM_MACRO(ES_QML_PROPERTY_, __VA_ARGS__)

/********************************************************************************/

#define ES_QML_READ_PROPERTY_3(pName, pType, pCustomCode) \
	ES_QML_PROPERTY_IMPL(pName, pType, , pCustomCode)

#define ES_QML_READ_PROPERTY_2(pName, pType) \
	ES_QML_PROPERTY_IMPL(pName, pType, , )

#define ES_QML_READ_PROPERTY(...) MULTI_PARAM_MACRO(ES_QML_READ_PROPERTY_, __VA_ARGS__)

/********************************************************************************/

int CeilIntDiv(int x, int y);

/********************************************************************************/

int constexpr constExprStringLength(const char* pStr)
{
	return *pStr ? 1 + constExprStringLength(pStr + 1) : 0;
}

/********************************************************************************/

bool getFilePathFromBase(const QString& pFilePath, const QString& pBaseFilePath, QString& pResult);