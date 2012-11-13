@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\liboggvorbis_d.txt

CALL dlextract.bat liboggvorbis %FILES%

cd %TMP_PATH%

xcopy include\ogg "%CUR_PATH%\include\ogg" /E /Q /I /Y
xcopy include\vorbis "%CUR_PATH%\include\vorbis" /E /Q /I /Y
copy bin\ogg.dll "%XBMC_PATH%\system\cdrip\" /Y
copy bin\vorbis.dll "%XBMC_PATH%\system\cdrip\" /Y
copy bin\vorbisenc.dll "%XBMC_PATH%\system\cdrip\" /Y
copy bin\vorbisfile.dll "%XBMC_PATH%\system\players\paplayer\" /Y

cd %LOC_PATH%
