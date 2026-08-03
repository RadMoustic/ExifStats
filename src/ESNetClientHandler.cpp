#include "ESNetClientHandler.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QImage>
#include <QBuffer>
#include <QtConcurrent>
#include <QRandomGenerator>
#include <QMessageAuthenticationCode>
#include <QDataStream>
#include <QCryptographicHash>
#include <QPointer>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

const QByteArray cSharedSecret = "MyUniqueSecureCode123";

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

ESNetClientHandler::ESNetClientHandler(qintptr pSocketDescriptor, QObject* pParent)
	: QObject(pParent)
	, mSocketDescriptor(pSocketDescriptor)
	, mSocket(nullptr)
	, mIsAuthenticated(false)
{
}

/********************************************************************************/

void ESNetClientHandler::initializeConnection()
{
	mSocket = new QTcpSocket(this);
		
	if (mSocket->setSocketDescriptor(mSocketDescriptor))
	{
		connect(mSocket, &QTcpSocket::readyRead, this, &ESNetClientHandler::processReadyRead);
		connect(mSocket, &QTcpSocket::disconnected, this, &ESNetClientHandler::terminateConnection);
			
		quint32 lBuffer[8];
		QRandomGenerator::system()->fillRange(lBuffer);
		mChallengeNonce = QByteArray(reinterpret_cast<const char*>(lBuffer), sizeof(lBuffer));

		mSocket->write(mChallengeNonce);
	}
	else
	{
		emit finished();
	}
}

/********************************************************************************/

void ESNetClientHandler::processReadyRead()
{
	if (!mIsAuthenticated)
	{
		if (mSocket->bytesAvailable() < 32)
		{
			return;
		}

		QByteArray lResponse = mSocket->read(32);
		QMessageAuthenticationCode lMac(QCryptographicHash::Sha256);
		lMac.setKey(cSharedSecret);
		lMac.addData(mChallengeNonce);
			
		if (lResponse == lMac.result())
		{
			mIsAuthenticated = true;
		}
		else
		{
			mSocket->disconnectFromHost();
			return;
		}
	}

	if (mIsAuthenticated)
	{
		QDataStream lStream(mSocket);
		lStream.setVersion(QDataStream::Qt_6_0);
			
		lStream.startTransaction();
		QString lRequestedPath;
		lStream >> lRequestedPath;
			
		if (lStream.commitTransaction())
		{
			// Prevent memory exhaustion attacks from malicious path sizes
			if (lRequestedPath.size() > 1024)
			{
				mSocket->disconnectFromHost();
				return;
			}

			QPointer<ESNetClientHandler> lSafeThis(this);
				
			QtConcurrent::run([lSafeThis, lRequestedPath]()
				{
					QByteArray lData;
					QFileInfo lInfo(lRequestedPath);
					QString lSuffix = lInfo.suffix().toLower();

					bool lIsAlreadyJpeg = (lSuffix == "jpg" || lSuffix == "jpeg");

					if (lIsAlreadyJpeg)
					{
						QFile lFile(lRequestedPath);
						if (lFile.open(QIODevice::ReadOnly))
						{
							lData = lFile.readAll();
						}
					}
					else
					{
						QImage lImage(lRequestedPath);
						if (!lImage.isNull())
						{
							QBuffer lBuffer(&lData);
							lBuffer.open(QIODevice::WriteOnly);
							// Convert to JPEG with quality 90
							lImage.save(&lBuffer, "JPG", 90);
						}
					}

					if (lSafeThis)
					{
						QMetaObject::invokeMethod(lSafeThis, "sendImageData", Qt::QueuedConnection, Q_ARG(QByteArray, lData));
					}
				});
		}
	}
}

/********************************************************************************/

void ESNetClientHandler::sendImageData(const QByteArray& pData)
{
	if (mSocket && mSocket->state() == QAbstractSocket::ConnectedState)
	{
		QDataStream lStream(mSocket);
		lStream.setVersion(QDataStream::Qt_6_0);

		// Send explicit size
		lStream << static_cast<quint32>(pData.size());

		// Send raw bytes if any
		if (!pData.isEmpty())
		{
			mSocket->write(pData);
		}

		mSocket->disconnectFromHost();
	}
}

/********************************************************************************/

void ESNetClientHandler::terminateConnection()
{
	mSocket->deleteLater();
	mSocket = nullptr;
	emit finished();
}
