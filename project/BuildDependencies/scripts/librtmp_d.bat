@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\librtmp_d.txt

CALL dlextract.bat librtmp %FILES%

cd %TMP_PATH%

xcopy librtmp-20110723-git-b627335-win32\include\*.h "%CUR_PATH%\include\librtmp\" /E /Q /I /Y
xcopy librtmp-20110723-git-b627335-win32\lib\*.dll "%APP_PATH%\system\players\dvdplayer\" /E /Q /I /Y

cd %LOC_PATH%
