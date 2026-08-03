#include "ESNetServer.h"

// ES
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
}

/********************************************************************************/

void ESNetServer::incomingConnection(qintptr pSocketDescriptor)
{
	QThread* lThread = new QThread(this);
	ESNetClientHandler* lHandler = new ESNetClientHandler(pSocketDescriptor);
		
	lHandler->moveToThread(lThread);
		
	connect(lThread, &QThread::started, lHandler, &ESNetClientHandler::initializeConnection);
	connect(lHandler, &ESNetClientHandler::finished, lThread, &QThread::quit);
	connect(lHandler, &ESNetClientHandler::finished, lHandler, &QObject::deleteLater);
	connect(lThread, &QThread::finished, lThread, &QObject::deleteLater);
		
	lThread->start();
}
