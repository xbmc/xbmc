@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libsdl_d.txt

CALL dlextract.bat libsdl %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

xcopy SDL-1.2.14\include\* "%CUR_PATH%\include\SDL\" /E /Q /I /Y || EXIT /B 5
copy SDL-1.2.14\lib\SDL.lib "%CUR_PATH%\lib\SDL.lib" /Y || EXIT /B 5

copy SDL-1.2.14\lib\SDL.dll "%XBMC_PATH%\tools\TexturePacker\SDL.dll" || EXIT /B 5
copy SDL_image-1.2.10\include\SDL_image.h "%CUR_PATH%\include\SDL\" || EXIT /B 5
copy SDL_image-1.2.10\lib\*.dll "%XBMC_PATH%\tools\TexturePacker\" || EXIT /B 5
copy SDL_image-1.2.10\lib\SDL_image.lib "%CUR_PATH%\lib\SDL_image.lib" /Y || EXIT /B 5


cd %LOC_PATH% || EXIT /B 1