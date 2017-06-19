@ECHO OFF

PUSHD %~dp0\..
CALL make-mingwlibs.bat build64 %*
POPD
