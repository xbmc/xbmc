@echo off
rem subwcrev is included in the tortoise svn client: http://tortoisesvn.net/downloads
SET REV_FILE=..\..\xbmc\win32\svn_rev.h
IF EXIST %REV_FILE% (
  del %REV_FILE%
)

IF EXIST "..\..\.svn" (
  subwcrev ../.. ../../xbmc/win32/svn_rev.tmpl %REV_FILE%
  goto :eof
)

SetLocal EnableDelayedExpansion

IF EXIST "..\..\.git" (
  FOR /F "delims=: skip=8 tokens=2*" %%a IN ('git svn info') DO (call :set_var %%a %%b)
)

copy "..\..\xbmc\win32\svn_rev_git.tmpl" "%REV_FILE%"

echo #define SVN_REV   "%REV%" >> %REV_FILE%
echo #define SVN_DATE  "%REV_DATE%" >> %REV_FILE%

echo #endif >> %REV_FILE%

goto :eof

:set_var
IF "%REV%"=="" (
  SET REV=%1
) ELSE IF "%REV_DATE%"=="" (
  call :set_var_2 %1 %2 %3 %4 %5 %6 %7 %8
)
goto :eof

:set_var_2
SET REV_DATE=%1 %2:%3 %4 %5, %6 %7 %8^)
goto :eof

:eof