@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\yajl_d.txt

CALL dlextract.bat yajl %FILES%

cd %TMP_PATH%

xcopy yajl_2.0.1_win32-lib\include\yajl "%CUR_PATH%\include\yajl" /E /Q /I /Y
copy yajl_2.0.1_win32-lib\lib\yajl.lib "%CUR_PATH%\lib\" /Y

cd %LOC_PATH%
