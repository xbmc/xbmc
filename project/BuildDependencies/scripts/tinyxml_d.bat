@ECHO ON

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\tinyxml_d.txt

CALL dlextract.bat tinyxml %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

xcopy tinyxml-2.6.2-win32\include\tinyxml\* "%CUR_PATH%\include\" /E /Q /I /Y || EXIT /B 5
copy tinyxml-2.6.2-win32\lib\* "%CUR_PATH%\lib\" /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
