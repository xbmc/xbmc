@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libbzip2_d.txt

CALL dlextract.bat libbzip2 %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

copy include\bzlib.h "%CUR_PATH%\include\" /Y || EXIT /B 5
copy lib\bzip2.lib "%CUR_PATH%\lib\" /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
