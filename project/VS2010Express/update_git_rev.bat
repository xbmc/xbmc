@echo off

SET TEMPLATE=..\..\xbmc\win32\git_rev.tmpl
SET TEMP=..\..\xbmc\win32\git_rev.tmp
SET REV_FILE=..\..\xbmc\win32\git_rev.h
IF EXIST %TEMP% (
  del %TEMP%
)

CALL ..\Win32BuildSetup\extract_git_rev.bat

copy "%TEMPLATE%" "%TEMP%"

echo #define GIT_REV "%GIT_REV%" >> %TEMP%

fc /B %TEMP% %REV_FILE% > nul
IF ERRORLEVEL 1 (
  IF EXIST %REV_FILE% (
    del %REV_FILE%
  )
  COPY %TEMP% %REV_FILE%
)

DEL %TEMP%