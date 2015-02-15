@ECHO OFF

SETLOCAL

SET CUR_PATH=%CD%
SET APP_PATH=%CD%\..\..
SET TMP_PATH=%CD%\scripts\tmp

rem can't run rmdir and md back to back. access denied error otherwise.
IF EXIST lib rmdir lib /S /Q
IF EXIST include rmdir include /S /Q
IF EXIST %TMP_PATH% rmdir %TMP_PATH% /S /Q

IF $%1$ == $$ (
  SET DL_PATH="%CD%\downloads"
) ELSE (
  SET DL_PATH="%1"
)

SET WGET=%CUR_PATH%\bin\wget
SET ZIP=%CUR_PATH%\..\Win32BuildSetup\tools\7z\7za

IF NOT EXIST %DL_PATH% md %DL_PATH%

md lib
md include
md %TMP_PATH%

cd scripts

SET FORMED_OK_FLAG=%TMP_PATH%\got-all-formed-packages
REM Trick to preserve console title
start /b /wait cmd.exe /c get_formed.cmd
IF NOT EXIST %FORMED_OK_FLAG% (
  ECHO ERROR: Not all formed packages are ready!
  EXIT /B 101
)

cd %CUR_PATH%

rmdir %TMP_PATH% /S /Q
EXIT /B 0
