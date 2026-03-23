#pragma once

// Qt
#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QString>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESPerfLog
{
public:
	/********************************* METHODS ***********************************/
	
	explicit ESPerfLog(const QString& pLabel)
		: mLabel(pLabel)
	{
		mTimer.start();
	}

	~ESPerfLog()
	{
		qInfo("%s: %lld ns", qPrintable(mLabel), mTimer.nsecsElapsed());
	}

private:
	/******************************** ATTRIBUTES **********************************/
	QString mLabel;
	QElapsedTimer mTimer;
};