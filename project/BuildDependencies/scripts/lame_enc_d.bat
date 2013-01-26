@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\lame_enc_d.txt

CALL dlextract.bat lame_enc %FILES%

cd %TMP_PATH%

del lame_enc-3.99.5-win32\readme.txt
xcopy lame_enc-3.99.5-win32\* "%XBMC_PATH%\" /E /Q /I /Y

cd %LOC_PATH%
