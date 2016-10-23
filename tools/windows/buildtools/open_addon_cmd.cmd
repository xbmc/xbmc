@echo off

set path=%path%;%~dp0
set kodi_src=%~dp0..\..\..

cd %~dp0..\..\..\..

if not defined DevEnvDir (
  call "%VS140COMNTOOLS%..\..\VC\bin\vcvars32.bat"
)

cmd
