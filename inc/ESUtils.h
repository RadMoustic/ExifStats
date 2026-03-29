#pragma once

// Qt
#include <QString>

// Stl
#include <limits>
#include <new>

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

#if defined(_MSC_VER)
	#define ES_BREAKPOINT() __debugbreak();
#elif defined(Q_OS_ANDROID)
	#define ES_BREAKPOINT() raise(SIGTRAP);
#else
	#define ES_BREAKPOINT()
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

/********************************************************************************/

/**
 * Returns aligned pointers when allocations are requested. Default alignment
 * is 64B = 512b, sufficient for AVX-512 and most cache line sizes.
 *
 * @tparam ALIGNMENT_IN_BYTES Must be a positive power of 2.
 */
template<typename    ElementType,
	std::size_t ALIGNMENT_IN_BYTES = 64>
class ESAlignedAllocator
{
private:
	static_assert(
		ALIGNMENT_IN_BYTES >= alignof(ElementType),
		"Beware that types like int have minimum alignment requirements "
		"or access will result in crashes."
		);

public:
	using value_type = ElementType;
	static std::align_val_t constexpr ALIGNMENT{ ALIGNMENT_IN_BYTES };

	/**
	 * This is only necessary because AlignedAllocator has a second template
	 * argument for the alignment that will make the default
	 * std::allocator_traits implementation fail during compilation.
	 * @see https://stackoverflow.com/a/48062758/2191065
	 */
	template<class OtherElementType>
	struct rebind
	{
		using other = ESAlignedAllocator<OtherElementType, ALIGNMENT_IN_BYTES>;
	};

public:
	constexpr ESAlignedAllocator() noexcept = default;

	constexpr ESAlignedAllocator(const ESAlignedAllocator&) noexcept = default;

	template<typename U>
	constexpr ESAlignedAllocator(ESAlignedAllocator<U, ALIGNMENT_IN_BYTES> const&) noexcept
	{
	}

	[[nodiscard]] ElementType*
		allocate(std::size_t pElementsToAllocate)
	{
		if (pElementsToAllocate
			 > std::numeric_limits<std::size_t>::max() / sizeof(ElementType)) {
			throw std::bad_array_new_length();
		}

		const std::size_t lBytesToAllocate = pElementsToAllocate * sizeof(ElementType);
		return reinterpret_cast<ElementType*>(
			::operator new[](lBytesToAllocate, ALIGNMENT));
	}

	void
		deallocate(ElementType* pAllocatedPointer,
			[[maybe_unused]] std::size_t  pBytesAllocated)
	{
		/* According to the C++20 draft n4868 § 17.6.3.3, the delete operator
		 * must be called with the same alignment argument as the new expression.
		 * The size argument can be omitted but if present must also be equal to
		 * the one used in new. */
		::operator delete[](pAllocatedPointer, ALIGNMENT);
	}

	struct propagate_on_container_move_assignment : std::true_type {};
};
