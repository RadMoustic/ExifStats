#pragma once

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

private:
	/********************************* METHODS ***********************************/

	ESNetClientOriginalImageDownloadRequestInternal(const QString& pImageHash, const QString& pHost, quint16 pPort);
	virtual ~ESNetClientOriginalImageDownloadRequestInternal() override;

	void cancelRequest();
	void process();
	void readbytes();
	void handleerror(QAbstractSocket::SocketError pError);
	void finish(const QImage& pImage);

	/******************************** ATTRIBUTES **********************************/

	QTcpSocket* mSocket = nullptr;
	QString mImageHash;
	QString mHost;
	quint16 mPort;
	bool mAwaitingchallenge = true;
	quint32 mExpectedimagesize = 0;
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

	/********************************* METHODS ***********************************/

	static std::shared_ptr<ESNetClientOriginalImageDownloadRequest> downloadOriginalImage(const std::shared_ptr<ESImage>& pImage, const QString& pHost, quint16 pPort);

	ESNetClientOriginalImageDownloadRequest(const std::shared_ptr<ESImage>& pImage, const QString& pHost, quint16 pPort);
	virtual ~ESNetClientOriginalImageDownloadRequest() override;

	void cancelRequest();
	void process();

	const bool isCancelled() const;
	const bool isFinished() const;
	const QImage& getDownloadedImage() const;

	std::shared_ptr<ESImage> getImage() const;

	static void garbageCollectRequests();

signals:
	/********************************* SIGNALS ***********************************/

	void finished(const ESNetClientOriginalImageDownloadRequest& pRequest);

private:
	/********************************* METHODS ***********************************/

	void onInternalRequestFinished(const QImage& pImage);

	/******************************** ATTRIBUTES **********************************/

	std::weak_ptr<ESImage> mParentImage;
	ESNetClientOriginalImageDownloadRequestInternal* mInternalRequest = nullptr;
	QImage mDownloadedImage;
	std::atomic_bool mIsFinished = false;

	static std::mutex msActiveRequestsMutex;
	static std::vector<std::weak_ptr<ESNetClientOriginalImageDownloadRequest>> msActiveRequests;
};
