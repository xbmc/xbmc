@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libflac_d.txt

CALL dlextract.bat libflac %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

xcopy include\FLAC "%CUR_PATH%\include\FLAC" /E /Q /I /Y || EXIT /B 5
copy lib\libFLAC.dll "%XBMC_PATH%\system\players\paplayer\" /Y || EXIT /B 5

IF EXIST "include\FLAC++" rmdir "include\FLAC++" /S /Q || EXIT /B 6
IF EXIST "include\ogg" rmdir "include\ogg" /S /Q || EXIT /B 6

cd %LOC_PATH% || EXIT /B 1
