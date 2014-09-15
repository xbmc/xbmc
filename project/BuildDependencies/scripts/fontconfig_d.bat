@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\fontconfig_d.txt

CALL dlextract.bat fontconfig %FILES%

cd %TMP_PATH%

xcopy include\fontconfig "%CUR_PATH%\include\fontconfig" /E /Q /I /Y
copy lib\fontconfig.lib "%CUR_PATH%\lib\" /Y
rem libfontconfig-1.dll requires libexpat-1.dll which is copied by libexpat_d.bat
copy bin\libfontconfig-1.dll "%APP_PATH%\system\players\dvdplayer\"
copy freetype-2.4.6-1-win32\bin\freetype6.dll "%APP_PATH%\system\players\dvdplayer\"

cd %LOC_PATH%
