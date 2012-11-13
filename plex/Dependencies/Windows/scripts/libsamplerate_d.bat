@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libsamplerate_d.txt

CALL dlextract.bat libsamplerate %FILES%

cd %TMP_PATH%

copy include\samplerate.h "%CUR_PATH%\include" /Y
copy lib\libsamplerate-0.lib "%CUR_PATH%\lib\" /Y
copy bin\libsamplerate-0.dll "%XBMC_PATH%\system\" /Y

cd %LOC_PATH%
