#pragma once

// Qt
#include <QTcpServer>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESNetServer : public QTcpServer
{
	Q_OBJECT

public:
	/********************************* METHODS ***********************************/

	explicit ESNetServer(QObject* pParent = nullptr);

protected:
	/********************************* METHODS ***********************************/

	void incomingConnection(qintptr pSocketDescriptor) override;
};
