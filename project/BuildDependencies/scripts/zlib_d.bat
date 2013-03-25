@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\zlib_d.txt

CALL dlextract.bat zlib %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

xcopy include\* "%CUR_PATH%\include\" /E /Q /I /Y || EXIT /B 5
copy lib\zlib.lib "%CUR_PATH%\lib\" /Y || EXIT /B 5
copy bin\zlib1.dll "%XBMC_PATH%\system\" /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
