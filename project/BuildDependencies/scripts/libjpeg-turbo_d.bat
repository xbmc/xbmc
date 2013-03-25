@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libjpeg-turbo_d.txt

CALL dlextract.bat libjpeg-turbo %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

xcopy libjpeg-turbo-1.2.0-win32\project\BuildDependencies\include\* "%CUR_PATH%\include\" /E /Q /I /Y || EXIT /B 5
xcopy libjpeg-turbo-1.2.0-win32\project\BuildDependencies\lib\*.lib "%CUR_PATH%\lib\" /E /Q /I /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
