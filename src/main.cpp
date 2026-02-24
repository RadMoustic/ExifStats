// ExifStats
#include "ESStringPool.h"
#include "ESWindow.h"
#include "ESLogger.h"
#include "ESImageCache.h"
#include "ESImageTaggerManager.h"

// Qt
#include <QApplication>
#include <QtPlugin>
#include <QThreadPool>

// Stl
#include <iostream>
#if !defined(QT_DEBUG) && defined(_MSC_VER)
#include <Windows.h>
#endif

// Plugins
#ifdef HEIF_PLUGIN_ENABLED
Q_IMPORT_PLUGIN(QHeifPlugin)
#endif
#ifdef TURBOJPEG_PLUGIN_ENABLED
Q_IMPORT_PLUGIN(QTurboJpegPlugin)
#endif

/********************************************************************************/

#if !defined(QT_DEBUG) && defined(_MSC_VER)
int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, PWSTR /*pCmdLine*/, int /*nCmdShow*/)
#else
int main(int argc, char* argv[])
#endif
{
	qInstallMessageHandler(ESLogger::qtMessageHandler);

#if !defined(QT_DEBUG) && defined(_MSC_VER)
	QApplication app(__argc, __argv);
#else
	QApplication app(argc, argv);
#endif

	app.setOrganizationName("ExifStats");
	app.setOrganizationDomain("exifstats.com");
	app.setApplicationName("ExifStats");

	QImageReader::setAllocationLimit(512);

	ESStringPool lStringPool;

	ESWindow lMainWindow;
	lMainWindow.initialize();
	lMainWindow.show();

	int lAppResult = app.exec();

	ESImageCache::getInstance().stopAndCancelAllLoadings();
#ifdef IMAGETAGGER_ENABLE
	ESImageTaggerManager::getInstance().stopAndCancelAllLoadings();
#endif // IMAGETAGGER_ENABLE

	QThreadPool::globalInstance()->waitForDone();

	return lAppResult;
}
