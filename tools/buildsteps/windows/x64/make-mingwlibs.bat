@ECHO OFF

PUSHD %~dp0\..
CALL vswhere.bat x64
CALL make-mingwlibs.bat build64 %*
POPD
