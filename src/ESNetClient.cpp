#include "ESNetClient.h"

// ExifStats
#include "ESImage.h"

// Qt
#include <QTcpSocket>
#include <QMessageAuthenticationCode>
#include <QImage>
#include <QDataStream>
#include <QCryptographicHash>
#include <QTimer>
#include <QThread>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

const QByteArray cSharedSecret = "MyUniqueSecureCode123";
const int cMaxRetries = 3;
const int cRetryDelayMs = 500;
const int cMaxRequestAlive = 10;

/*static*/ std::mutex ESNetClientOriginalImageDownloadRequest::msActiveRequestsMutex;
/*static*/ std::vector<std::weak_ptr<ESNetClientOriginalImageDownloadRequest>> ESNetClientOriginalImageDownloadRequest::msActiveRequests;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

std::shared_ptr<ESNetClientOriginalImageDownloadRequest> ESNetClientOriginalImageDownloadRequest::downloadOriginalImage(const std::shared_ptr<ESImage>& pImage, const QString& pHost, quint16 pPort)
{
	QThread* lThread = new QThread();
	std::shared_ptr<ESNetClientOriginalImageDownloadRequest> lRequest = std::make_shared<ESNetClientOriginalImageDownloadRequest>(pImage, pHost, pPort);
	lRequest->moveToThread(lThread);
	lRequest->mInternalRequest->moveToThread(lThread);

	QObject::connect(lThread, &QThread::started, lRequest.get(), &ESNetClientOriginalImageDownloadRequest::process);
	QObject::connect(lRequest.get(), &ESNetClientOriginalImageDownloadRequest::finished, lThread, &QThread::quit);
	QObject::connect(lThread, &QThread::finished, lThread, &QObject::deleteLater);

	lThread->start();

	std::lock_guard<std::mutex> lLock(msActiveRequestsMutex);
	msActiveRequests.push_back(lRequest);

	garbageCollectRequests();

	return lRequest;
}

/********************************************************************************/

ESNetClientOriginalImageDownloadRequest::ESNetClientOriginalImageDownloadRequest(const std::shared_ptr<ESImage>& pImage, const QString& pHost, quint16 pPort)
	: mParentImage(pImage)
	, mIsFinished(false)
	, mInternalRequest(new ESNetClientOriginalImageDownloadRequestInternal(pImage->getImageHash(), pHost, pPort))
	, mLastUsedImage(QDateTime::currentMSecsSinceEpoch())
{
	QObject::connect(mInternalRequest, &ESNetClientOriginalImageDownloadRequestInternal::finished, this, &ESNetClientOriginalImageDownloadRequest::onInternalRequestFinished);
}

/********************************************************************************/

/*virtual*/ ESNetClientOriginalImageDownloadRequest::~ESNetClientOriginalImageDownloadRequest() /*override*/
{
	QObject::disconnect(mInternalRequest, &ESNetClientOriginalImageDownloadRequestInternal::finished, this, &ESNetClientOriginalImageDownloadRequest::onInternalRequestFinished);
	cancelRequest();
}

/********************************************************************************/

void ESNetClientOriginalImageDownloadRequest::onInternalRequestFinished(const QImage& pImage)
{
	mDownloadedImage = pImage;
	mInternalRequest = nullptr;
	mIsFinished = true;

	if(!mDownloadedImage.isNull())
	{
		if(std::shared_ptr<ESImage> lImage = mParentImage.lock())
		{
			if (lImage->getExif().mOrientation != ESExifOrientation::Unspecified && lImage->getExif().mOrientation != ESExifOrientation::UpperLeft)
			{
				QTransform lTransform;
				switch (lImage->getExif().mOrientation)
				{
				case ESExifOrientation::UpperRight:
					lTransform.rotate(90);
					break;
				case ESExifOrientation::LowerRight:
					lTransform.rotate(180);
					break;
				case ESExifOrientation::LowerLeft:
					lTransform.rotate(270);
					break;
				default:
					break;
				}
				mDownloadedImage = mDownloadedImage.transformed(lTransform, Qt::SmoothTransformation);
			}
		}
	}
	emit finished(*this);
}

/********************************************************************************/

void ESNetClientOriginalImageDownloadRequest::cancelRequest()
{
	if(mInternalRequest)
	{
		mInternalRequest->cancelRequest();
		mInternalRequest = nullptr;
	}
}

/********************************************************************************/

void ESNetClientOriginalImageDownloadRequest::process()
{
	mInternalRequest->process();
}

/********************************************************************************/

const bool ESNetClientOriginalImageDownloadRequest::isCancelled() const
{
	return mInternalRequest == nullptr && !mIsFinished;
}

/********************************************************************************/

const bool ESNetClientOriginalImageDownloadRequest::isFinished() const
{
	return mIsFinished;
}

/********************************************************************************/

const QImage& ESNetClientOriginalImageDownloadRequest::getDownloadedImage() const
{
	return mDownloadedImage;
}

/********************************************************************************/

std::shared_ptr<ESImage> ESNetClientOriginalImageDownloadRequest::getImage() const
{
	return mParentImage.lock();
}

/********************************************************************************/

ESNetClientOriginalImageDownloadRequestInternal::ESNetClientOriginalImageDownloadRequestInternal(const QString& pImageHash, const QString& pHost, quint16 pPort)
	: mImageHash(pImageHash)
	, mHost(pHost)
	, mPort(pPort)
	, mRetryCount(0)
	, mIsCancelled(false)
{

}

/********************************************************************************/

/*virtual*/ ESNetClientOriginalImageDownloadRequestInternal::~ESNetClientOriginalImageDownloadRequestInternal() /*override*/
{

}

/********************************************************************************/

void ESNetClientOriginalImageDownloadRequestInternal::cancelRequest()
{
	if (mIsCancelled)
		return;
	mIsCancelled = true;

	QMetaObject::invokeMethod(this, [this]()
		{
			if (mSocket)
				mSocket->abort();
			finish(QImage());
		}, Qt::QueuedConnection);
}

/********************************************************************************/

void ESNetClientOriginalImageDownloadRequestInternal::process()
{
	mSocket = new QTcpSocket();
	connect(mSocket, &QTcpSocket::readyRead, this, &ESNetClientOriginalImageDownloadRequestInternal::readbytes);
	connect(mSocket, &QTcpSocket::errorOccurred, this, &ESNetClientOriginalImageDownloadRequestInternal::handleerror);

	mAwaitingchallenge = true;
	mSocket->connectToHost(mHost, mPort);
}

/********************************************************************************/

void ESNetClientOriginalImageDownloadRequestInternal::readbytes()
{
	if (mIsCancelled)
		return;

	if (mAwaitingchallenge)
	{
		if (mSocket->bytesAvailable() < 32)
		{
			return;
		}

		QByteArray lChallenge = mSocket->read(32);
		QMessageAuthenticationCode lMac(QCryptographicHash::Sha256);
		lMac.setKey(cSharedSecret);
		lMac.addData(lChallenge);

		mSocket->write(lMac.result());

		QDataStream lStream(mSocket);
		lStream.setVersion(QDataStream::Qt_6_0);
		uint lProtocolVersion = 1; // For futur improvements and retro compatibility
		uint lRequestType = 1; // For futur improvements and retro compatibility
		lStream << lProtocolVersion;
		lStream << lRequestType;
		lStream << mImageHash;

		mAwaitingchallenge = false;
	}
	else
	{
		if (mExpectedimagesize == 0)
		{
			if (mSocket->bytesAvailable() < static_cast<qint64>(sizeof(quint32)))
			{
				return;
			}
			QDataStream lStream(mSocket);
			lStream.setVersion(QDataStream::Qt_6_0);
			lStream >> mExpectedimagesize;

			if (mExpectedimagesize == 0)
			{
				finish(QImage());
				return;
			}
		}

		if (mSocket->bytesAvailable() >= static_cast<qint64>(mExpectedimagesize))
		{
			QByteArray lData = mSocket->read(mExpectedimagesize);
			QImage lImage;
			lImage.loadFromData(lData, "JPG");

			finish(lImage);
			return;
		}
	}
}

/********************************************************************************/

void ESNetClientOriginalImageDownloadRequestInternal::finish(const QImage& pImage)
{
	if(mSocket)
	{
		disconnect(mSocket, &QTcpSocket::readyRead, this, &ESNetClientOriginalImageDownloadRequestInternal::readbytes);
		disconnect(mSocket, &QTcpSocket::errorOccurred, this, &ESNetClientOriginalImageDownloadRequestInternal::handleerror);
		mSocket->disconnectFromHost();
		mSocket->deleteLater();
		mSocket = nullptr;
	}
	emit finished(pImage);
	deleteLater();
}

/********************************************************************************/

void ESNetClientOriginalImageDownloadRequestInternal::handleerror(QAbstractSocket::SocketError /*pError*/)
{
	if (mIsCancelled)
		return;

	mSocket->abort();
	mSocket->deleteLater();
	mSocket = nullptr;

	if (mRetryCount < cMaxRetries)
	{
		mRetryCount++;
		QTimer::singleShot(cRetryDelayMs, this, &ESNetClientOriginalImageDownloadRequestInternal::process);
	}
	else
	{
		finish(QImage());
	}
}

/********************************************************************************/

/*static*/ void ESNetClientOriginalImageDownloadRequest::garbageCollectRequests()
{
	// Singleshot to avoid mutex
	QTimer::singleShot(0, []()
		{
			std::lock_guard<std::mutex> lLock(msActiveRequestsMutex);
			if (msActiveRequests.size() > cMaxRequestAlive)
			{
				std::sort(msActiveRequests.begin(), msActiveRequests.end(), [](const std::weak_ptr<ESNetClientOriginalImageDownloadRequest>& a, const std::weak_ptr<ESNetClientOriginalImageDownloadRequest>& b)
					{
						auto aLock = a.lock();
						auto bLock = b.lock();
						if (!aLock) return false;
						if (!bLock) return true;
						return aLock->mLastUsedImage > bLock->mLastUsedImage;
					});
			
				for(int i = cMaxRequestAlive; i < static_cast<int>(msActiveRequests.size()); ++i)
				{
					if (std::shared_ptr<ESNetClientOriginalImageDownloadRequest> lRequest = msActiveRequests[i].lock())
					{
						lRequest->getImage()->mOriginalImageDownloadRequest = nullptr;
					}
				}
				msActiveRequests.erase(msActiveRequests.begin() + cMaxRequestAlive, msActiveRequests.end());
			}
		});
}