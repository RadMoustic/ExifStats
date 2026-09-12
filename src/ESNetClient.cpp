#include "ESNetClient.h"

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

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

std::shared_ptr<ESNetClientOriginalImageDownloadRequest> ESNetClientOriginalImageDownloadRequest::downloadOriginalImage(QString pImagePath, const QString& pHost, quint16 pPort, const std::function<void(const QImage&)>& pFinishedCallback)
{
	QThread* lThread = new QThread();
	std::shared_ptr<ESNetClientOriginalImageDownloadRequest> lRequest = std::make_shared<ESNetClientOriginalImageDownloadRequest>(pImagePath, pHost, pPort, pFinishedCallback);
	lRequest->moveToThread(lThread);
	lRequest->mInternalRequest->moveToThread(lThread);

	QObject::connect(lThread, &QThread::started, lRequest.get(), &ESNetClientOriginalImageDownloadRequest::process);
	QObject::connect(lRequest.get(), &ESNetClientOriginalImageDownloadRequest::finished, lThread, &QThread::quit);
	QObject::connect(lThread, &QThread::finished, lThread, &QObject::deleteLater);

	lThread->start();

	return lRequest;
}

/********************************************************************************/

ESNetClientOriginalImageDownloadRequest::ESNetClientOriginalImageDownloadRequest(const QString& pPath, const QString& pHost, quint16 pPort, const std::function<void(const QImage&)>& pFinishedCallback)
{
	mInternalRequest = new ESNetClientOriginalImageDownloadRequestInternal(pPath, pHost, pPort, pFinishedCallback);
	QObject::connect(mInternalRequest, &ESNetClientOriginalImageDownloadRequestInternal::finished, this, &ESNetClientOriginalImageDownloadRequest::onInternalRequestFinished);
}

/********************************************************************************/

/*virtual*/ ESNetClientOriginalImageDownloadRequest::~ESNetClientOriginalImageDownloadRequest() /*override*/
{
	QObject::disconnect(mInternalRequest, &ESNetClientOriginalImageDownloadRequestInternal::finished, this, &ESNetClientOriginalImageDownloadRequest::onInternalRequestFinished);
	cancelRequest();
}

/********************************************************************************/

void ESNetClientOriginalImageDownloadRequest::onInternalRequestFinished()
{
	emit finished();
	mInternalRequest = nullptr;
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

ESNetClientOriginalImageDownloadRequestInternal::ESNetClientOriginalImageDownloadRequestInternal(const QString& pPath, const QString& pHost, quint16 pPort, const std::function<void(const QImage&)>& pCallback)
	: mPath(pPath)
	, mHost(pHost)
	, mPort(pPort)
	, mFinishedCallback(pCallback)
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
		lStream << mPath;

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
	mFinishedCallback(pImage);
	emit finished();
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
