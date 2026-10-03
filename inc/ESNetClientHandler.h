#pragma once

// Qt
#include <QObject>
#include <QByteArray>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class QTcpSocket;
class ESNetServer;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESNetClientHandler : public QObject
{
	Q_OBJECT

public:
	
	enum Message : quint8
	{
		MsgNone = 0,

		MsgFirstSuccess = 1,
		MsgOpeningFile = MsgFirstSuccess,
		MsgConvertingFile,
		MsgSendingFile,
		MsgSucessCount,

		MsgFirstError = 10,
		MsgFileNotFound = MsgFirstError,
		MsgFailedToOpenFile,
		MsgErrorCount,
	};

	/********************************* METHODS ***********************************/

	explicit ESNetClientHandler(qintptr pSocketDescriptor, ESNetServer* pParent);
	void initializeConnection();

signals:
	/********************************* SIGNALS ***********************************/

	void finished();

private slots:
	/********************************* METHODS ***********************************/

	void sendMessage(Message pMsg);
	void sendImageData(const QByteArray& pData, QString pFilePath);

private:
	/********************************* METHODS ***********************************/

	void processReadyRead();
	void onDisconnected();

	/******************************** ATTRIBUTES **********************************/

	ESNetServer* mServer;
	qintptr mSocketDescriptor;
	QTcpSocket* mSocket;
	QByteArray mChallengeNonce;
	bool mIsAuthenticated;
};
