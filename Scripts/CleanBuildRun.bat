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
call Build.bat
endlocal

setlocal
call Deploy.bat
endlocal

setlocal
call Run.bat
endlocal

endlocal
