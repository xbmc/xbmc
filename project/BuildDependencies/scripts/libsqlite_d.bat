@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libsqlite_d.txt

CALL dlextract.bat libsqlite %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

echo readme.txt > sqlite_exclude.txt || EXIT /B 12

xcopy sqlite-3.7.12.1-win32\* "%XBMC_PATH%\" /E /Q /I /Y /EXCLUDE:sqlite_exclude.txt || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
