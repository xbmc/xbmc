@ECHO OFF

rem create directories
IF NOT EXIST "%XBMC_PATH%\system\players\paplayer" md "%XBMC_PATH%\system\players\paplayer"
IF NOT EXIST "%XBMC_PATH%\project\VS2010Express\XBMC\Debug (DirectX)" md "%XBMC_PATH%\project\VS2010Express\XBMC\Debug (DirectX)"
IF NOT EXIST "%XBMC_PATH%\system\webserver" md "%XBMC_PATH%\system\webserver"