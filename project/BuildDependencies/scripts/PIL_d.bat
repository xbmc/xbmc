@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\PIL_d.txt
SET OUTDIR="%XBMC_PATH%\addons\script.module.pil\lib\PIL"

CALL dlextract.bat PIL %FILES% || EXIT /B 2

IF EXIST %OUTDIR% rmdir %OUTDIR% /S /Q || EXIT /B 6

cd %TMP_PATH% || EXIT /B 1

xcopy PLATLIB\PIL %OUTDIR% /E /Q /I /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
