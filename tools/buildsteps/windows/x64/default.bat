@echo OFF

pushd %~dp0\..
call .helpers\default.bat
call vswhere.bat x64
if ERRORLEVEL 1 (
	echo ERROR! Something went wrong when calling vswhere.bat
	popd
	exit /B 1
)
popd

set BUILD_TYPE=Release

set TARGET_CMAKE_GENERATOR=Visual Studio %vsver%
set TARGET_CMAKE_GENERATOR_PLATFORM=x64
set TARGET_ARCHITECTURE=x64
set TARGET_PLATFORM=%TARGET_ARCHITECTURE%
