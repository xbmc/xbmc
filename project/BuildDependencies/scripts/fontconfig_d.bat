@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\fontconfig_d.txt

CALL dlextract.bat fontconfig %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

xcopy include\fontconfig "%CUR_PATH%\include\fontconfig" /E /Q /I /Y || EXIT /B 5
copy lib\fontconfig.lib "%CUR_PATH%\lib\" /Y || EXIT /B 5
rem libfontconfig-1.dll requires libexpat-1.dll which is copied by libexpat_d.bat
copy bin\libfontconfig-1.dll "%XBMC_PATH%\system\players\dvdplayer\" || EXIT /B 5
copy freetype-2.4.6-1-win32\bin\freetype6.dll "%XBMC_PATH%\system\players\dvdplayer\" || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
