@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\taglib_d.txt

CALL dlextract.bat taglib %FILES%

cd %TMP_PATH%

del taglib-1.8-win32\readme.txt
xcopy taglib-1.8-win32\* "%XBMC_PATH%\" /E /Q /I /Y

cd %LOC_PATH%
