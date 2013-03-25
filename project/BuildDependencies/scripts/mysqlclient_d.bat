@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\mysqlclient_d.txt

CALL dlextract.bat mysqlclient %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

xcopy mysql-connector-c-noinstall-6.0.2-win32\include\* "%CUR_PATH%\include\mysql\" /E /Q /I /Y || EXIT /B 5
copy mysql-connector-c-noinstall-6.0.2-win32\lib\mysqlclient.lib "%CUR_PATH%\lib\" /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
