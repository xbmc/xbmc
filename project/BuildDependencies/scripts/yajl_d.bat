@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\yajl_d.txt

CALL dlextract.bat yajl %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

xcopy yajl_2.0.1_win32-lib\include\yajl "%CUR_PATH%\include\yajl" /E /Q /I /Y || EXIT /B 5
copy yajl_2.0.1_win32-lib\lib\yajl.lib "%CUR_PATH%\lib\" /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
