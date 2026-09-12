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

class ESNetClientOriginalImageDownloadRequestInternal : public QObject
{
	friend class ESNetClientOriginalImageDownloadRequest;

	Q_OBJECT

signals:
	/********************************* SIGNALS ***********************************/

	void finished();

private:
	/********************************* METHODS ***********************************/

	ESNetClientOriginalImageDownloadRequestInternal(const QString& pPath, const QString& pHost, quint16 pPort, const std::function<void(const QImage&)>& pCallback);
	virtual ~ESNetClientOriginalImageDownloadRequestInternal() override;

	void cancelRequest();
	void process();
	void readbytes();
	void handleerror(QAbstractSocket::SocketError pError);
	void finish(const QImage& pImage);

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

class ESNetClientOriginalImageDownloadRequest : public QObject
{
	Q_OBJECT

public:
	/********************************* METHODS ***********************************/

	static std::shared_ptr<ESNetClientOriginalImageDownloadRequest> downloadOriginalImage(QString pImagePath, const QString& pHost, quint16 pPort, const std::function<void(const QImage&)>& pFinishedCallback);

	ESNetClientOriginalImageDownloadRequest(const QString& pPath, const QString& pHost, quint16 pPort, const std::function<void(const QImage&)>& pCallback);
	virtual ~ESNetClientOriginalImageDownloadRequest() override;

	void cancelRequest();
	void process();

signals:
	/********************************* SIGNALS ***********************************/

	void finished();

private:
	/********************************* METHODS ***********************************/

	void onInternalRequestFinished();

	/******************************** ATTRIBUTES **********************************/

	ESNetClientOriginalImageDownloadRequestInternal* mInternalRequest = nullptr;
};
