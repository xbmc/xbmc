@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\mysqlclient_d.txt

CALL dlextract.bat mysqlclient %FILES%

cd %TMP_PATH%

xcopy mysql-connector-c-noinstall-6.0.2-win32\include\* "%CUR_PATH%\include\mysql\" /E /Q /I /Y
copy mysql-connector-c-noinstall-6.0.2-win32\lib\mysqlclient.lib "%CUR_PATH%\lib\" /Y

cd %LOC_PATH%
