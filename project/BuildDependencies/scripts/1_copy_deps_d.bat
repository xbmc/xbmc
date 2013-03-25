@ECHO OFF

IF EXIST "%XBMC_PATH%\system\webserver" rmdir "%XBMC_PATH%\system\webserver" /S /Q || EXIT /B 6
IF EXIST "%XBMC_PATH%\system\airplay" rmdir "%XBMC_PATH%\system\airplay" /S /Q || EXIT /B 6

rem create directories
IF NOT EXIST "%XBMC_PATH%\system\players\paplayer" md "%XBMC_PATH%\system\players\paplayer" || EXIT /B 3
IF NOT EXIST "%XBMC_PATH%\project\VS2010Express\XBMC\Debug (DirectX)" md "%XBMC_PATH%\project\VS2010Express\XBMC\Debug (DirectX)" || EXIT /B 3
IF NOT EXIST "%XBMC_PATH%\system\webserver" md "%XBMC_PATH%\system\webserver" || EXIT /B 3
IF NOT EXIST "%XBMC_PATH%\system\airplay" md "%XBMC_PATH%\system\airplay" || EXIT /B 3
IF NOT EXIST "%XBMC_PATH%\project\Win32BuildSetup\dependencies" md "%XBMC_PATH%\project\Win32BuildSetup\dependencies" || EXIT /B 3
IF NOT EXIST "%XBMC_PATH%\system\cdrip" md "%XBMC_PATH%\system\cdrip" || EXIT /B 3