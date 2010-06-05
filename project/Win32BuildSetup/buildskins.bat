@ECHO OFF
rem XBMC for Windows install script
rem Copyright (C) 2005-2008 Team XBMC
rem http://xbmc.org

rem Script by chadoe
rem This script builds the Confluence and all skins in the optinal directory provided and copy the builds to BUILD_WIN32 for further packaging

SET SKIN_PATH="%1"
SET CUR_PATH=%CD%
ECHO ------------------------------------------------------------
ECHO Compiling skins...

SET PATH=%PATH%;%CD%\..\..\Tools\XBMCTex

rem default skins

ECHO Compiling Confluence...
cd "..\..\addons\skin.confluence"
CALL build.bat > NUL
cd "%CUR_PATH%"

IF "%SKIN_PATH%" == "" GOTO DONE
rem optional skins
SETLOCAL ENABLEDELAYEDEXPANSION
SET _BAT=""
FOR /F "tokens=*" %%S IN ('dir /B /AD "%SKIN_PATH%"') DO (
  IF "%%S" NEQ ".svn" (
    SET _BAT=""
	CD "%SKIN_PATH%\%%S"
	IF EXIST "build.bat" (
	  ECHO Found build.bat
	  SET _BAT=build.bat
	)
	IF EXIST "build_skin.bat" (
	  ECHO Found build_skin.bat
	  SET _BAT=build_skin.bat
	)
	
	IF !_BAT! NEQ "" (
	  IF EXIST _btmp.bat del _btmp.bat > NUL
	  rem create temp bat file without the pause statements in the original bat file.
	  for /f "tokens=*" %%a in ('findstr /v /i /c:"pause" "!_BAT!"') do (
		echo %%a>> _btmp.bat
	  )
	  
      ECHO Building skin %%S
	  call _btmp.bat
	  del _btmp.bat > NUL
	  CD "%CUR_PATH%"
	  if EXIST "%SKIN_PATH%\%%S\BUILD\%%S\skin.xml" (
	    ECHO Copying files...
	    xcopy "%SKIN_PATH%\%%S\BUILD\%%S" "%CUR_PATH%\BUILD_WIN32\Xbmc\skin\%%S" /E /Q /I /Y /EXCLUDE:exclude.txt > NUL
	  ) ELSE (
	    ECHO "%SKIN_PATH%\%%S\BUILD\%%S\skin.xml not found, not including in build." >> error.log
	  )
	) ELSE (
	  CD "%CUR_PATH%"
	  IF EXIST "%SKIN_PATH%\%%S\skin.xml" (
	    xcopy "%SKIN_PATH%\%%S" "%CUR_PATH%\BUILD_WIN32\Xbmc\skin\%%S" /E /Q /I /Y /EXCLUDE:exclude.txt > NUL
	  ) ELSE (
	    ECHO "No build.bat, build_skin.bat or skin.xml found for directory %%S, not including in build." >> error.log
	  )
	)
  )
)
ENDLOCAL
:DONE
CD "%CUR_PATH%"