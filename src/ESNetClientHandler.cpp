#include "ESNetClientHandler.h"

// ExifStats
#include "ESDatabase.h"
#include "ESNetServer.h"

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

ESNetClientHandler::ESNetClientHandler(qintptr pSocketDescriptor, ESNetServer* pParent)
	: QObject()
	, mServer(pParent)
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
		connect(mSocket, &QTcpSocket::disconnected, this, &ESNetClientHandler::onDisconnected);
			
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
		lMac.setKey(mServer->getSaltedPassword().toUtf8());
		lMac.addData(mChallengeNonce);
			
		if (lResponse == lMac.result())
		{
			mIsAuthenticated = true;
		}
		else
		{
			qWarning() << "Incorrect password, disconnecting client: " << mSocket->peerAddress().toString();
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
		QString lRequestedFileName;

		lStream >> lProtocolVersion;
		if(lProtocolVersion == 1)
		{
			lStream >> lRequestType;
			if(lRequestType == 1)
			{
				lStream >> lRequestedFileHash;
				lStream >> lRequestedFileName;
			
				if (lStream.commitTransaction())
				{
					// Prevent memory exhaustion attacks from malicious path sizes
					if (lRequestedFileHash.size() > 1024)
					{
						qWarning() << "Incorrect file hash size, disconnecting client: " << mSocket->peerAddress().toString();
						mSocket->disconnectFromHost();
						return;
					}

					QPointer<ESNetClientHandler> lSafeThis(this);

					qInfo() << "File Requested '" << lRequestedFileHash << " / " << lRequestedFileName << "' by client: " << mSocket->peerAddress().toString();
				
					QtConcurrent::run([lSafeThis, lRequestedFileHash, lRequestedFileName, lPeerAddress = mSocket->peerAddress().toString()]()
						{
							QByteArray lData;
					
							const std::vector<const ESFileInfo*> lRequestedFileInfos = ESDatabase::getInstance().getFileInfoFromHash(lRequestedFileHash);
							const ESFileInfo* lRequestedFileInfo = nullptr;
							if(!lRequestedFileInfos.empty())
							{
								if (lRequestedFileInfos.size() > 1)
								{
									for(auto lFileInfo : lRequestedFileInfos)
									{
										QString lFileName = QFileInfo(lFileInfo->mFilePath.getString()).fileName();
										if(lRequestedFileName.compare(lFileName, Qt::CaseInsensitive) == 0)
										{
											lRequestedFileInfo = lFileInfo;
											break;
										}
									}
								}
								else
								{
									lRequestedFileInfo = lRequestedFileInfos.front();
								}
							}

							if(!lRequestedFileInfo)
							{
								static const bool lsIsDebugHash = qApp->arguments().contains("-debughash");
								if(lsIsDebugHash)
								{
									// Search all database images with the same file name and print the hash data for debugging purposes
									// Loop over all fles in the database
									for(const auto& [lFileInfoId, lFileInfo] : ESDatabase::getInstance().getFiles())
									{
										QString lFileName = QFileInfo(lFileInfo.mFilePath.getString()).fileName();
										if(lFileName.endsWith(lRequestedFileName, Qt::CaseInsensitive))
										{
											qInfo() << "Found file with same name: " << lFileInfo.mFilePath.getString() << ", hash: " << lFileInfo.mHash;
											ESHash<true> lHash(QCryptographicHash::Sha256);
											lFileInfo.addHashData(lHash);
											qInfo() << "Hash data: " << lHash.getData();
										}
									}
								}
								QMetaObject::invokeMethod(lSafeThis.data(),[lSafeThis](){lSafeThis->sendMessage(ESNetClientHandler::MsgFileNotFound);},	Qt::QueuedConnection);
								qWarning() << "File info not found for hash/name: " << lRequestedFileHash << " / " << lRequestedFileName << ", disconnecting client: " << lPeerAddress;
								return;
							}
							QMetaObject::invokeMethod(lSafeThis.data(), [lSafeThis]() {lSafeThis->sendMessage(ESNetClientHandler::MsgOpeningFile); }, Qt::QueuedConnection);
							QFileInfo lInfo(lRequestedFileInfo->mFilePath.getString());
							QString lSuffix = lInfo.suffix().toLower();
							QImage lImage(lRequestedFileInfo->mFilePath.getString());
							if (lImage.isNull())
							{
								QMetaObject::invokeMethod(lSafeThis.data(), [lSafeThis]() {lSafeThis->sendMessage(ESNetClientHandler::MsgFailedToOpenFile); }, Qt::QueuedConnection);
								qWarning() << "Failed to open file: " << lRequestedFileInfo->mFilePath.getString() << ", disconnecting client: " << lPeerAddress;
								return;
							}
							else
							{
								QMetaObject::invokeMethod(lSafeThis.data(), [lSafeThis]() {lSafeThis->sendMessage(ESNetClientHandler::MsgConvertingFile); }, Qt::QueuedConnection);
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
								QMetaObject::invokeMethod(lSafeThis.data(), [lSafeThis]() {lSafeThis->sendMessage(ESNetClientHandler::MsgSendingFile); }, Qt::QueuedConnection);
								qInfo() << "Sending file '" << lRequestedFileInfo->mFilePath.getString() << "' to client: " << lPeerAddress << "...";
							}
							
							QMetaObject::invokeMethod(
								lSafeThis.data(),
								[lSafeThis, lData = std::move(lData), lFilePath = lRequestedFileInfo ? lRequestedFileInfo->mFilePath.getString() : "(not found)"]()
								{
									if (lSafeThis)
									{
										lSafeThis->sendImageData(lData, lFilePath);
									}
								},
								Qt::QueuedConnection);
						});
				}
			}
			else
			{
				qWarning() << "Unknown request type: " << lRequestType << ", disconnecting client: " << mSocket->peerAddress().toString();
				mSocket->disconnectFromHost();
				return;
			}
		}
		else
		{
			qWarning() << "Unknown protocol version: " << lProtocolVersion << ", disconnecting client: " << mSocket->peerAddress().toString();
			mSocket->disconnectFromHost();
			return;
		}
	}
}

/********************************************************************************/

void ESNetClientHandler::sendMessage(Message pMsg)
{
	if (mSocket && mSocket->state() == QAbstractSocket::ConnectedState)
	{
		QDataStream lStream(mSocket);
		lStream.setVersion(QDataStream::Qt_6_0);
		lStream << pMsg;

		lStream.commitTransaction();

		if (	pMsg == MsgFailedToOpenFile
			||	pMsg == MsgFileNotFound)
		{
			mSocket->waitForBytesWritten(3000);
			if(mSocket) // Can be deleted after the wait if disconnected
				mSocket->disconnectFromHost();
		}
	}
}

/********************************************************************************/

void ESNetClientHandler::sendImageData(const QByteArray& pData, QString pFilePath)
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
			qInfo() << "File '" << pFilePath << "' successfully sent to client: " << mSocket->peerAddress().toString();
		}
		else
		{
			qWarning() << "File '" << pFilePath << "' not found or failed to load, sending empty data to client: " << mSocket->peerAddress().toString();
		}

		
		mSocket->disconnectFromHost();
	}
}

/********************************************************************************/

void ESNetClientHandler::onDisconnected()
{
	qInfo() << "Client disconnected: " << mSocket->peerAddress().toString();
	mSocket->deleteLater();
	mSocket = nullptr;
	emit finished();
}
