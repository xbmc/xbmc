@ECHO OFF
rem libxslt depends on libxml2

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libxslt_d.txt

CALL dlextract.bat libxslt %FILES%

cd %TMP_PATH%

echo readme.txt > libxslt_exclude.txt

xcopy libxslt-1.1.26-win32\* "%XBMC_PATH%\" /E /Q /I /Y /EXCLUDE:libxslt_exclude.txt

cd %LOC_PATH%
