@ECHO OFF

PUSHD %~dp0\..
CALL download-dependencies.bat arm-uwp
POPD
