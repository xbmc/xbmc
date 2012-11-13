@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libsqlite_d.txt

CALL dlextract.bat libsqlite %FILES%

cd %TMP_PATH%

echo readme.txt > sqlite_exclude.txt

xcopy sqlite-3.7.12.1-win32\* "%XBMC_PATH%\" /E /Q /I /Y /EXCLUDE:sqlite_exclude.txt

cd %LOC_PATH%
