@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libbzip2_d.txt

CALL dlextract.bat libbzip2 %FILES%

cd %TMP_PATH%

copy include\* "%CUR_PATH%\include\" /Y
copy lib\bzip2.lib "%CUR_PATH%\lib\" /Y

cd %LOC_PATH%
