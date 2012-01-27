@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\dnssd_d.txt

CALL dlextract.bat dnssd %FILES%

cd %TMP_PATH%

copy dnssd-320.5.1-win32\include\dns_sd.h "%CUR_PATH%\include\" /Y
copy dnssd-320.5.1-win32\lib\dnssd.lib "%CUR_PATH%\lib\" /Y
copy dnssd-320.5.1-win32\bin\dnssd.dll "%XBMC_PATH%\system\" /Y

cd %LOC_PATH%
