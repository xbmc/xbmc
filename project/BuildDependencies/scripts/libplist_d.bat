@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libplist_d.txt

CALL dlextract.bat libplist %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

xcopy libplist-1.7-win32-2\include\* "%CUR_PATH%\include\" /E /Q /I /Y || EXIT /B 5
xcopy libplist-1.7-win32-2\bin\* "%XBMC_PATH%\system\" /E /Q /I /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
