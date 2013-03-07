@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\doxygen_d.txt

CALL dlextract.bat doxygen %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

xcopy doxygen-1.8.2-win32\* "%XBMC_PATH%\" /E /Q /I /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
