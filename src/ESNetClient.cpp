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
#include <QSettings>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

const int cMaxRetries = 3;
const int cRetryDelayMs = 1000;
const int cMaxRequestAlive = 5;

/*static*/ QString ESNetClientOriginalImageDownloadRequest::msServerSaltedPassword;
/*static*/ QString ESNetClientOriginalImageDownloadRequest::msServerAddress;
/*static*/ quint16 ESNetClientOriginalImageDownloadRequest::msServerPort = 0;

/*static*/ std::mutex ESNetClientOriginalImageDownloadRequest::msActiveRequestsMutex;
/*static*/ std::vector<std::weak_ptr<ESNetClientOriginalImageDownloadRequest>> ESNetClientOriginalImageDownloadRequest::msActiveRequests;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

std::shared_ptr<ESNetClientOriginalImageDownloadRequest> ESNetClientOriginalImageDownloadRequest::downloadOriginalImage(const std::shared_ptr<ESImage>& pImage)
{
	std::shared_ptr<ESNetClientOriginalImageDownloadRequest> lRequest;
	if (	!ESNetClientOriginalImageDownloadRequest::msServerAddress.isEmpty()
		&&	ESNetClientOriginalImageDownloadRequest::msServerPort != 0
		&&	!ESNetClientOriginalImageDownloadRequest::msServerSaltedPassword.isEmpty())
	{
		QThread* lThread = new QThread();

		lRequest = std::make_shared<ESNetClientOriginalImageDownloadRequest>(pImage);
		lRequest->moveToThread(lThread);
		lRequest->mInternalRequest->moveToThread(lThread);

		QObject::connect(lThread, &QThread::started, lRequest.get(), &ESNetClientOriginalImageDownloadRequest::process);
		QObject::connect(lRequest.get(), &ESNetClientOriginalImageDownloadRequest::finished, lThread, &QThread::quit);
		QObject::connect(lThread, &QThread::finished, lThread, &QObject::deleteLater);

		lThread->start();

		std::lock_guard<std::mutex> lLock(msActiveRequestsMutex);
		msActiveRequests.push_back(lRequest);

		garbageCollectRequests();
	}

	return lRequest;
}

/********************************************************************************/

ESNetClientOriginalImageDownloadRequest::ESNetClientOriginalImageDownloadRequest(const std::shared_ptr<ESImage>& pImage)
	: mParentImage(pImage)
	, mIsFinished(false)
	, mInternalRequest(new ESNetClientOriginalImageDownloadRequestInternal(pImage->getImageHash(), pImage->getImageFileName()))
	, mLastUsedImage(QDateTime::currentMSecsSinceEpoch())
	, mLastMessage(ESNetClientHandler::MsgNone)
{
	QObject::connect(mInternalRequest, &ESNetClientOriginalImageDownloadRequestInternal::lastMessageChanged, this, &ESNetClientOriginalImageDownloadRequest::onInternalRequestLastMessageChanged);
	QObject::connect(mInternalRequest, &ESNetClientOriginalImageDownloadRequestInternal::finished, this, &ESNetClientOriginalImageDownloadRequest::onInternalRequestFinished);
	QObject::connect(mInternalRequest, &ESNetClientOriginalImageDownloadRequestInternal::downloadProgress, this, &ESNetClientOriginalImageDownloadRequest::downloadProgress);
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
	if(!mInternalRequest)
	{
		return;
	}
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

void ESNetClientOriginalImageDownloadRequest::onInternalRequestLastMessageChanged(ESNetClientHandler::Message pLastMessage)
{
	mLastMessage = pLastMessage;
	emit lastMessageChanged(pLastMessage);
}

/********************************************************************************/

void ESNetClientOriginalImageDownloadRequest::cancelRequest()
{
	if(mInternalRequest)
	{
		QObject::disconnect(mInternalRequest, &ESNetClientOriginalImageDownloadRequestInternal::lastMessageChanged, this, &ESNetClientOriginalImageDownloadRequest::onInternalRequestLastMessageChanged);
		QObject::disconnect(mInternalRequest, &ESNetClientOriginalImageDownloadRequestInternal::finished, this, &ESNetClientOriginalImageDownloadRequest::onInternalRequestFinished);
		QObject::disconnect(mInternalRequest, &ESNetClientOriginalImageDownloadRequestInternal::downloadProgress, this, &ESNetClientOriginalImageDownloadRequest::downloadProgress);
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

const ESNetClientHandler::Message& ESNetClientOriginalImageDownloadRequest::getLastMessage() const
{
	return mLastMessage;
}

/********************************************************************************/

const bool ESNetClientOriginalImageDownloadRequest::hasFailed() const
{
	return mInternalRequest == nullptr && (!mIsFinished || mDownloadedImage.isNull());
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

ESNetClientOriginalImageDownloadRequestInternal::ESNetClientOriginalImageDownloadRequestInternal(const QString& pImageHash, const QString& pImageFileName)
	: mImageHash(pImageHash)
	, mImageFileName(pImageFileName)
	, mRetryCount(0)
	, mIsCancelled(false)
	, mSocket(nullptr)
	, mAwaitingchallenge(true)
	, mExpectedimagesize(0)
	, mLastMessage(ESNetClientHandler::MsgNone)
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
	mSocket->connectToHost(ESNetClientOriginalImageDownloadRequest::msServerAddress, ESNetClientOriginalImageDownloadRequest::msServerPort);
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
		lMac.setKey(ESNetClientOriginalImageDownloadRequest::getSaltedPassword().toUtf8());
		lMac.addData(lChallenge);

		mSocket->write(lMac.result());

		QDataStream lStream(mSocket);
		lStream.setVersion(QDataStream::Qt_6_0);
		uint lProtocolVersion = 1; // For futur improvements and retro compatibility
		uint lRequestType = 1; // For futur improvements and retro compatibility
		lStream << lProtocolVersion;
		lStream << lRequestType;
		lStream << mImageHash;
		lStream << mImageFileName;

		mAwaitingchallenge = false;
	}
	else if (mLastMessage != ESNetClientHandler::MsgSendingFile)
	{
		if (mSocket->bytesAvailable() < static_cast<qint64>(sizeof(quint8)))
		{
			return;
		}
		QDataStream lStream(mSocket);
		lStream.setVersion(QDataStream::Qt_6_0);
		lStream >> mLastMessage;
		emit lastMessageChanged(mLastMessage);
		if(		mLastMessage == ESNetClientHandler::MsgFailedToOpenFile
			||	mLastMessage == ESNetClientHandler::MsgFileNotFound)
		{
			finish(QImage());
			return;
		}
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

		if (mExpectedimagesize > 0)
		{
			emit downloadProgress(float(mSocket->bytesAvailable()) / float(mExpectedimagesize));
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
						std::shared_ptr<ESNetClientOriginalImageDownloadRequest> lLockA = a.lock();
						std::shared_ptr<ESNetClientOriginalImageDownloadRequest> lLockB = b.lock();
						if (!lLockA) return false;
						if (!lLockB) return true;
						return lLockA->mLastUsedImage > lLockB->mLastUsedImage;
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

/********************************************************************************/

/*static*/ void ESNetClientOriginalImageDownloadRequest::initialize()
{
	QSettings lSettings;
	msServerAddress = lSettings.value("ServerAddress").toString();
	msServerPort = lSettings.value("ServerPort").toInt();
	msServerSaltedPassword = lSettings.value("ServerPassword").toString();
}

/********************************************************************************/

/*static*/ void ESNetClientOriginalImageDownloadRequest::setServerAddressAndPort(QString pAddress, quint16 pPort)
{
	QSettings lSettings;
	lSettings.setValue("ServerAddress", pAddress);
	lSettings.setValue("ServerPort", pPort);

	msServerAddress = pAddress;
	msServerPort = pPort;
}

/********************************************************************************/

/*static*/ void ESNetClientOriginalImageDownloadRequest::setPassword(const QString& pPassword)
{
	if (pPassword.size() < 20)
	{
		qWarning() << "Password too short. It should be at least 20 characters long.";
		return;
	}
	msServerSaltedPassword = QCryptographicHash::hash(pPassword.toUtf8() + "ExifStatsSalt", QCryptographicHash::Sha256).toHex();

	QSettings lSettings;
	lSettings.setValue("ServerPassword", msServerSaltedPassword);
}

/********************************************************************************/

/*static*/ QString ESNetClientOriginalImageDownloadRequest::getSaltedPassword()
{
	return msServerSaltedPassword;
}