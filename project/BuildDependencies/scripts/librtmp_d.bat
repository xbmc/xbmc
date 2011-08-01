@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\librtmp_d.txt

CALL dlextract.bat librtmp %FILES%

cd %TMP_PATH%

xcopy librtmp-2.4-win32\include\*.h "%CUR_PATH%\include\librtmp\" /E /Q /I /Y
xcopy librtmp-2.4-win32\lib\*.dll "%XBMC_PATH%\system\players\dvdplayer\" /Y

cd %LOC_PATH%
