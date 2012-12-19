@echo off

rem
rem Usage - Sign.cmd <target file>
rem

setlocal

set TargetFile=%~1
if not exist "%SigningCert%" (
  set SigningCert=%~dp0PlexTestSPC.pfx
)

set PasswordArg=
if "%SigningCertPassword%" NEQ "" (
  set PasswordArg=/p %SigningCertPassword%
)

signtool.exe sign /f "%SigningCert%" %PasswordArg% /t http://timestamp.verisign.com/scripts/timstamp.dll "%TargetFile%"
if "%errorlevel%" == 1 (
  echo FATAL ERROR - could sign %TargetFile%
)

signtool.exe verify /pa "%TargetFile%"
if "%errorlevel%" == 1 (
  echo FATAL ERROR - could not verify signature for %TargetFile%
)

endlocal
