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

std::shared_ptr<ESNetClientOriginalImageDownloadRequest> ESNetClient::downloadOriginalImage(QString pImagePath, const QString& pHost, quint16 pPort, const std::function<void(const QImage&)>& pFinishedCallback)
{
	QThread* lThread = new QThread();
	std::shared_ptr<ESNetClientOriginalImageDownloadRequest> lRequest = std::make_shared<ESNetClientOriginalImageDownloadRequest>(pImagePath, pHost, pPort, pFinishedCallback);
	lRequest->moveToThread(lThread);

	QObject::connect(lThread, &QThread::started, lRequest.get(), &ESNetClientOriginalImageDownloadRequest::process);
	QObject::connect(lRequest.get(), &ESNetClientOriginalImageDownloadRequest::finishedthread, lThread, &QThread::quit);
	QObject::connect(lThread, &QThread::finished, lThread, &QObject::deleteLater);

	lThread->start();

	return lRequest;
}

/********************************************************************************/

ESNetClientOriginalImageDownloadRequest::ESNetClientOriginalImageDownloadRequest(const QString& pPath, const QString& pHost, quint16 pPort, const std::function<void(const QImage&)>& pFinishedCallback)
	: mPath(pPath)
	, mHost(pHost)
	, mPort(pPort)
	, mFinishedCallback(pFinishedCallback)
	, mRetryCount(0)
	, mIsCancelled(false)
{
}

/********************************************************************************/

/*virtual*/ ESNetClientOriginalImageDownloadRequest::~ESNetClientOriginalImageDownloadRequest() /*override*/
{
	cancelRequest();
}

/********************************************************************************/

void ESNetClientOriginalImageDownloadRequest::cancelRequest()
{
	mIsCancelled = true;
	if(mSocket)
	{
		mSocket->abort();
		mFinishedCallback(QImage());
	}
}

/********************************************************************************/

void ESNetClientOriginalImageDownloadRequest::process()
{
	mSocket = new QTcpSocket();
	connect(mSocket, &QTcpSocket::readyRead, this, &ESNetClientOriginalImageDownloadRequest::readbytes);
	connect(mSocket, &QTcpSocket::errorOccurred, this, &ESNetClientOriginalImageDownloadRequest::handleerror);

	mAwaitingchallenge = true;
	mSocket->connectToHost(mHost, mPort);
}

/********************************************************************************/

void ESNetClientOriginalImageDownloadRequest::readbytes()
{
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
				finished(QImage());
			}
		}

		if (mSocket->bytesAvailable() >= static_cast<qint64>(mExpectedimagesize))
		{
			QByteArray lData = mSocket->read(mExpectedimagesize);
			QImage lImage;
			lImage.loadFromData(lData, "JPG");

			finished(lImage);
		}
	}
}

/********************************************************************************/

void ESNetClientOriginalImageDownloadRequest::finished(QImage& pImage)
{
	mSocket->disconnectFromHost();
	mSocket->deleteLater();
	mSocket = nullptr;
	mFinishedCallback(pImage);
	emit finishedthread();
}

/********************************************************************************/

void ESNetClientOriginalImageDownloadRequest::handleerror(QAbstractSocket::SocketError /*pError*/)
{
	if (mIsCancelled)
	{
		return;
	}

	mSocket->abort();
	mSocket->deleteLater();
	mSocket = nullptr;

	if (mRetryCount < cMaxRetries)
	{
		mRetryCount++;
		QTimer::singleShot(cRetryDelayMs, this, &ESNetClientOriginalImageDownloadRequest::process);
	}
	else
	{
		finished(QImage());
	}
}
