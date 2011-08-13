@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libfribidi_d.txt

CALL dlextract.bat libfribidi %FILES%

cd %TMP_PATH%

xcopy fribidi-0.19.2-lib\include\fribidi "%CUR_PATH%\include\fribidi" /E /Q /I /Y
copy fribidi-0.19.2-lib\lib\libfribidi.lib "%CUR_PATH%\lib\" /Y

cd %LOC_PATH%
