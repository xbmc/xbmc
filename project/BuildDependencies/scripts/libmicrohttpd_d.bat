@ECHO ON

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libmicrohttpd_d.txt

CALL dlextract.bat libmicrohttpd %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

xcopy libmicrohttpd-0.4.5-win32\include\* "%CUR_PATH%\include" /E /Q /I /Y || EXIT /B 5
xcopy libmicrohttpd-0.4.5-win32\bin\*.dll "%XBMC_PATH%\system\webserver" /E /Q /I /Y || EXIT /B 5
copy libmicrohttpd-0.4.5-win32\lib\libmicrohttpd.dll.lib "%CUR_PATH%\lib\" /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
