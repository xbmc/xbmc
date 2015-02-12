@ECHO OFF

IF EXIST "%APP_PATH%\system\webserver" rmdir "%APP_PATH%\system\webserver" /S /Q
IF EXIST "%APP_PATH%\system\airplay" rmdir "%APP_PATH%\system\airplay" /S /Q

rem create directories
IF NOT EXIST "%APP_PATH%\system\players\paplayer" md "%APP_PATH%\system\players\paplayer"
IF NOT EXIST "%APP_PATH%\project\VS2010Express\Application\Debug" md "%APP_PATH%\project\VS2010Express\Application\Debug"
IF NOT EXIST "%APP_PATH%\system\airplay" md "%APP_PATH%\system\airplay"
IF NOT EXIST "%APP_PATH%\project\Win32BuildSetup\dependencies" md "%APP_PATH%\project\Win32BuildSetup\dependencies"
IF NOT EXIST "%APP_PATH%\system\cdrip" md "%APP_PATH%\system\cdrip"