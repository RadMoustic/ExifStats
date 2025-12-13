@echo off

call Internal\Setup.bat 

:DELETE_GENERATED
ECHO Deleting Generated ...
IF NOT EXIST %GENERATED_PATH% GOTO END
del /f /s /q %GENERATED_PATH% > nul 
rmdir /s /q %GENERATED_PATH%
IF EXIST %GENERATED_PATH% GOTO FOLDER_EXISTS
GOTO END

:FOLDER_EXISTS
echo off
echo.
echo WARNING: The folder %GENERATED_PATH% was not removed.
echo May be a program (like Visual Studio) is using this folder.
echo Please close this program and continue.
PING 1.2.2.1 -n 1 -w 1000 >NUL
echo on
GOTO DELETE_GENERATED

:END
ver > nul