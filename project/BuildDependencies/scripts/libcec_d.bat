@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libcec_d.txt

CALL dlextract.bat libcec %FILES%

cd %TMP_PATH%

mkdir "%CUR_PATH%\include\libcec"
xcopy libcec\include\* "%CUR_PATH%\include\libcec\." /E /Q /I /Y

copy libcec\libcec.dll "%XBMC_PATH%\system\."

cd %LOC_PATH%
