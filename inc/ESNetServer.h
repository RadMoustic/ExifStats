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

	void setPassword(const QString& pPassword);
	const QString& getSaltedPassword() const;

protected:
	/******************************** ATTRIBUTES **********************************/

	QString mSaltedPassword;

	/********************************* METHODS ***********************************/

	void incomingConnection(qintptr pSocketDescriptor) override;
};
