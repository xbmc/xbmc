@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\lame_enc_d.txt

CALL dlextract.bat lame_enc %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

del lame_enc-3.99.5-win32\readme.txt || EXIT /B 5
xcopy lame_enc-3.99.5-win32\* "%XBMC_PATH%\" /E /Q /I /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
