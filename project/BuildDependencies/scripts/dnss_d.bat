@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\dnssd_d.txt

CALL dlextract.bat dnssd %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

del dnssd-379.32.1-win32\readme.txt || EXIT /B 6
xcopy dnssd-379.32.1-win32\* "%XBMC_PATH%\" /E /Q /I /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
