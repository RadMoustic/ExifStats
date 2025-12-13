@echo off

call Internal\Setup.bat 

if "%VS_VERSION%" NEQ "" ( goto VSVERSIONSET )
reg query "HKEY_CLASSES_ROOT\VisualStudio.DTE.17.0" >> nul 2>&1
if "%ERRORLEVEL%" == "0" (
	set VS_VERSION=Visual Studio 17 2022
) else (
	set VS_VERSION=Visual Studio 16 2019
)
:VSVERSIONSET

if not exist %VS_PATH% mkdir %VS_PATH%

cd %VS_PATH%
cmake %ROOT% -G "%VS_VERSION%"

cd %ROOT%

if "%NO_PAUSE%" == "" (
	pause
)