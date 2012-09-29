@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\doxygen_d.txt

CALL dlextract.bat doxygen %FILES%

cd %TMP_PATH%

xcopy doxygen-1.8.2-win32\* "%XBMC_PATH%\" /E /Q /I /Y

cd %LOC_PATH%
