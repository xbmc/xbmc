@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\taglib_d.txt

CALL dlextract.bat taglib %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

del taglib-1.8-win32\readme.txt || EXIT /B 6
xcopy taglib-1.8-win32\* "%XBMC_PATH%\" /E /Q /I /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
