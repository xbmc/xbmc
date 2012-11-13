@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\PIL_d.txt
SET OUTDIR="%XBMC_PATH%\addons\script.module.pil\lib\PIL"

CALL dlextract.bat PIL %FILES%

IF EXIST %OUTDIR% rmdir %OUTDIR% /S /Q

cd %TMP_PATH%

xcopy PLATLIB\PIL %OUTDIR% /E /Q /I /Y

cd %LOC_PATH%
