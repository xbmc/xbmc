@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\dnssd_d.txt

CALL dlextract.bat dnssd %FILES%

cd %TMP_PATH%

del dnssd-379.32.1-win32\readme.txt
xcopy dnssd-379.32.1-win32\* "%XBMC_PATH%\" /E /Q /I /Y

cd %LOC_PATH%
