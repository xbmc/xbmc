@ECHO ON

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\harfbuzz_d.txt

CALL dlextract.bat harfbuzz %FILES%

cd %TMP_PATH%

xcopy harfbuzz-0.7.0\include\* "%CUR_PATH%\include\" /E /Q /I /Y
copy  harfbuzz-0.7.0\lib\harfbuzz.lib "%CUR_PATH%\lib\" /Y

cd %LOC_PATH%
