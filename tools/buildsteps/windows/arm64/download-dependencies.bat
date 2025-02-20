@ECHO OFF

PUSHD %~dp0\..
CALL download-dependencies.bat arm64
POPD
