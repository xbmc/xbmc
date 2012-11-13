@ECHO ON

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\freetype_d.txt

CALL dlextract.bat freetype %FILES%

cd %TMP_PATH%

xcopy freetype-2.4.6-win32-1\include\* "%CUR_PATH%\include\" /E /Q /I /Y
copy freetype-2.4.6-win32-1\lib\freetype246MT.lib "%CUR_PATH%\lib\" /Y

cd %LOC_PATH%
