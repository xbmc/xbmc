@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libsdl_d.txt

CALL dlextract.bat libsdl %FILES%

cd %TMP_PATH%

xcopy SDL-1.2.14\include\* "%CUR_PATH%\include\SDL\" /E /Q /I /Y
copy SDL-1.2.14\lib\SDL.lib "%CUR_PATH%\lib\SDL.lib" /Y
copy SDL-1.2.14\lib\SDL.dll "%XBMC_PATH%\project\Win32BuildSetup\dependencies\SDL.dll"

cd %LOC_PATH%