@ECHO OFF

rem copy webserver dlls
IF EXIST "%XBMC_PATH%\system\webserver" rmdir "%XBMC_PATH%\system\webserver" /S /Q
xcopy "%XBMC_PATH%\lib\libmicrohttpd_win32\bin\*.dll" "%XBMC_PATH%\system\webserver" /E /Q /I /Y

rem copy python lib
IF EXIST "%XBMC_PATH%\system\python\Lib" rmdir "%XBMC_PATH%\system\python\Lib" /S /Q
echo .svn>py_exclude.txt
echo test>>py_exclude.txt
echo plat->>py_exclude.txt
xcopy "%XBMC_PATH%\xbmc\lib\libPython\Python\Lib" "%XBMC_PATH%\system\python\Lib" /E /Q /I /Y /EXCLUDE:py_exclude.txt > NUL
del py_exclude.txt