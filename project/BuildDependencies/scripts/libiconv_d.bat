@ECHO ON

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libiconv_d.txt

CALL dlextract.bat libiconv %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

xcopy libiconv-1.13.1-win32\include\* "%CUR_PATH%\include\" /E /Q /I /Y || EXIT /B 5
copy libiconv-1.13.1-win32\lib\libiconv.lib "%CUR_PATH%\lib\" /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
