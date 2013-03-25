@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libglew_d.txt

CALL dlextract.bat libglew %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

xcopy glew\include\* "%CUR_PATH%\include\" /E /Q /I /Y || EXIT /B 5
copy glew\lib\*.lib "%CUR_PATH%\lib\" /Y || EXIT /B 5
copy glew\bin\glew32.dll "%XBMC_PATH%\project\Win32BuildSetup\dependencies\glew32.dll" || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1