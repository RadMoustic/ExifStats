@echo on
setlocal

call Internal\Setup.bat

set NO_PAUSE=nope

setlocal
call DeleteGenerated.bat
endlocal

setlocal
call CMake.bat
endlocal

setlocal
SET BUILD_CONFIG=Debug
call Build.bat
endlocal

setlocal
call Deploy.bat
endlocal

setlocal
call OpenSln.bat
endlocal

endlocal
