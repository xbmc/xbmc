@echo OFF

set PATH_CHANGE_REV_FILENAME=.last_success_revision

if not exist %1\%PATH_CHANGE_REV_FILENAME% (
	exit /b 1
)

for /f "delims=" %%a in (%1\%PATH_CHANGE_REV_FILENAME%) do set previousBuildHash=%%a
for /f "tokens=*" %%a in ('call %~dp0\getBuildHash.bat') do set currentBuildHash=%%a

if "%previousBuildHash%" neq "%currentBuildHash%" (
	exit /b 1
)
