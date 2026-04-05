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

	QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QSurfaceFormat lFormat;
#ifdef Q_OS_ANDROID
    lFormat.setVersion(3, 2);
    lFormat.setRenderableType(QSurfaceFormat::OpenGLES);
#endif // Q_OS_ANDROID
#ifdef QT_DEBUG
    lFormat.setOption(QSurfaceFormat::DebugContext);
#endif // QT_DEBUG
    QSurfaceFormat::setDefaultFormat(lFormat);

#if !defined(QT_DEBUG) && defined(_MSC_VER)
	QApplication lApp(__argc, __argv);
#else
	QApplication lApp(argc, argv);
#endif

	lApp.setOrganizationName("ExifStats");
	lApp.setOrganizationDomain("github.com/RadMoustic/ExifStats");
	lApp.setApplicationName("ExifStats");
	lApp.setWindowIcon(QIcon(":/Images/ExifStats.ico"));

	QImageReader::setAllocationLimit(512);

	ESStringPool lStringPool;

	ESWindow lMainWindow;
	lMainWindow.initialize();
	lMainWindow.show();

	int lAppResult = lApp.exec();

	ESImageCache::getInstance().stopAndCancelAllLoadings();
#ifdef IMAGETAGGER_ENABLE
	ESImageTaggerManager::getInstance().stopAndCancelAllLoadings();
#endif // IMAGETAGGER_ENABLE

	QThreadPool::globalInstance()->waitForDone();

	return lAppResult;
}
