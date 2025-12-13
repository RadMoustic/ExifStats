if "%QT_ROOT_DIR%" == "" (
	echo QT_ROOT_DIR not set. Create a User.Setup.bat next to the Project.Setup.bat with QT_ROOT_DIR=c:\Qt
	pause
	exit -1
)

set SLN_NAME=ExifStats
set QT_VERSION=6.6.2
set QT_MSVC_DIR=msvc2019_64
set QT_STATIC_DIR=Src\BuildStatic
set QML_DIR=%ROOT%\rc\Qml
set QT_DEPLOY_PLUGINS=true
set RUN_ARGS=
