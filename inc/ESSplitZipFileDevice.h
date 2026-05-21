#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

#include <QIODevice>
#include <QFile>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESSplitZipFileDevice : public QIODevice
{
public:
	/********************************* METHODS **********************************/

	ESSplitZipFileDevice(const std::vector<QString> &pFiles, QObject *pParent = nullptr);
	~ESSplitZipFileDevice();

	bool isSequential() const override;
	qint64 size() const override;
	bool open(OpenMode pMode) override;
	void close() override;

protected:
	/********************************* METHODS ***********************************/

	qint64 readData(char *pData, qint64 pMaxSize) override;
	qint64 writeData(const char*, qint64) override;
	bool seek(qint64 pPos) override;

private:
	/********************************** TYPES ************************************/

	struct SegmentInfo
	{
		QString mFilePath;
		qint64 mLogicalStart;
		qint64 mLogicalEnd;
		qint64 mHeaderSize;
	};

	/******************************** ATTRIBUTES **********************************/

	std::vector<QString> mFiles;
	QVector<SegmentInfo> mSegments;
	QFile mCurrentFile;
	int mCurrentIndex;
	qint64 mTotalSize;

	/********************************* METHODS ***********************************/

	bool switchToFile(int pIndex);
};