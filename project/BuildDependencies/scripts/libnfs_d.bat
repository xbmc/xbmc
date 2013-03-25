@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libnfs_d.txt

CALL dlextract.bat libnfs %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

xcopy libnfs-1.3.0-win32\project\BuildDependencies\include\* "%CUR_PATH%\include\" /E /Q /I /Y || EXIT /B 5
copy libnfs-1.3.0-win32\system\libnfs.dll "%XBMC_PATH%\system\" /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
