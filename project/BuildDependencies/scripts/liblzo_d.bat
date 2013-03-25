@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\liblzo_d.txt

CALL dlextract.bat liblzo %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

xcopy lzo-2.04_win32\include\lzo "%CUR_PATH%\include\lzo" /E /Q /I /Y || EXIT /B 5
copy lzo-2.04_win32\lib\liblzo2.lib "%CUR_PATH%\lib\" /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
