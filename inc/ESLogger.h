#pragma once

// Qt
#include <qfile.h>
#include <qobject.h>
#include <qmutex.h>

// Stl
#include <memory>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

Q_NAMESPACE

enum PpcLogLevel
{
	eInformation = 0,
	eDebug,
	eWarning,
	eError,
	eFatal
};

Q_ENUM_NS(PpcLogLevel)

class ESLogger : public QObject
{
	Q_OBJECT
public:
	/********************************* METHODS ***********************************/

	static ESLogger& get();

	const QStringList& getLoggedMessages() const;

	void logMsg(const QString& pMsg, PpcLogLevel pLevel);

	static void qtMessageHandler(QtMsgType pType, const QMessageLogContext& pContext, const QString& pMsg);

signals:
	/********************************* SIGNALS ***********************************/

	void newMsg(QString pMsg, int pLevel);

protected:
	/********************************* METHODS ***********************************/

	ESLogger();

	/******************************** ATTRIBUTES **********************************/

	static std::shared_ptr<ESLogger> msInstance;
	QStringList mMsgsLogged;
	QFile mLogFile;
	QMutex mMutex;
};

