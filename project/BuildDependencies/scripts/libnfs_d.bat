@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libnfs_d.txt

CALL dlextract.bat libnfs %FILES%

cd %TMP_PATH%

xcopy libnfs-1.3.0-win32\project\BuildDependencies\include\* "%CUR_PATH%\include\" /E /Q /I /Y
copy libnfs-1.3.0-win32\system\libnfs.dll "%XBMC_PATH%\system\" /Y

cd %LOC_PATH%
