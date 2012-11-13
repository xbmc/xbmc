@ECHO OFF

IF EXIST "%XBMC_PATH%\system\webserver" rmdir "%XBMC_PATH%\system\webserver" /S /Q
IF EXIST "%XBMC_PATH%\system\airplay" rmdir "%XBMC_PATH%\system\airplay" /S /Q

rem create directories
IF NOT EXIST "%XBMC_PATH%\system\players\paplayer" md "%XBMC_PATH%\system\players\paplayer"
IF NOT EXIST "%XBMC_PATH%\project\VS2010Express\XBMC\Debug (DirectX)" md "%XBMC_PATH%\project\VS2010Express\XBMC\Debug (DirectX)"
IF NOT EXIST "%XBMC_PATH%\system\webserver" md "%XBMC_PATH%\system\webserver"
IF NOT EXIST "%XBMC_PATH%\system\airplay" md "%XBMC_PATH%\system\airplay"
IF NOT EXIST "%XBMC_PATH%\project\Win32BuildSetup\dependencies" md "%XBMC_PATH%\project\Win32BuildSetup\dependencies"
IF NOT EXIST "%XBMC_PATH%\system\cdrip" md "%XBMC_PATH%\system\cdrip"