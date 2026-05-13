#include <ESSplitZipFileDevice.h>

// Qt
#include <QIODevice>
#include <QFile>
#include <QStringList>
#include <QVector>
#include <QUrl>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

ESSplitZipFileDevice::ESSplitZipFileDevice(const std::vector<QUrl>& pFiles, QObject* pParent)
	: QIODevice(pParent)
	, mFiles(pFiles)
	, mCurrentIndex(0)
	, mTotalSize(0)
{
}

/********************************************************************************/

ESSplitZipFileDevice::~ESSplitZipFileDevice()
{
	close();
}

/********************************************************************************/

bool ESSplitZipFileDevice::isSequential() const /*override*/
{
	return false;
}

/********************************************************************************/

qint64 ESSplitZipFileDevice::size() const /*override*/
{
	return mTotalSize;
}

/********************************************************************************/

bool ESSplitZipFileDevice::open(OpenMode pMode) /*override*/
{
	if (pMode != ReadOnly || mFiles.empty())
		return false;

	mSegments.clear();
	mTotalSize = 0;

	for (const QUrl& lUrl : mFiles)
	{
#ifdef Q_OS_ANDROID
		QString lFilePath = lUrl.toString();
#else
		QString lFilePath = lUrl.toLocalFile();
#endif
		QFile lFile(lFilePath);
		if (!lFile.open(ReadOnly))
			return false;

		qint64 lHeaderSize = 0;
		char lMagic[4];
		if (lFile.peek(lMagic, 4) == 4)
		{
			if (lMagic[0] == 'P' && lMagic[1] == 'K' && (uchar)lMagic[2] == 0x07 && (uchar)lMagic[3] == 0x08)
			{
				lHeaderSize = 4;
			}
		}

		SegmentInfo lInfo;
		lInfo.mFilePath = lFilePath;
		lInfo.mHeaderSize = lHeaderSize;
		lInfo.mLogicalStart = mTotalSize;
		mTotalSize += (lFile.size() - lHeaderSize);
		lInfo.mLogicalEnd = mTotalSize;

		mSegments.append(lInfo);
		lFile.close();
	}

	mCurrentIndex = 0;
	mCurrentFile.setFileName(mSegments[0].mFilePath);
	if (!mCurrentFile.open(ReadOnly))
		return false;
	mCurrentFile.seek(mSegments[0].mHeaderSize);

	return QIODevice::open(pMode);
}

/********************************************************************************/

void ESSplitZipFileDevice::close() /*override*/
{
	if (mCurrentFile.isOpen())
		mCurrentFile.close();
	QIODevice::close();
}

/********************************************************************************/

qint64 ESSplitZipFileDevice::readData(char* pData, qint64 pMaxSize) /*override*/
{
	if (pMaxSize <= 0)
		return 0;

	qint64 lTotalRead = 0;
	qint64 lCurrentLogicalPos = pos();

	while (lTotalRead < pMaxSize && lCurrentLogicalPos < mTotalSize)
	{
		qint64 lInSegment = mSegments[mCurrentIndex].mLogicalEnd - lCurrentLogicalPos;

		if (lInSegment <= 0)
		{
			if (mCurrentIndex < mSegments.size() - 1)
			{
				if (!switchToFile(mCurrentIndex + 1))
					return -1;
				lInSegment = mSegments[mCurrentIndex].mLogicalEnd - lCurrentLogicalPos;
			}
			else break;
		}

		qint64 lToRead = qMin(pMaxSize - lTotalRead, lInSegment);
		qint64 lRead = mCurrentFile.read(pData + lTotalRead, lToRead);

		if (lRead < 0)
			return -1;
		if (lRead == 0)
			break;

		lTotalRead += lRead;
		lCurrentLogicalPos += lRead;
	}

	return lTotalRead;
}

/********************************************************************************/

qint64 ESSplitZipFileDevice::writeData(const char*, qint64) /*override*/
{
	return -1;
}

/********************************************************************************/

bool ESSplitZipFileDevice::seek(qint64 pPos) /*override*/
{
	if (pPos < 0 || pPos > mTotalSize)
		return false;

	int lNewIndex = -1;
	for (int i = 0; i < mSegments.size(); ++i)
	{
		if (pPos >= mSegments[i].mLogicalStart && pPos < mSegments[i].mLogicalEnd)
		{
			lNewIndex = i;
			break;
		}
	}
	if (pPos == mTotalSize)
		lNewIndex = mSegments.size() - 1;

	if (lNewIndex != -1)
	{
		if (lNewIndex != mCurrentIndex)
		{
			if (!switchToFile(lNewIndex))
				return false;
		}
		qint64 lPhysicalPos = (pPos - mSegments[mCurrentIndex].mLogicalStart) + mSegments[mCurrentIndex].mHeaderSize;
		if (mCurrentFile.seek(lPhysicalPos))
		{
			return QIODevice::seek(pPos);
		}
	}
	return false;
}

/********************************************************************************/

bool ESSplitZipFileDevice::switchToFile(int pIndex)
{
	if (mCurrentFile.isOpen())
		mCurrentFile.close();
	mCurrentIndex = pIndex;
	mCurrentFile.setFileName(mSegments[mCurrentIndex].mFilePath);
	if (!mCurrentFile.open(ReadOnly))
		return false;
	return true;
}
