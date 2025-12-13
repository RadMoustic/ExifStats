@echo off

call Internal\Setup.bat

if "%BUILD_CONFIG%" == "" (
	SET BUILD_CONFIG=Release
)

pushd %ProgramFiles(x86)%\Microsoft Visual Studio\Installer
FOR /F "tokens=* USEBACKQ" %%F IN (`vswhere.exe -latest -property productPath -format value`) DO (
	SET DEV_ENV_PATH=%%~pF
)
"%DEV_ENV_PATH%\devenv.com" "%GENERATED_PATH%\vs\%SLN_NAME%.sln" /Build %BUILD_CONFIG%

if "%NO_PAUSE%" == "" (
	pause
)