@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\pysqlite_d.txt
SET OUTDIR="%XBMC_PATH%\addons\script.module.pysqlite\lib\pysqlite2"

CALL dlextract.bat pysqlite %FILES%

IF EXIST %OUTDIR% rmdir %OUTDIR% /S /Q

cd %TMP_PATH%

xcopy PLATLIB\pysqlite2 %OUTDIR% /E /Q /I /Y

cd %LOC_PATH%
