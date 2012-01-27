@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libcurl_d.txt

CALL dlextract.bat libcurl %FILES%

cd %TMP_PATH%

rem we are using zlib1.dll from the zlib package
rem I found no reference to zlib1.dll in any curl dll
del curl-7.21.6-devel-mingw32\bin\zlib1.dll

xcopy curl-7.21.6-devel-mingw32\include\curl "%CUR_PATH%\include\curl" /E /Q /I /Y
copy curl-7.21.6-devel-mingw32\bin\*.dll "%XBMC_PATH%\system\" /Y

cd %LOC_PATH%
