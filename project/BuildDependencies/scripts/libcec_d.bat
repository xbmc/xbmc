@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libcec_d.txt

CALL dlextract.bat libcec %FILES%

cd %TMP_PATH%

xcopy libcec\include\* "%CUR_PATH%\include" /E /Q /I /Y

copy libcec\libcec.dll "%XBMC_PATH%\system\."
copy libcec\pthreadVC2.dll "%XBMC_PATH%\system\."

cd %LOC_PATH%
