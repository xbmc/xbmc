@ECHO ON

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\harfbuzz_d.txt

CALL dlextract.bat harfbuzz %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

xcopy harfbuzz-0.7.0\include\* "%CUR_PATH%\include\" /E /Q /I /Y || EXIT /B 5
copy  harfbuzz-0.7.0\lib\harfbuzz.lib "%CUR_PATH%\lib\" /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
