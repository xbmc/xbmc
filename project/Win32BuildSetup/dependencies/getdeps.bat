@echo off
rem copies additional dependencies from lib directories

set CUR_PATH=%cd%
cd "%~dp0"
xcopy /Y ..\..\..\lib\libmicrohttpd_win32\bin\*.dll
cd "%CUR_PATH%"
set CUR_PATH=