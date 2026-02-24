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
#include "exif.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/


/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

enum ReadExifFileResult: int16_t
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

struct UsefullExif
{
	StringId mCameraModel;
	StringId mLensModel;		
	uint64_t mDateTime;
	float mShutterSpeedValue;
	float mFNumber;
	struct
	{
		float mLatitude;
		float mLongitude;
	} mGeoLococation;
	unsigned short mFocalLengthIn35mm;
	unsigned short mFocalLength;
	ESExifOrientation mOrientation;
};

struct FileInfo
{
	StringId mFilePath;
	UsefullExif mExif;
	uint8_t mCameraModelIdx = std::numeric_limits<uint8_t>::max();
	uint8_t mLensModelIdx = std::numeric_limits<uint8_t>::max();
	ReadExifFileResult mReadResult = eNone;
	QVector<uint16_t> mTagIndexes;
	bool mTagsGenerated = false;
};