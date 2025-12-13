if "%QT_ROOT_DIR%" == "" (
	echo Project.Setup.bat does not set the variable QT_ROOT_DIR.
	pause
	exit -1
)

if "%QT_VERSION%" == "" (
	echo Project.Setup.bat does not set the variable QT_VERSION.
	pause
	exit -1
)

if "%QT_MSVC_DIR%" == "" (
	echo Project.Setup.bat does not set the variable QT_MSVC_DIR.
	pause
	exit -1
)

if "%QT_MSVC_ROOT_DIR%" == "" (
	if "%QT_STATIC%" == "true" (
		if "%QT_STATIC_DIR%" == "" (
			echo Project.Setup.bat does not set the variable QT_STATIC_DIR.
			pause
			exit -1
		)
		set QT_MSVC_ROOT_DIR=%QT_ROOT_DIR%\%QT_VERSION%\%QT_STATIC_DIR%
	) else (
		set QT_MSVC_ROOT_DIR=%QT_ROOT_DIR%\%QT_VERSION%\%QT_MSVC_DIR%
	)
)

set QT_BIN_DIR=%QT_MSVC_ROOT_DIR%\bin

if not exist "%QT_MSVC_ROOT_DIR%" (
	echo Qt root directory missing: %QT_MSVC_ROOT_DIR%
	pause
	exit -1
)