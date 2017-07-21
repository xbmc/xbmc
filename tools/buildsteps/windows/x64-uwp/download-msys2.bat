@ECHO OFF

PUSHD %~dp0\..
CALL download-msys2.bat build64 %*
POPD
