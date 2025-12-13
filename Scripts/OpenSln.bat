@echo off

call Internal\Setup.bat 

start %GENERATED_PATH%\vs\%SLN_NAME%.sln 