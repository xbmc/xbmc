@ECHO ON

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\tinyxml_d.txt

CALL dlextract.bat tinyxml %FILES%

cd %TMP_PATH%

xcopy tinyxml-2.6.2-win32\include\tinyxml\* "%CUR_PATH%\include\" /E /Q /I /Y
copy tinyxml-2.6.2-win32\lib\* "%CUR_PATH%\lib\" /Y

cd %LOC_PATH%
