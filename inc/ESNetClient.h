#pragma once

// ExifStats
#include "ESNetClientHandler.h"

// Qt
#include <QObject>
#include <QImage>
#include <QAbstractSocket>

// Stl
#include <mutex>
#include <vector>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class QTcpSocket;
class ESImage;
class ESNetClientOriginalImageDownloadRequest;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESNetClientOriginalImageDownloadRequestInternal : public QObject
{
	friend class ESNetClientOriginalImageDownloadRequest;

	Q_OBJECT

signals:
	/********************************* SIGNALS ***********************************/

	void finished(const QImage& pImage);
	void lastMessageChanged(ESNetClientHandler::Message pLastMessage);
	void downloadProgress(float pProgress);

private:
	/********************************* METHODS ***********************************/

	ESNetClientOriginalImageDownloadRequestInternal(const QString& pImageHash, const QString& pImageFileName);
	virtual ~ESNetClientOriginalImageDownloadRequestInternal() override;

	void cancelRequest();
	void process();
	void readbytes();
	void handleerror(QAbstractSocket::SocketError pError);
	void finish(const QImage& pImage);

	/******************************** ATTRIBUTES **********************************/

	ESNetClientHandler::Message mLastMessage;
	QTcpSocket* mSocket;
	QString mImageHash;
	QString mImageFileName;
	bool mAwaitingchallenge;
	quint32 mExpectedimagesize;
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
	quint64 mLastUsedImage;
	static QString msServerAddress;
	static quint16 msServerPort;

	/********************************* METHODS ***********************************/

	static void initialize();
	static void setServerAddressAndPort(QString pAddress, quint16 pPort);
	static void setPassword(const QString& pPassword);
	static QString getSaltedPassword();
	static std::shared_ptr<ESNetClientOriginalImageDownloadRequest> downloadOriginalImage(const std::shared_ptr<ESImage>& pImage);

	ESNetClientOriginalImageDownloadRequest(const std::shared_ptr<ESImage>& pImage);
	virtual ~ESNetClientOriginalImageDownloadRequest() override;

	void cancelRequest();
	void process();

	const ESNetClientHandler::Message& getLastMessage() const;
	const bool hasFailed() const;
	const bool isFinished() const;
	const QImage& getDownloadedImage() const;

	std::shared_ptr<ESImage> getImage() const;

	static void garbageCollectRequests();

signals:
	/********************************* SIGNALS ***********************************/

	void lastMessageChanged(ESNetClientHandler::Message pLastMessage);
	void finished(const ESNetClientOriginalImageDownloadRequest& pRequest);
	void downloadProgress(float pProgress);

private:
	/********************************* METHODS ***********************************/

	void onInternalRequestFinished(const QImage& pImage);
	void onInternalRequestLastMessageChanged(ESNetClientHandler::Message pLastMessage);

	/******************************** ATTRIBUTES **********************************/

	std::weak_ptr<ESImage> mParentImage;
	ESNetClientOriginalImageDownloadRequestInternal* mInternalRequest;
	QImage mDownloadedImage;
	std::atomic_bool mIsFinished;
	ESNetClientHandler::Message mLastMessage;

	static QString msServerSaltedPassword;
	static std::mutex msActiveRequestsMutex;
	static std::vector<std::weak_ptr<ESNetClientOriginalImageDownloadRequest>> msActiveRequests;
};
