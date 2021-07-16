@echo OFF

set PATH_CHANGE_REV_FILENAME=.last_success_revision

call %~dp0\getBuildHash.bat > %1\%PATH_CHANGE_REV_FILENAME%
