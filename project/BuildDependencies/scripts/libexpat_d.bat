@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libexpat_d.txt

CALL dlextract.bat libexpat %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

copy include\* "%CUR_PATH%\include\" /Y || EXIT /B 5
copy lib\expat.lib "%CUR_PATH%\lib\libexpat.lib" /Y || EXIT /B 
copy bin\libexpat-1.dll "%XBMC_PATH%\system\libexpat.dll" || EXIT /B 5
rem libexpat-1.dll for libfontconfig-1.dll which is needed for libass.dll
copy bin\libexpat-1.dll "%XBMC_PATH%\system\players\dvdplayer\" || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
