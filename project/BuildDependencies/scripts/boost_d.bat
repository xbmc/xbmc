@ECHO ON

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\boost_d.txt

CALL dlextract.bat boost %FILES%

cd %TMP_PATH%

xcopy boost-1_46_1-xbmc-win32\include\* "%CUR_PATH%\include\" /E /Q /I /Y
xcopy boost-1_46_1-xbmc-win32\lib\* "%CUR_PATH%\lib\" /E /Q /I /Y

cd %LOC_PATH%