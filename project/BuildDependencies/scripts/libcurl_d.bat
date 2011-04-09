@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libcurl_d.txt

CALL dlextract.bat libcurl %FILES%

cd %TMP_PATH%

del curl-7.21.1-devel-mingw32\bin\zlib1.dll

xcopy curl-7.21.1-devel-mingw32\include\curl "%CUR_PATH%\include\curl" /E /Q /I /Y
copy curl-7.21.1-devel-mingw32\bin\*.dll "%XBMC_PATH%\system\" /Y

cd %LOC_PATH%
