@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libglew_d.txt

CALL dlextract.bat libglew %FILES%

cd %TMP_PATH%

xcopy glew\include\* "%CUR_PATH%\include\" /E /Q /I /Y
copy glew\lib\*.lib "%CUR_PATH%\lib\" /Y
copy glew\bin\glew32.dll "%XBMC_PATH%\project\Win32BuildSetup\dependencies\glew32.dll"

cd %LOC_PATH%