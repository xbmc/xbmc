@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\zlib_d.txt

CALL dlextract.bat zlib %FILES%

cd %TMP_PATH%

xcopy include\* "%CUR_PATH%\include\" /E /Q /I /Y
copy lib\zlib.lib "%CUR_PATH%\lib\" /Y
copy bin\zlib1.dll "%APP_PATH%\system\" /Y

cd %LOC_PATH%
