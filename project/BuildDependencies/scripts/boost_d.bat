@ECHO ON

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\boost_d.txt

CALL dlextract.bat boost %FILES%

cd %TMP_PATH%

xcopy boost\include\* "%CUR_PATH%\include\" /E /Q /I /Y
xcopy boost\lib\* "%CUR_PATH%\lib\" /E /Q /I /Y

cd %LOC_PATH%