#include "ESNetClientHandler.h"

// ExifStats
#include "ESDatabase.h"

// Qt
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

		uint lProtocolVersion = 0;
		uint lRequestType = 0;
		QString lRequestedFileHash;

		lStream >> lProtocolVersion;
		if(lProtocolVersion == 1)
		{
			lStream >> lRequestType;
			if(lRequestType == 1)
			{
				lStream >> lRequestedFileHash;
			
				if (lStream.commitTransaction())
				{
					// Prevent memory exhaustion attacks from malicious path sizes
					if (lRequestedFileHash.size() > 1024)
					{
						mSocket->disconnectFromHost();
						return;
					}

					QPointer<ESNetClientHandler> lSafeThis(this);
				
					QtConcurrent::run([lSafeThis, lRequestedFileHash]()
						{
							QByteArray lData;
					
							const ESFileInfo* lRequestedFileInfo = ESDatabase::getInstance().getFileInfoFromHash(lRequestedFileHash);
							if(lRequestedFileInfo)
							{
								QFileInfo lInfo(lRequestedFileInfo->mFilePath.getString());
								QString lSuffix = lInfo.suffix().toLower();
								QImage lImage(lRequestedFileInfo->mFilePath.getString());
								if (!lImage.isNull())
								{
									QBuffer lBuffer(&lData);
									lBuffer.open(QIODevice::WriteOnly);
									if (lImage.width() > 4096 || lImage.height() > 4096)
									{
										if(lImage.width() > lImage.height())
										{
											lImage = lImage.scaledToWidth(4096, Qt::SmoothTransformation);
										}
										else
										{
											lImage = lImage.scaledToHeight(4096, Qt::SmoothTransformation);
										}
									}
									lImage.save(&lBuffer, "JPG", 90);
								}
							}

							QMetaObject::invokeMethod(
								lSafeThis.data(),
								[lSafeThis, lData = std::move(lData)]()
								{
									if (lSafeThis)
									{
										lSafeThis->sendImageData(lData);
									}
								},
								Qt::QueuedConnection);
						});
				}
			}
			else
			{
				qInfo() << "Unknown request type: " << lRequestType;
				mSocket->disconnectFromHost();
				return;
			}
		}
		else
		{
			qInfo() << "Unknown protocol version: " << lProtocolVersion;
			mSocket->disconnectFromHost();
			return;
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
