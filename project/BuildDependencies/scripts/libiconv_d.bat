@ECHO ON

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libiconv_d.txt

CALL dlextract.bat libiconv %FILES%

cd %TMP_PATH%

xcopy libiconv-1.13.1-win32\include\* "%CUR_PATH%\include\" /E /Q /I /Y
copy libiconv-1.13.1-win32\lib\libiconv.lib "%CUR_PATH%\lib\" /Y

cd %LOC_PATH%
