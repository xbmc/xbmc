@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libshairplay_d.txt

CALL dlextract.bat libshairplay %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

echo readme.txt > shairplay_exclude.txt || EXIT /B 12

xcopy libshairplay-495f02-win32\* "%XBMC_PATH%\" /E /Q /I /Y /EXCLUDE:shairplay_exclude.txt || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
