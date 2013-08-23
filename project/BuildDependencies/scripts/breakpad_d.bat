@ECHO ON

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\breakpad_d.txt

CALL dlextract.bat boost %FILES%

cd %TMP_PATH%

xcopy breakpad\include\* "%CUR_PATH%\include\" /E /Q /I /Y
xcopy breakpad\lib\* "%CUR_PATH%\lib\" /E /Q /I /Y
xcopy breakpad\bin\* "%CUR_PATH%\bin\" /E /Q /I /Y

cd %LOC_PATH%