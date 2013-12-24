@ECHO ON
rem libxml2 is needed by libxslt

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libxml2_d.txt

CALL dlextract.bat libxml2 %FILES%

cd %TMP_PATH%

echo readme.txt > libxml2_exclude.txt

xcopy libxml2-2.7.8-win32\* "%XBMC_PATH%\" /E /Q /I /Y /EXCLUDE:libxml2_exclude.txt

cd %LOC_PATH%
