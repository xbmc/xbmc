@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libsamplerate_d.txt

CALL dlextract.bat libsamplerate %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

copy include\samplerate.h "%CUR_PATH%\include" /Y || EXIT /B 5
copy lib\libsamplerate-0.lib "%CUR_PATH%\lib\" /Y || EXIT /B 5
copy bin\libsamplerate-0.dll "%XBMC_PATH%\system\" /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
