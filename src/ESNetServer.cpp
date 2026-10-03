#include "ESNetServer.h"

// ES
#include "ESDatabase.h"
#include "ESNetClientHandler.h"

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

ESNetServer::ESNetServer(QObject* pParent)
	: QTcpServer(pParent)
{
	QSettings lSettings;
	mSaltedPassword = lSettings.value("ServerPassword").toString();
}

/********************************************************************************/

void ESNetServer::incomingConnection(qintptr pSocketDescriptor)
{
	if(mSaltedPassword.isEmpty())
	{
		qInfo() << "No password set for the server. Connection rejected.";
		return;
	}
	QThread* lThread = new QThread(this);
	ESNetClientHandler* lHandler = new ESNetClientHandler(pSocketDescriptor, this);
		
	lHandler->moveToThread(lThread);
		
	connect(lThread, &QThread::started, lHandler, &ESNetClientHandler::initializeConnection);
	connect(lHandler, &ESNetClientHandler::finished, lThread, &QThread::quit);
	connect(lHandler, &ESNetClientHandler::finished, lHandler, &QObject::deleteLater);
	connect(lThread, &QThread::finished, lThread, &QObject::deleteLater);
		
	lThread->start();
}

/********************************************************************************/

void ESNetServer::setPassword(const QString& pPassword)
{
	if(pPassword.size() < 20)
	{
		qWarning() << "Password too short. It should be at least 20 characters long.";
		return;
	}
	mSaltedPassword = QCryptographicHash::hash(pPassword.toUtf8() + "ExifStatsSalt", QCryptographicHash::Sha256).toHex();

	QSettings lSettings;
	lSettings.setValue("ServerPassword", mSaltedPassword);
}

/********************************************************************************/

const QString& ESNetServer::getSaltedPassword() const
{
	return mSaltedPassword;
}