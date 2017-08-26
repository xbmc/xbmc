@ECHO OFF

PUSHD %~dp0\..
CALL download-dependencies.bat x64-uwp
POPD
