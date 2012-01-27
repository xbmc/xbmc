@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libssh_d.txt

CALL dlextract.bat libssh %FILES%

cd %TMP_PATH%

xcopy include\libssh "%CUR_PATH%\include\libssh" /E /Q /I /Y
copy lib\ssh.lib "%CUR_PATH%\lib\" /Y
copy bin\ssh.dll "%XBMC_PATH%\system\" /Y

cd %LOC_PATH%
