@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libshairplay_d.txt

CALL dlextract.bat libnfs %FILES%

cd %TMP_PATH%

xcopy libshairplay-d689c6-win32\* "%XBMC_PATH%\" /E /Q /I /Y

cd %LOC_PATH%
