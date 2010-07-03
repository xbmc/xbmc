@echo off
rem copies additional dependencies from lib directories

set CUR_PATH=%cd%
cd "%~dp0"
md ..\..\..\system\webserver > nul
xcopy /Y ..\..\..\lib\libmicrohttpd_win32\bin\*.dll ..\..\..\system\webserver

rem copy python lib
IF EXIST ..\..\..\system\python\Lib rmdir ..\..\..\system\python\Lib /S /Q
echo .svn>py_exclude.txt
echo test>>py_exclude.txt
echo plat->>py_exclude.txt
md ..\..\..\system\python\Lib
xcopy ..\..\..\xbmc\lib\libPython\Python\Lib ..\..\..\system\python\Lib /E /Q /I /Y /EXCLUDE:py_exclude.txt > NUL
del py_exclude.txt
cd "%CUR_PATH%"
set CUR_PATH=