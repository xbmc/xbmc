@echo OFF

pushd %~dp0\..
call .helpers\default.bat
call vswhere.bat x86
if ERRORLEVEL 1 (
	echo ERROR! Something went wrong when calling vswhere.bat
	popd
	exit /B 1
)
popd

set TARGET_CMAKE_GENERATOR=Visual Studio %vsver%
set TARGET_CMAKE_GENERATOR_PLATFORM=Win32
set TARGET_ARCHITECTURE=x86
set TARGET_PLATFORM=%TARGET_ARCHITECTURE%
