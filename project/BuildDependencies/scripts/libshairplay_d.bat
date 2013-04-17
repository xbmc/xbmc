@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libshairplay_d.txt

CALL dlextract.bat libshairplay %FILES%

cd %TMP_PATH%

echo readme.txt > shairplay_exclude.txt

xcopy libshairplay-c159ca7-win32\* "%XBMC_PATH%\" /E /Q /I /Y /EXCLUDE:shairplay_exclude.txt

cd %LOC_PATH%
