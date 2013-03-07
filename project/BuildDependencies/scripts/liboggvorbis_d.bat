@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\liboggvorbis_d.txt

CALL dlextract.bat liboggvorbis %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

xcopy include\ogg "%CUR_PATH%\include\ogg" /E /Q /I /Y || EXIT /B 5
xcopy include\vorbis "%CUR_PATH%\include\vorbis" /E /Q /I /Y || EXIT /B 5
copy bin\ogg.dll "%XBMC_PATH%\system\cdrip\" /Y || EXIT /B 5
copy bin\vorbis.dll "%XBMC_PATH%\system\cdrip\" /Y || EXIT /B 5
copy bin\vorbisenc.dll "%XBMC_PATH%\system\cdrip\" /Y || EXIT /B 5
copy bin\vorbisfile.dll "%XBMC_PATH%\system\players\paplayer\" /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
