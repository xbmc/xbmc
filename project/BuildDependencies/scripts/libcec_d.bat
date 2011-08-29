@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libcec_d.txt

CALL dlextract.bat libcec %FILES%

cd %TMP_PATH%

xcopy libcec\include\* "%CUR_PATH%\include\libcec" /E /Q /I /Y
xcopy libcec\*.dll "%CUR_PATH%\lib\." /Y
xcopy libcec\*.lib "%CUR_PATH%\lib\." /Y

cd %LOC_PATH%