@ECHO OFF

PUSHD %~dp0\..\..\..
SET WORKSPACE=%CD%
POPD

ECHO Workspace is %WORKSPACE%

cd %WORKSPACE%
rem clean the BUILD_WIN32 at first to avoid problems with possible git files in there
IF EXIST %WORKSPACE%\project\Win32BuildSetup\BUILD_WIN32 rmdir %WORKSPACE%\project\Win32BuildSetup\BUILD_WIN32 /S /Q

rem we assume git in path as this is a requirement
rem git clean the untracked files and directories
rem but keep the downloaded dependencies
ECHO running git clean -xffd -e "project/BuildDependencies/downloads" -e "project/BuildDependencies/downloads2" -e "project/BuildDependencies/mingwlibs"
git clean -xffd -e "project/BuildDependencies/downloads" -e "project/BuildDependencies/downloads2" -e "project/BuildDependencies/mingwlibs"
