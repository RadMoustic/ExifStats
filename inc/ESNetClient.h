#pragma once

#include <QObject>
#include <QImage>
#include <QAbstractSocket>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class QTcpSocket;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESNetClientOriginalImageDownloadRequest : public QObject
{
	Q_OBJECT

public:
	/********************************* METHODS ***********************************/

	ESNetClientOriginalImageDownloadRequest(const QString& pPath, const QString& pHost, quint16 pPort, const std::function<void(const QImage&)>& pCallback);
	virtual ~ESNetClientOriginalImageDownloadRequest() override;

	void cancelRequest();
	void process();

signals:
	/********************************* SIGNALS ***********************************/

	void finished(const QImage& pImage);
	void finishedthread();

private:
	/********************************* METHODS ***********************************/

	void readbytes();
	void handleerror(QAbstractSocket::SocketError pError);
	void finished(QImage& pImage);

	/******************************** ATTRIBUTES **********************************/

	QTcpSocket* mSocket = nullptr;
	QString mPath;
	QString mHost;
	quint16 mPort;
	bool mAwaitingchallenge = true;
	quint32 mExpectedimagesize = 0;
	std::function<void(const QImage&)> mFinishedCallback;
	int mRetryCount;
	bool mIsCancelled;
};

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESNetClient
{
public:
	/********************************* METHODS ***********************************/

	static std::shared_ptr<ESNetClientOriginalImageDownloadRequest> downloadOriginalImage(QString pImagePath, const QString& pHost, quint16 pPort, const std::function<void(const QImage&)>& pFinishedCallback);
};
