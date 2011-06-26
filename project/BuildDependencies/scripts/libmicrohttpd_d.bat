@ECHO ON

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libmicrohttpd_d.txt

CALL dlextract.bat libmicrohttpd %FILES%

cd %TMP_PATH%

xcopy libmicrohttpd-0.4.5-win32\include\* "%CUR_PATH%\include" /E /Q /I /Y
xcopy libmicrohttpd-0.4.5-win32\bin\*.dll "%XBMC_PATH%\system\webserver" /E /Q /I /Y
copy libmicrohttpd-0.4.5-win32\lib\libmicrohttpd.dll.lib "%CUR_PATH%\lib\" /Y

cd %LOC_PATH%
