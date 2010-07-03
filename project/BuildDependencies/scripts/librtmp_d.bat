@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\librtmp_d.txt

CALL dlextract.bat librtmp %FILES%

cd %TMP_PATH%

%ZIP% x -y rtmpdump-2.3.tar

xcopy rtmpdump-2.3\librtmp\*.h "%CUR_PATH%\include\librtmp\" /E /Q /I /Y
copy rtmpdump-2.3\librtmp\COPYING "%CUR_PATH%\include\librtmp\" /Y
copy rtmpdump-2.3\librtmp\librtmp.dll "%XBMC_PATH%\system\players\dvdplayer\" /Y

cd %LOC_PATH%
