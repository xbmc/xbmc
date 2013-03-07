@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libcec_d.txt

CALL dlextract.bat libcec %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

mkdir "%CUR_PATH%\include\libcec" || EXIT /B 3
xcopy libcec\include\* "%CUR_PATH%\include\libcec\." /E /Q /I /Y || EXIT /B 5

copy libcec\libcec.dll "%XBMC_PATH%\system\." || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
