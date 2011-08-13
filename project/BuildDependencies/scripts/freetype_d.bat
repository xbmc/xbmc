@ECHO ON

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\freetype_d.txt

CALL dlextract.bat freetype %FILES%

cd %TMP_PATH%

xcopy freetype-2.4.4-win32\include\* "%CUR_PATH%\include\" /E /Q /I /Y
copy freetype-2.4.4-win32\lib\freetype244MT.lib "%CUR_PATH%\lib\" /Y

cd %LOC_PATH%
