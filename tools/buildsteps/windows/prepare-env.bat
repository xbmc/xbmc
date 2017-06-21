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
ECHO running git clean -xffd -e "project/BuildDependencies/downloads" -e "project/BuildDependencies/downloads2" -e "project/BuildDependencies/mingwlibs" -e "project/Win32BuildSetup/dependencies/vcredist"
git clean -xffd -e "project/BuildDependencies/downloads" -e "project/BuildDependencies/downloads2" -e "project/BuildDependencies/mingwlibs" -e "project/Win32BuildSetup/dependencies/vcredist"

rem cleaning additional directories
ECHO delete build directories
IF EXIST %WORKSPACE%\project\Win32BuildSetup\dependencies rmdir %WORKSPACE%\project\Win32BuildSetup\dependencies /S /Q

IF EXIST %WORKSPACE%\project\BuildDependencies\include rmdir %WORKSPACE%\project\BuildDependencies\include /S /Q
IF EXIST %WORKSPACE%\project\BuildDependencies\lib rmdir %WORKSPACE%\project\BuildDependencies\lib /S /Q
IF EXIST %WORKSPACE%\project\BuildDependencies\msys rmdir %WORKSPACE%\project\BuildDependencies\msys /S /Q
