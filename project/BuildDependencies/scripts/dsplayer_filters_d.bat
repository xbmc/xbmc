@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\dsplayer_filters_d.txt
SET OUTDIR="%XBMC_PATH%\system\players\dsplayer"

CALL dlextract.bat dsplayer_filters %FILES%

cd %TMP_PATH%

del "%OUTDIR%\*.ax"

xcopy standalone_filters-1.5.3.3757_x86\*.ax "%OUTDIR%" /E /Q /I /Y

cd %LOC_PATH%
