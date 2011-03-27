@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libflac_d.txt

CALL dlextract.bat libflac %FILES%

cd %TMP_PATH%

xcopy include\FLAC "%CUR_PATH%\include\FLAC" /E /Q /I /Y
copy lib\libFLAC.dll "%XBMC_PATH%\system\players\paplayer\" /Y

cd %LOC_PATH%
