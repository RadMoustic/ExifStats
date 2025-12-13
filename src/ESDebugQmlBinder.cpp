#include <ESDebugQmlBinder.h>

// ES
#include <ESLogger.h>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

ESDebugQmlBinder::ESDebugQmlBinder()
	: mConsoleLines(new QStandardItemModel())
{
	for (QString lLine : ESLogger::get().getLoggedMessages())
	{
		mConsoleLines->appendRow(new QStandardItem(lLine));
	}
	connect(&ESLogger::get(), &ESLogger::newMsg, this, &ESDebugQmlBinder::onNewLoggerMsg);
}

/********************************************************************************/

/*virtual*/ ESDebugQmlBinder::~ESDebugQmlBinder()
{
	QStandardItemModel* lConsoleLines = getConsoleLines();
	setConsoleLines(nullptr);
	delete lConsoleLines;
}

/********************************************************************************/

void ESDebugQmlBinder::onNewLoggerMsg(QString pMsg, int pLevel)
{
	Q_UNUSED(pLevel)

	QMutexLocker lLocker(&mMutex);
	mConsoleLines->appendRow(new QStandardItem(pMsg));
}
