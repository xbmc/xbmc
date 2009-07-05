@ECHO OFF
rem XBMC for Windows install script
rem Copyright (C) 2005-2008 Team XBMC
rem http://xbmc.org

rem Script by chadoe
rem This script builds all scripts in the optinal directory provided and copy the builds to BUILD_WIN32 for further packaging

SET SCRIPT_PATH="%1"
SET CUR_PATH=%CD%
ECHO ------------------------------------------------------------
ECHO Compiling scripts...

IF "%SCRIPT_PATH%" == "" GOTO DONE
rem optional plugins
SETLOCAL ENABLEDELAYEDEXPANSION
SET _BAT=""
FOR /F "tokens=*" %%S IN ('dir /B /AD "%SCRIPT_PATH%"') DO (
  IF "%%S" NEQ ".svn" (
    SET _BAT=""
	CD "%SCRIPT_PATH%\%%S"
	IF EXIST "build.bat" (
	  ECHO Found build.bat
	  SET _BAT=build.bat
	)
	IF !_BAT! NEQ "" (
	  IF EXIST _btmp.bat del _btmp.bat > NUL
	  rem create temp bat file without the pause statements in the original bat file.
	  for /f "tokens=*" %%a in ('findstr /v /i /c:"pause" "!_BAT!"') do (
		echo %%a>> _btmp.bat
	  )
	  
      ECHO Building plugin %%S
	  call _btmp.bat
	  del _btmp.bat > NUL
	  CD "%CUR_PATH%"
	  if EXIST "%SCRIPT_PATH%\%%S\BUILD\%%S\default.py" (
	    ECHO Copying files...
	    xcopy "%SCRIPT_PATH%\%%S\BUILD\%%S" "%CUR_PATH%\BUILD_WIN32\Xbmc\scripts\%%S" /E /Q /I /Y /EXCLUDE:exclude.txt > NUL
	  ) ELSE (
	    ECHO "%SCRIPT_PATH%\%%S\BUILD\%%S\default.py not found, not including in build." >> error.log
	  )
	) ELSE (
	  CD "%CUR_PATH%"
	  IF EXIST "%SCRIPT_PATH%\%%S\default.py" (
	    xcopy "%SCRIPT_PATH%\%%S" "%CUR_PATH%\BUILD_WIN32\Xbmc\scripts\%%S" /E /Q /I /Y /EXCLUDE:exclude.txt > NUL
	  ) ELSE (
	    ECHO "No build.bat or default.py found for directory %%S, not including in build." >> error.log
	  )
	)
  )
)
ENDLOCAL
:DONE
CD "%CUR_PATH%"