@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libfribidi_d.txt

CALL dlextract.bat libfribidi %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

xcopy fribidi-0.19.2-lib\include\fribidi "%CUR_PATH%\include\fribidi" /E /Q /I /Y || EXIT /B 5
copy fribidi-0.19.2-lib\lib\libfribidi.lib "%CUR_PATH%\lib\" /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
