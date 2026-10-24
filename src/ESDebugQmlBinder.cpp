#include <ESDebugQmlBinder.h>

// ES
#include <ESLogger.h>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

ESDebugQmlBinder::ESDebugQmlBinder()
	: mConsoleLines(new QStandardItemModel())
	, mPreviousSessionLogs(new QStandardItemModel())
{
	for (QString lLine : ESLogger::get().getLoggedMessages())
	{
		mConsoleLines->appendRow(new QStandardItem(lLine));
	}
	for (QString lLine : ESLogger::get().getPreviousSessionLoggedMessages())
	{
		mPreviousSessionLogs->appendRow(new QStandardItem(lLine));
	}
	connect(&ESLogger::get(), &ESLogger::newMsg, this, &ESDebugQmlBinder::onNewLoggerMsg);
}

/********************************************************************************/

/*virtual*/ ESDebugQmlBinder::~ESDebugQmlBinder()
{
	QStandardItemModel* lConsoleLines = getConsoleLines();
	QStandardItemModel* lPreviousSessionLogs = getPreviousSessionLogs();
	setConsoleLines(nullptr);
	setPreviousSessionLogs(nullptr);
	delete lConsoleLines;
	delete lPreviousSessionLogs;
}

/********************************************************************************/

void ESDebugQmlBinder::onNewLoggerMsg(QString pMsg, int pLevel)
{
	Q_UNUSED(pLevel)

	QMutexLocker lLocker(&mMutex);
	mConsoleLines->appendRow(new QStandardItem(pMsg));
}
