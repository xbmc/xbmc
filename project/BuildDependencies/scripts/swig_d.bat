@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\swig_d.txt

CALL dlextract.bat swig %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

xcopy swig-2.0.7-win32\* "%CUR_PATH%\bin\swig" /E /Q /I /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
