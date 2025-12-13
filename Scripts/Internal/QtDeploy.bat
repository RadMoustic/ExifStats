if "%BIN_DIR%" == "" (
	echo BIN_DIR not set
	pause
	exit -1
)

set OUTPUT_BIN_DIR=%GENERATED_PATH%\%BIN_DIR%

if "%QT_BIN_DIR%" == "" goto end
if not exist "%OUTPUT_BIN_DIR%" goto end

if "%QT_DEPLOY_PLUGINS%" NEQ "true" (
	xcopy /D /Y "%QT_BIN_DIR%\..\plugins\platforms\qwindows%QT_DLL_EXT%" "%OUTPUT_BIN_DIR%\platforms\"
	set WINDEPLOYQT_ARGS=%WINDEPLOYQT_ARGS% --no-plugins
)

%QT_BIN_DIR%\windeployqt.exe %QT_TARGET_OPTIONS% --no-translations --no-system-d3d-compiler --no-opengl-sw %WINDEPLOYQT_ARGS% --qmldir "%QML_DIR%" "%OUTPUT_BIN_DIR%\%SLN_NAME%.exe"

:end