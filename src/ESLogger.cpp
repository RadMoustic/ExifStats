#include <ESLogger.h>

// Qt
#include <qdatetime.h>
#include <qstandardpaths.h>
#include <qdir.h>
#include <qapplicationstatic.h>

// Stl
#include <iostream>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

/*static*/ std::shared_ptr<ESLogger> ESLogger::msInstance;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

/*static*/ void ESLogger::qtMessageHandler(QtMsgType pType, const QMessageLogContext& /*pContext*/, const QString& pMsg)
{
	QString lMsg = pMsg;
	PpcLogLevel lLogLevel = eInformation;
	switch (pType)
	{
	case QtDebugMsg:
		lLogLevel = eInformation;
		break;
	case QtInfoMsg:
		lLogLevel = eInformation;
		break;
	case QtWarningMsg:
	{
		lLogLevel = eWarning;
		break;
	}
	case QtCriticalMsg:
		lLogLevel = eError;
		break;
	case QtFatalMsg:
		lLogLevel = eFatal;
		break;
	}
	//ESLogger::get().logMsg(QString("%1 (%2:%3, %4)").arg(lMsg).arg(pContext.file).arg(pContext.line).arg(pContext.function), lLogLevel);
	ESLogger::get().logMsg(lMsg, lLogLevel);
}

/********************************************************************************/

ESLogger::ESLogger()
	: mLogFile()
{
	QString lAppDataLocation = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
	if (!lAppDataLocation.isEmpty())
	{
		QDir().mkpath(lAppDataLocation);
	}
	mLogFile.setFileName(QString("%1/%2.log").arg(lAppDataLocation).arg(qApp->applicationName()));
	if (!mLogFile.open(QFileDevice::WriteOnly))
	{
		logMsg(QString("Failed to open log file: '%1'").arg(mLogFile.fileName()), eFatal);
	}
}

/********************************************************************************/

/*static*/ ESLogger& ESLogger::get()
{
	if (!msInstance)
	{
		msInstance = std::shared_ptr<ESLogger>(new ESLogger());
	}

	return *msInstance;
}

/********************************************************************************/

void ESLogger::logMsg(const QString& pMsg, PpcLogLevel pLevel)
{
#ifndef QT_DEBUG
	if(pLevel == eDebug)
		return;
#endif
	QString lFormatedMsg = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
	lFormatedMsg += " - ";
	switch (pLevel)
	{
	case eDebug:
		lFormatedMsg += "Debug: ";
		break;
	case eInformation:
		lFormatedMsg += "Info: ";
		break;
	case eWarning:
		lFormatedMsg += "Warning: ";
		break;
	case eError:
		lFormatedMsg += "Error: ";
		break;
	case eFatal:
		lFormatedMsg += "Fatal: ";
		break;
	}
	lFormatedMsg += pMsg;
	std::cout << lFormatedMsg.toUtf8().data() << '\n';

	if (mLogFile.isOpen())
	{
		QMutexLocker lLocker(&mLogFileMutex);
		mLogFile.write(lFormatedMsg.toLocal8Bit());
		mLogFile.write("\n", 1);
		mLogFile.flush();
	}

	if (pLevel == eFatal)
	{
		std::flush(std::cout);
		__debugbreak();
		exit(-1);
	}
	else if (pLevel == eError)
	{
		std::flush(std::cout);
		// Errors should be fixed.
		__debugbreak();
	}
	/*
	else if (pLevel == eWarning)
	{
		std::flush(std::cout);
		__debugbreak();
	}
	*/

	{
		QMutexLocker lLocker(&mMutex);
		mMsgsLogged.append(lFormatedMsg);
	}

	emit newMsg(lFormatedMsg, static_cast<int>(pLevel));
}

/********************************************************************************/

const QStringList& ESLogger::getLoggedMessages() const
{
	return mMsgsLogged;
}
