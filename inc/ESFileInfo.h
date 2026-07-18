#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// Qt
#include <QObject>
#include <QVector2D>
#include <QGeoShape>
#include <QDateTime>

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

constexpr uint USEFULLEXIF_VERSION = 6;
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

struct ESFileInfo
{
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
};
