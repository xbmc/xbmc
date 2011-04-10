@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\liblzo_d.txt

CALL dlextract.bat liblzo %FILES%

cd %TMP_PATH%

xcopy lzo-2.04_win32\include\lzo "%CUR_PATH%\include\lzo" /E /Q /I /Y
copy lzo-2.04_win32\lib\liblzo2.lib "%CUR_PATH%\lib\" /Y

cd %LOC_PATH%
