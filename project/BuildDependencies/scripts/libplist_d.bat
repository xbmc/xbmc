@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libplist_d.txt

CALL dlextract.bat libplist %FILES%

cd %TMP_PATH%

xcopy libplist-1.7-win32-2\include\* "%CUR_PATH%\include\" /E /Q /I /Y
xcopy libplist-1.7-win32-2\bin\* "%XBMC_PATH%\system\" /E /Q /I /Y

cd %LOC_PATH%
