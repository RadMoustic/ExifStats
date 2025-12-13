@echo off

call Internal\Setup.bat 

rem Copy Qt binaries
if "%QT_VERSION%" == "" (
	exit 0
)

setlocal
	set QT_DLL_EXT=d.dll
	set DEPLOY_TARGET=Debug
	set BIN_DIR=bin\Debug
	set QT_TARGET_OPTIONS=--debug --pdb
	call Internal\QtDeploy.bat
	if exist "%ROOT%\Project.Deploy.bat" (
		set OUTPUT_BIN_DIR=%GENERATED_PATH%\%BIN_DIR%
		call "%ROOT%\Project.Deploy.bat"
	)
endlocal
setlocal
	set QT_DLL_EXT=.dll
	set DEPLOY_TARGET=Release
	set BIN_DIR=bin\Release
	set QT_TARGET_OPTIONS=--release
	call Internal\QtDeploy.bat
	if exist "%ROOT%\Project.Deploy.bat" (
		set OUTPUT_BIN_DIR=%GENERATED_PATH%\%BIN_DIR%
		call "%ROOT%\Project.Deploy.bat"
	)
endlocal
setlocal
	set QT_DLL_EXT=.dll
	set DEPLOY_TARGET=RelWithDebInfo
	set BIN_DIR=bin\RelWithDebInfo
	set QT_TARGET_OPTIONS=--release
	call Internal\QtDeploy.bat
	if exist "%ROOT%\Project.Deploy.bat" (
		set OUTPUT_BIN_DIR=%GENERATED_PATH%\%BIN_DIR%
		call "%ROOT%\Project.Deploy.bat"
	)
endlocal
setlocal
	set QT_MINGW_ROOT_DIR=%QT_ROOT_DIR%\%QT_VERSION%\%QT_MINGW_DIR%
	set QT_BIN_DIR=%QT_MINGW_ROOT_DIR%\bin
	set QT_DLL_EXT=.dll
	set DEPLOY_TARGET=MingwBinDebug
	set BIN_DIR=bin\MingwBinRelease
	set QT_TARGET_OPTIONS=--release
	set OUTPUT_BIN_DIR=%GENERATED_PATH%\%BIN_DIR%
	if not exist "%OUTPUT_BIN_DIR%" goto skipMingwRelease
	call Internal\QtDeploy.bat
	xcopy /D /Y "%QT_BIN_DIR%\libgcc_s_seh-1.dll" "%OUTPUT_BIN_DIR%\"
	xcopy /D /Y "%QT_BIN_DIR%\libwinpthread-1.dll" "%OUTPUT_BIN_DIR%\"
	xcopy /D /Y "%QT_BIN_DIR%\libstdc++-6.dll" "%OUTPUT_BIN_DIR%\"
	if exist "%ROOT%\Project.Deploy.bat" (
		call "%ROOT%\Project.Deploy.bat"
	)
:skipMingwRelease
endlocal

if "%NO_PAUSE%" == "" (
	pause
)