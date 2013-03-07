@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\librtmp_d.txt

CALL dlextract.bat librtmp %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

xcopy librtmp-20110723-git-b627335-win32\include\*.h "%CUR_PATH%\include\librtmp\" /E /Q /I /Y || EXIT /B 5
xcopy librtmp-20110723-git-b627335-win32\lib\*.dll "%XBMC_PATH%\system\players\dvdplayer\" /E /Q /I /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
