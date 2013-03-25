@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libssh_d.txt

CALL dlextract.bat libssh %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

xcopy include\libssh "%CUR_PATH%\include\libssh" /E /Q /I /Y || EXIT /B 5
copy lib\ssh.lib "%CUR_PATH%\lib\" /Y || EXIT /B 5
copy bin\ssh.dll "%XBMC_PATH%\system\" /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
