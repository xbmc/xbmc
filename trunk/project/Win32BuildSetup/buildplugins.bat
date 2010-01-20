@ECHO OFF
rem XBMC for Windows install plugin
rem Copyright (C) 2005-2008 Team XBMC
rem http://xbmc.org

rem Script by chadoe
rem This plugin builds all plugins in the optinal directory provided and copy the builds to BUILD_WIN32 for further packaging

SET PLUGIN_PATH="%1"
SET CUR_PATH=%CD%
ECHO ------------------------------------------------------------
ECHO Compiling plugins...

IF "%PLUGIN_PATH%" == "" GOTO DONE

rem optional plugins
for %%a IN (music pictures programs video weather) DO (
SETLOCAL ENABLEDELAYEDEXPANSION
SET _BAT=""
FOR /F "tokens=*" %%S IN ('dir /B /AD "%PLUGIN_PATH%\%%a"') DO (
  IF "%%S" NEQ ".svn" (
    SET _BAT=""
	CD "%PLUGIN_PATH%\%%a\%%S"
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
	  if EXIST "%PLUGIN_PATH%\%%a\%%S\BUILD\%%a\%%S\default.py" (
	    ECHO Copying files...
	    xcopy "%PLUGIN_PATH%\%%a\%%S\BUILD\%%a\%%S" "%CUR_PATH%\BUILD_WIN32\Xbmc\plugins\%%a\%%S" /E /Q /I /Y /EXCLUDE:exclude.txt > NUL
	  ) ELSE (
	    ECHO "%PLUGIN_PATH%\%%a\%%S\BUILD\%%a\%%S\default.py not found, not including in build." >> error.log
	  )
	) ELSE (
	  CD "%CUR_PATH%"
	  IF EXIST "%PLUGIN_PATH%\%%a\%%S\default.py" (
	    xcopy "%PLUGIN_PATH%\%%a\%%S" "%CUR_PATH%\BUILD_WIN32\Xbmc\plugins\%%a\%%S" /E /Q /I /Y /EXCLUDE:exclude.txt > NUL
	  ) ELSE (
	    ECHO "No build.bat or default.py found for directory %%S, not including in build." >> error.log
	  )
	)
  )
)
ENDLOCAL
)
:DONE
CD "%CUR_PATH%"