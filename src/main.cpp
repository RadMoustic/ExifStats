// ExifStats
#include "ESStringPool.h"
#include "ESWindow.h"
#include "ESLogger.h"
#include "ESImageCache.h"
#include "ESImageTaggerManager.h"
#include "ESCrashHandler.h"
#include "ESNetServer.h"

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

#if defined(_MSC_VER)
#include <windows.h>
#include <Dbghelp.h>
#endif // _MSC_VER

/********************************************************************************/

#if !defined(QT_DEBUG) && defined(_MSC_VER)
int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, PWSTR /*pCmdLine*/, int /*nCmdShow*/)
#else
int main(int argc, char* argv[])
#endif
{
	qInstallMessageHandler(ESLogger::qtMessageHandler);

#if defined(_MSC_VER)
	// Required to get valid backtraces with backward on windows
	SymInitialize(GetCurrentProcess(), nullptr, true);
	SymSetOptions(SYMOPT_LOAD_LINES);
#endif

	ESCrashHandler::init();

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

	const bool isServer = lApp.arguments().contains("-server");

	int lAppResult = 0;

	if(isServer)
	{
		ESNetServer lServer;
		if(!lServer.listen(QHostAddress::Any, 12345))
		{
			std::cerr << "Failed to start server: " << lServer.errorString().toStdString() << std::endl;
			return -1;
		}
		std::cout << "Server started on port 12345" << std::endl;

		lAppResult = lApp.exec();
	}
	else
	{
		ESWindow lMainWindow;
		lMainWindow.initialize();
		lMainWindow.show();

		lAppResult = lApp.exec();
	}

	if(!isServer)
	{
		ESImageCache::getInstance().stopAndCancelAllLoadings();
	#if defined(IMAGETAGGER_ENABLE) && !defined(EXIFSTATS_READONLY)
		ESImageTaggerManager::getInstance().stopAndCancelAllLoadings();
	#endif // defined(IMAGETAGGER_ENABLE) && !defined(EXIFSTATS_READONLY)
	}

	QThreadPool::globalInstance()->waitForDone();

	return lAppResult;
}
