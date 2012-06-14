@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libshairplay_d.txt

CALL dlextract.bat libnfs %FILES%

cd %TMP_PATH%

echo readme.txt > shairplay_exclude.txt

xcopy libshairplay-d689c6-win32\* "%XBMC_PATH%\" /E /Q /I /Y /EXCLUDE:shairplay_exclude.txt

cd %LOC_PATH%
