@echo off

SET REV_FILE=..\..\git_revision.h

CALL ..\Win32BuildSetup\extract_git_rev.bat

IF NOT [%GIT_REV%]==[] (
  echo #define GIT_REV "%GIT_REV%" > "%REV_FILE%"
) ELSE (
  echo. > "%REV_FILE%"
)
