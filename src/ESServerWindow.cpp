#include "ESServerWindow.h"

// ExifStats
#include "ESDatabase.h"
#include "ESQmlBinder.h"
#include "ESDebugQmlBinder.h"

// Qt
#include <qdir.h>
#include <qfilesystemwatcher.h>
#include <qqmlengine.h>
#include <qqmlcontext.h>
#include <qquickstyle.h>
#include <qsettings.h>
#include <qtimer.h>
#include <QtConcurrent>

// Std
#include <stack>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

#if defined(QT_DEBUG)
static const char* scMainQmlDirPath = "../../../rc/Qml";
#else
static const char* scMainQmlDirPath = "ESQml";
#endif

static const char* scMainQmlLocalPath = "MainServer.qml";
static const char* scMainQmlPathQRC = "qrc:/Qml/MainServer.qml";
static const char* scFallbackQmlQRC = "qrc:/Qml/FallbackQmlErrors.qml";


/********************************************************************************/
/********************************************************************************/
/********************************************************************************/


ESServerWindow::ESServerWindow()
: mFallbackQmlLoaded(false)
{
	QQuickStyle::setStyle("Material");
	setWidth(800);
	setHeight(600);

	ESDatabase::getInstance();

	mBinder = std::make_shared<ESQmlBinder>(true);
	mDebugBinder = std::make_shared<ESDebugQmlBinder>();

	(void)connect(mBinder.get(), &ESQmlBinder::propertyFullScreenChanged, this, 
		[&]()
		{
			if (mBinder->getFullScreen())
				showFullScreen();
			else
				showNormal();
		});
}

/********************************************************************************/

/*virtual*/ ESServerWindow::~ESServerWindow() /*override*/
{
	mBinder->save();
	setSource(QUrl());
}

/********************************************************************************/

void ESServerWindow::initialize()
{
	initializeQml();
	loadMainQml();

	ESDatabase::getInstance().loadDatabase();
	mBinder->initialize(false);
}

/********************************************************************************/

void ESServerWindow::initializeQml()
{
	setResizeMode(QQuickView::SizeRootObjectToView);

	connect(engine(), &QQmlEngine::quit, this, &ESServerWindow::onQuit);
	connect(engine(), &QQmlEngine::warnings, this, &ESServerWindow::onQmlWarnings, Qt::QueuedConnection);
	engine()->setOutputWarningsToStandardError(false);

	engine()->rootContext()->setContextProperty("MainQmlBinder", mBinder.get());
	engine()->rootContext()->setContextProperty("DebugQmlBinder", mDebugBinder.get());
	engine()->rootContext()->setContextProperty("GQmlFromResources",
#ifdef QT_DEBUG
	false);
#else
	true);
#endif
}

/********************************************************************************/

QString ESServerWindow::getMainQmlFilePath() const
{
	return qApp->applicationDirPath() + QDir::separator() + QString(scMainQmlDirPath) + QDir::separator() + scMainQmlLocalPath;
}

/********************************************************************************/

void ESServerWindow::loadMainQml()
{
	QString lMainQmlFilePath = getMainQmlFilePath();
	if (QFile::exists(lMainQmlFilePath))
	{
		QStringList lQmlFiles;

		std::stack<QString> lDirsToCheck;
		lDirsToCheck.push(scMainQmlDirPath);

		while (lDirsToCheck.size() > 0)
		{
			QString lTopDir = lDirsToCheck.top();
			lDirsToCheck.pop();

			QDir lQmlDir(lTopDir);
			QFileInfoList lQmlFilesInfo = lQmlDir.entryInfoList({ "*.qml" }, QDir::NoDot | QDir::NoDotDot | QDir::Files | QDir::AllDirs);
			for (QFileInfo& lEntry : lQmlFilesInfo)
			{
				if (lEntry.isDir())
				{
					lDirsToCheck.push(lEntry.absoluteFilePath());
				}
				else
				{
					lQmlFiles.append(lEntry.absoluteFilePath());
				}
			}
		}

		mQmlFileWatcher = std::make_shared<QFileSystemWatcher>(lQmlFiles);
		(void)connect(mQmlFileWatcher.get(), &QFileSystemWatcher::fileChanged, this, &ESServerWindow::onQmlFileChanged);
		setSource(QUrl::fromLocalFile(lMainQmlFilePath));
		logQmlErrors();
	}
	else
	{
		setSource(QUrl(scMainQmlPathQRC));
		logQmlErrors();
	}
}

/********************************************************************************/

void ESServerWindow::onQuit()
{
	qInfo() << "Closing";
	engine()->rootContext()->setContextProperty("GClosing", QVariant::fromValue(true));
}

/********************************************************************************/

void ESServerWindow::onQmlFileChanged(const QString& pFilePath)
{
	Q_UNUSED(pFilePath);

	QRect lGeometry = geometry();
	QUrl lSource = source();
	setSource(QUrl());
	engine()->clearComponentCache();
	setSource(lSource);
	logQmlErrors();
	setGeometry(lGeometry);
}

/********************************************************************************/

void ESServerWindow::logQmlErrors()
{
	if (errors().size() > 0)
	{
		QString lErrors;
		for (QQmlError& lError : errors())
		{
			lErrors += "\n" + lError.toString();
		}
		qWarning() << QString("Failed to load QML file with the following errors:\n%1").arg(lErrors);
		setSource(QUrl(scFallbackQmlQRC));
		mFallbackQmlLoaded = true;
	}
	else
	{
		if (mFallbackQmlLoaded)
		{
			mFallbackQmlLoaded = false;
			setSource(QUrl::fromLocalFile(getMainQmlFilePath()));
			logQmlErrors();
		}
	}
}

/********************************************************************************/

void ESServerWindow::onQmlWarnings(const QList<QQmlError>& pWarnings)
{
	QString lWarnings;
	for (const QQmlError& lWarning : pWarnings)
	{
		lWarnings += "\n" + lWarning.toString();
	}
	qWarning() << QString("Qml warnings:\n%1").arg(lWarnings);
}
