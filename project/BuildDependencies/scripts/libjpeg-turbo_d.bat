@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libjpeg-turbo_d.txt

CALL dlextract.bat libjpeg-turbo %FILES%

cd %TMP_PATH%

xcopy libjpeg-turbo-1.1.1-win32-1\include\* "%CUR_PATH%\include\" /E /Q /I /Y
copy libjpeg-turbo-1.1.1-win32-1\lib\jpeg-static.lib "%CUR_PATH%\lib\" /Y

cd %LOC_PATH%
