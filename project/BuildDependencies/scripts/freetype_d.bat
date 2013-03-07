@ECHO ON

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\freetype_d.txt

CALL dlextract.bat freetype %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

xcopy freetype-2.4.6-win32-1\include\* "%CUR_PATH%\include\" /E /Q /I /Y || EXIT /B 5
copy freetype-2.4.6-win32-1\lib\freetype246MT.lib "%CUR_PATH%\lib\" /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
