@echo off
rem copies additional dependencies from lib directories

set CUR_PATH=%cd%
cd "%~dp0"
md ..\..\..\system\webserver > nul
xcopy /Y ..\..\..\lib\libmicrohttpd_win32\bin\*.dll ..\..\..\system\webserver
cd "%CUR_PATH%"
set CUR_PATH=