@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libcurl_d.txt

CALL dlextract.bat libcurl %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

rem we are using zlib1.dll from the zlib package
rem I found no reference to zlib1.dll in any curl dll
del curl-7.21.6-devel-mingw32\bin\zlib1.dll || EXIT /B 6

xcopy curl-7.21.6-devel-mingw32\include\curl "%CUR_PATH%\include\curl" /E /Q /I /Y || EXIT /B 5
copy curl-7.21.6-devel-mingw32\bin\*.dll "%XBMC_PATH%\system\" /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
