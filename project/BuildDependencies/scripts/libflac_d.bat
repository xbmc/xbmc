@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libflac_d.txt

CALL dlextract.bat libflac %FILES%

cd %TMP_PATH%

xcopy include\FLAC "%CUR_PATH%\include\FLAC" /E /Q /I /Y
copy lib\libFLAC.dll "%APP_PATH%\system\players\paplayer\" /Y

IF EXIST "include\FLAC++" rmdir "include\FLAC++" /S /Q
IF EXIST "include\ogg" rmdir "include\ogg" /S /Q

cd %LOC_PATH%
