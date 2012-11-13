@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libbluray_d.txt

CALL dlextract.bat libbluray %FILES%

cd %TMP_PATH%

del libbluray-0.2.3-win32\how_to_build.txt
xcopy libbluray-0.2.3-win32\* "%XBMC_PATH%\" /E /Q /I /Y

cd %LOC_PATH%
