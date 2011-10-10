@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libcec_d.txt

CALL dlextract.bat libcec %FILES%

cd %TMP_PATH%

xcopy libcec\include\* "%CUR_PATH%\include\libcec" /E /Q /I /Y

copy libcec\libcec.dll "%CUR_PATH%\lib\."
copy libcec\pthreadVC2.dll "%CUR_PATH%\lib\."
copy libcec\libcec.lib "%CUR_PATH%\lib\."
copy libcec\libcec.dll "%XBMC_PATH%\project\Win32BuildSetup\dependencies\libcec.dll"
copy libcec\pthreadVC2.dll "%XBMC_PATH%\project\Win32BuildSetup\dependencies\pthreadVC2.dll"

cd %LOC_PATH%
