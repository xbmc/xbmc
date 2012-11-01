@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\swig_d.txt

CALL dlextract.bat swig %FILES%

cd %TMP_PATH%

xcopy swig-2.0.7-win32\* "%CUR_PATH%\bin\swig" /E /Q /I /Y

cd %LOC_PATH%
