@ECHO OFF

rem copy webserver dlls
IF EXIST "%XBMC_PATH%\system\webserver" rmdir "%XBMC_PATH%\system\webserver" /S /Q
xcopy "%XBMC_PATH%\lib\win32\libmicrohttpd_win32\bin\*.dll" "%XBMC_PATH%\system\webserver" /E /Q /I /Y

rem create directories
IF NOT EXIST "%XBMC_PATH%\system\players\paplayer" md "%XBMC_PATH%\system\players\paplayer"
