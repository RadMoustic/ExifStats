@echo off

call Internal\Setup.bat

start %GENERATED_PATH%\bin\Release\%SLN_NAME%.exe %RUN_ARGS%
