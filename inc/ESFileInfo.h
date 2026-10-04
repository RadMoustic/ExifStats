#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// Qt
#include <QObject>
#include <QVector2D>
#include <QGeoShape>
#include <QDateTime>
#include <QCryptographicHash>

// Stl
#include <functional>
#include <unordered_set>

// ES
#include "ESStringPool.h"
#include "ESUtils.h"
#include "exif.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/


/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

enum ESReadExifFileResult: int16_t
{
	eNone = 1,
	eSuccess = 0,
	eFileNotFound = -1,
	eCantOpenFile = -2,
	eFailedToRead = -3,
	eBufferTooSmallToReadExifSize = -4,
	eExifSizeTooSmall = -5,

	eParseExifErrorNoJpeg = 1982, // No JPEG markers found in buffer, possibly invalid JPEG file
	eParseExifErrorNoExif = 1983, // No EXIF header found in JPEG file.
	eParseExifErrorUnknownByteAlign = 1984, // Byte alignment specified in EXIF file was unknown (not Motorola or Intel).
	eParseExifErrorCorrupt = 1985, // EXIF header was found, but data was corrupted.
};

// First byte orientation
enum ESExifOrientation : unsigned short
{
	Unspecified = 0,
	UpperLeft = 1,
	LowerRight = 3,
	UpperRight = 6,
	LowerLeft = 8,
	Undefined = 9
};

constexpr uint USEFULLEXIF_VERSION = 10;
struct ESUsefullExif
{
	ESStringId mCameraModel;
	ESStringId mLensModel;
	quint64 mDateTime = 0;
	float mShutterSpeedValue = 0.f;
	float mFNumber = 0.f;
	struct GeoLocation
	{
		float mLatitude = 0.f;
		float mLongitude = 0.f;
		bool isValid() const { return mLatitude != 0.f || mLongitude != 0.f; }
	} mGeoLocation;
	quint16 mFocalLengthIn35mm = 0;
	quint16 mFocalLength = 0;
	ESExifOrientation mOrientation = Unspecified;
	unsigned short mISOSpeedRatings = 0;
	unsigned short mWidth = 0;
	unsigned short mHeight = 0;
	bool mGeoLocationGuessed = false;

	unsigned short getOrientedWidth() const
	{
		return mOrientation == ESExifOrientation::UpperRight || mOrientation == ESExifOrientation::LowerLeft ? mHeight : mWidth;
	}

	unsigned short getOrientedHeight() const
	{
		return mOrientation == ESExifOrientation::UpperRight || mOrientation == ESExifOrientation::LowerLeft ? mWidth : mHeight;
	}

	float getOrientedRatio() const
	{
		return mWidth > 0 && mHeight > 0 ? float(getOrientedWidth()) / float(getOrientedHeight()) : 1.f;
	}
};

typedef std::vector<float, ESAlignedAllocator<float, 64>> ESEmbeddings;
typedef uint32_t ESFileInfoId;

template<bool KeepData>
class ESHash : private QCryptographicHash
{
public:
	using QCryptographicHash::QCryptographicHash;
	using QCryptographicHash::result;

	void addDataCustom(const QByteArrayView& pData)
	{
		if constexpr (KeepData)
		{
			mData.append(pData);
			mData.append("_|_");
		}
		addData(pData);
	}

	const QByteArray& getData() const { return mData; }

private:
	QByteArray mData;
};

struct ESFileInfo
{
	QString mHash;

	ESFileInfoId mId;
	ESStringId mFilePath;
	ESUsefullExif mExif;
	uint8_t mCameraModelIdx = std::numeric_limits<uint8_t>::max();
	uint8_t mLensModelIdx = std::numeric_limits<uint8_t>::max();
	ESStringId mResolutionStr;
	ESReadExifFileResult mReadResult = eNone;
	std::vector<uint16_t> mTagIndexes;
	ESEmbeddings mEmbeddings;
	bool mTagsGenerated = false;

	template<bool KeepData>
	void addHashData(ESHash<KeepData>& pHash) const
	{
		pHash.addDataCustom(mExif.mCameraModel.getString().toUtf8());
		pHash.addDataCustom(mExif.mLensModel.getString().toUtf8());
		pHash.addDataCustom(QByteArrayView(reinterpret_cast<const char*>(&mExif.mDateTime), sizeof(mExif.mDateTime)));
		pHash.addDataCustom(QByteArrayView(reinterpret_cast<const char*>(&mExif.mShutterSpeedValue), sizeof(mExif.mShutterSpeedValue)));
		pHash.addDataCustom(QByteArrayView(reinterpret_cast<const char*>(&mExif.mFNumber), sizeof(mExif.mFNumber)));
		if(mExif.mGeoLocationGuessed)
		{
			float lZero = 0.f;
			pHash.addDataCustom(QByteArrayView(reinterpret_cast<const char*>(&lZero), sizeof(lZero)));
			pHash.addDataCustom(QByteArrayView(reinterpret_cast<const char*>(&lZero), sizeof(lZero)));
		}
		else
		{
			pHash.addDataCustom(QByteArrayView(reinterpret_cast<const char*>(&mExif.mGeoLocation.mLatitude), sizeof(mExif.mGeoLocation.mLatitude)));
			pHash.addDataCustom(QByteArrayView(reinterpret_cast<const char*>(&mExif.mGeoLocation.mLongitude), sizeof(mExif.mGeoLocation.mLongitude)));
		}
		pHash.addDataCustom(QByteArrayView(reinterpret_cast<const char*>(&mExif.mFocalLengthIn35mm), sizeof(mExif.mFocalLengthIn35mm)));
		pHash.addDataCustom(QByteArrayView(reinterpret_cast<const char*>(&mExif.mFocalLength), sizeof(mExif.mFocalLength)));
		pHash.addDataCustom(QByteArrayView(reinterpret_cast<const char*>(&mExif.mOrientation), sizeof(mExif.mOrientation)));
		pHash.addDataCustom(QByteArrayView(reinterpret_cast<const char*>(&mExif.mISOSpeedRatings), sizeof(mExif.mISOSpeedRatings)));
		pHash.addDataCustom(QByteArrayView(reinterpret_cast<const char*>(&mExif.mWidth), sizeof(mExif.mWidth)));
		pHash.addDataCustom(QByteArrayView(reinterpret_cast<const char*>(&mExif.mHeight), sizeof(mExif.mHeight)));
	}

	void computeHash()
	{
		mHash = "";
		if(mReadResult == eSuccess)
		{
			ESHash<false> lHash(QCryptographicHash::Sha256);

			addHashData(lHash);

			mHash = QString::fromLatin1(lHash.result());
		}
	}
};
