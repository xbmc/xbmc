@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libexpat_d.txt

CALL dlextract.bat libexpat %FILES%

cd %TMP_PATH%

copy include\* "%CUR_PATH%\include\" /Y
copy lib\expat.lib "%CUR_PATH%\lib\libexpat.lib" /Y
copy bin\libexpat-1.dll "%XBMC_PATH%\system\libexpat.dll"
rem libexpat-1.dll for libfontconfig-1.dll which is needed for libass.dll
copy bin\libexpat-1.dll "%XBMC_PATH%\system\players\dvdplayer\"

cd %LOC_PATH%
