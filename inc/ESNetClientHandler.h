#pragma once

// Qt
#include <QObject>
#include <QByteArray>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class QTcpSocket;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESNetClientHandler : public QObject
{
	Q_OBJECT

public:
	/********************************* METHODS ***********************************/

	explicit ESNetClientHandler(qintptr pSocketDescriptor, QObject* pParent = nullptr);
	void initializeConnection();

signals:
	/********************************* SIGNALS ***********************************/

	void finished();

private slots:
	/********************************* METHODS ***********************************/

	void sendImageData(const QByteArray& pData);

private:
	/********************************* METHODS ***********************************/

	void processReadyRead();
	void terminateConnection();

	/******************************** ATTRIBUTES **********************************/

	qintptr mSocketDescriptor;
	QTcpSocket* mSocket;
	QByteArray mChallengeNonce;
	bool mIsAuthenticated;
};
