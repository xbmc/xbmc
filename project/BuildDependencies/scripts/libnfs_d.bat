@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libnfs_d.txt

CALL dlextract.bat libnfs %FILES%

cd %TMP_PATH%

xcopy libnfs-20120112-win32\include\* "%CUR_PATH%\include\" /E /Q /I /Y
copy libnfs-20120112-win32\bin\libnfs.dll "%XBMC_PATH%\system\" /Y

cd %LOC_PATH%
