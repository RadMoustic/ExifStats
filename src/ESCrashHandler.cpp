#include "ESCrashHandler.h"

// ES
#include "ESLogger.h"

// Qt
#include <QStandardPaths>
#include <QDateTime>
#include <QSettings>
#include <QDir>
#include <QFile>

// Stl
#include <csignal>
#include <fstream>

// Backward
#ifdef Q_OS_ANDROID
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>

extern "C" {
	inline char** backtrace_symbols(void* const* pBuffer, int pSize)
	{
		char** lResult = (char**)malloc(sizeof(char*) * pSize);
		for (int i = 0; i < pSize; ++i)
		{
			Dl_info lInfo;
			if (dladdr(pBuffer[i], &lInfo) && lInfo.dli_sname)
			{
				lResult[i] = strdup(lInfo.dli_sname);
			}
			else
			{
				lResult[i] = strdup("??");
			}
		}
		return lResult;
	}

	inline int backtrace(void** pBuffer, int pSize) { return 0; }
}
#else
#define BACKWARD_HAS_UNWIND 1
#define BACKWARD_HAS_BACKTRACE_SYMBOL 1
#define BACKWARD_HAS_PDB_SYMBOL_CONTAINER 1
#endif // Q_OS_ANDROID

#pragma warning( push, 0 )
#pragma warning( disable : 4996 )
#include "backward.hpp"
#pragma warning( pop )

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

void ESCrashHandler::init()
{
#ifdef Q_OS_WIN
    std::signal(SIGSEGV, ESCrashHandler::handleSignal);
    std::signal(SIGABRT, ESCrashHandler::handleSignal);
    std::signal(SIGFPE, ESCrashHandler::handleSignal);
    std::signal(SIGILL, ESCrashHandler::handleSignal);
#else
    struct sigaction lAction;
    lAction.sa_sigaction = ESCrashHandler::handleSignal;
    lAction.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&lAction.sa_mask);
    sigaction(SIGSEGV, &lAction, nullptr);
    sigaction(SIGABRT, &lAction, nullptr);
    sigaction(SIGFPE, &lAction, nullptr);
    sigaction(SIGILL, &lAction, nullptr);
    sigaction(SIGBUS, &lAction, nullptr);
    sigaction(SIGTRAP, &lAction, nullptr);
#endif
}

/********************************************************************************/

#ifdef Q_OS_WIN
void ESCrashHandler::handleSignal(int pSignal)
{
#else
void ESCrashHandler::handleSignal(int pSignal, siginfo* pInfo, void* pContext)
{
Q_UNUSED(pInfo);
#endif
	qWarning("Application crashed with signal: %d", pSignal);

	QString lAppDataPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
	QDir lDir(lAppDataPath);
	if (!lDir.exists())
	{
		lDir.mkpath(".");
	}

	QString lDateTime = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
	QString lCrashLogPath = lDir.filePath("Crash_" + lDateTime + ".log");

	std::ofstream lFileStream(lCrashLogPath.toStdString());
	if (lFileStream.is_open())
	{
		lFileStream << ESLogger::get().getLoggedMessages().join("\n").toLocal8Bit().toStdString() << std::endl;

		backward::StackTrace lStackTrace;
#ifdef Q_OS_WIN
        lStackTrace.load_here(32);
#else
        lStackTrace.load_from(pContext, 32);
#endif
#ifdef Q_OS_ANDROID
		lStackTrace.skip_n_firsts(4);
#else
		lStackTrace.skip_n_firsts(8);
#endif // _MSC_VER
		
		backward::Printer lPrinter;
		lPrinter.object = true;
		lPrinter.color_mode = backward::ColorMode::never;
		lPrinter.address = true;
		lPrinter.snippet = true;
		lPrinter.print(lStackTrace, lFileStream);
		
		lFileStream.close();

		QSettings lSettings;
		lSettings.setValue("LastCrashLog", lCrashLogPath);
		lSettings.sync();

		qWarning("Crash log saved to: %s", lCrashLogPath.toStdString().c_str());
	}
	else
	{
		qWarning("Failed to open crash log file for writing: %s", lCrashLogPath.toStdString().c_str());
	}

	std::exit(pSignal);
}

/********************************************************************************/

bool ESCrashHandler::hasPreviousCrash()
{
	QSettings lSettings;
	return lSettings.contains("LastCrashLog");
}

/********************************************************************************/

QString ESCrashHandler::getPreviousCrashLogs()
{
	QString lContent;
	QSettings lSettings;
	
	if (lSettings.contains("LastCrashLog"))
	{
		QString lLogPath = lSettings.value("LastCrashLog").toString();
		QFile lFile(lLogPath);
		if (lFile.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			lContent = lFile.readAll();
			lFile.close();
		}
	}
	
	return lContent;
}

/********************************************************************************/

void ESCrashHandler::resetPreviousCrash()
{
	QSettings lSettings;
	lSettings.remove("LastCrashLog");
}
