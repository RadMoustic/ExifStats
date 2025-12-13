pushd %~dp0\..\..
set ROOT=%cd%
popd

if not exist "%ROOT%\Project.Setup.bat" (
	echo Project.Setup.bat not found. Please create it in the project root folder.
	pause
	exit -1
)

if not exist %ROOT%\User.Setup.bat goto noUserSetup
call %ROOT%\User.Setup.bat
:noUserSetup

call %ROOT%\Project.Setup.bat

if not exist %ROOT%\User.Setup.bat goto noUserSetup
call %ROOT%\User.Setup.bat
:noUserSetup

rem Qt config
if "%QT_VERSION%" == "" goto noQt
call %~dp0\Setup.Qt.bat
:noQt

set GENERATED_PATH=%ROOT%\generated
set VS_PATH=%GENERATED_PATH%\vs

if "%SLN_NAME%" == "" (
	echo SLN_NAME not set. Please create your Project.Setup.Bat and use it.
	pause
	exit -1
)
