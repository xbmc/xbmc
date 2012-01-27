@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libsqlite_d.txt

CALL dlextract.bat libsqlite %FILES%

cd %TMP_PATH%

xcopy include "%CUR_PATH%\include" /E /Q /I /Y
copy lib\sqlite3.lib "%CUR_PATH%\lib\" /Y
copy bin\sqlite3.dll "%XBMC_PATH%\system\" /Y

cd %LOC_PATH%
