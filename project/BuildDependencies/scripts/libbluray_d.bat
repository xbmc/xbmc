@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libbluray_d.txt

CALL dlextract.bat libbluray %FILES%

cd %TMP_PATH%

xcopy libbluray-0.2.2-win32\project\BuildDependencies\include\libbluray "%CUR_PATH%\include\libbluray" /E /Q /I /Y
copy libbluray-0.2.2-win32\system\players\dvdplayer\libbluray.dll "%XBMC_PATH%\system\players\dvdplayer\" /Y

cd %LOC_PATH%
