@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libbluray_d.txt

CALL dlextract.bat libbluray %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

del libbluray-0.2.3-win32\how_to_build.txt || EXIT /B 5
xcopy libbluray-0.2.3-win32\* "%XBMC_PATH%\" /E /Q /I /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
