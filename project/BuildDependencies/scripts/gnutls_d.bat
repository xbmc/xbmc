@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\gnutls_d.txt

CALL dlextract.bat dnssd %FILES%

cd %TMP_PATH%

del gnutls-3.2.3-win32\readme.txt
xcopy gnutls-3.2.3-win32\* "%XBMC_PATH%\" /E /Q /I /Y

cd %LOC_PATH%
