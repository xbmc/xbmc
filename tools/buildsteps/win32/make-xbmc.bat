@ECHO OFF
rem ----Usage----
rem BuildSetup [clean|noclean]
rem clean to force a full rebuild
rem noclean to force a build without clean
rem noprompt to avoid all prompts
CLS
COLOR 1B
TITLE XBMC for Windows Build Script
rem ----PURPOSE----
rem - Create a working XBMC build with a single click
rem -------------------------------------------------------------
rem Config
rem If you get an error that Visual studio was not found, SET your path for VSNET main executable.
rem -------------------------------------------------------------
rem	CONFIG START
SET comp=vs2010
SET buildconfig=Release (DirectX)
SET buildmode=ask
SET promptlevel=prompt
SET buildmingwlibs=true
SET exitcode=0
FOR %%b in (%1, %2, %3, %4, %5) DO (
	IF %%b==clean SET buildmode=clean
	IF %%b==noclean SET buildmode=noclean
	IF %%b==noprompt SET promptlevel=noprompt
	IF %%b==nomingwlibs SET buildmingwlibs=false
)

IF $%Configuration%$ == $$ (
  IF %Configuration%==Release SET buildconfig=Release (DirectX)
  IF %Configuration%==Debug   SET buildconfig=Debug (DirectX)
)

SET BS_DIR=%WORKSPACE%\project\Win32BuildSetup
cd %BS_DIR%

IF %comp%==vs2010 (
  IF "%VS100COMNTOOLS%"=="" (
		set NET="%ProgramFiles%\Microsoft Visual Studio 10.0\Common7\IDE\VCExpress.exe"
	) ELSE IF EXIST "%VS100COMNTOOLS%\..\IDE\VCExpress.exe" (
		set NET="%VS100COMNTOOLS%\..\IDE\VCExpress.exe"
	) ELSE IF EXIST "%VS100COMNTOOLS%\..\IDE\devenv.exe" (
		set NET="%VS100COMNTOOLS%\..\IDE\devenv.exe"
	)
)

IF NOT EXIST %NET% (
 set DIETEXT=Visual Studio .NET 2010 Express was not found.
 goto DIE
)

set OPTS_EXE="..\VS2010Express\XBMC for Windows.sln" /build "%buildconfig%"
set CLEAN_EXE="..\VS2010Express\XBMC for Windows.sln" /clean "%buildconfig%"
set EXE= "..\VS2010Express\XBMC\%buildconfig%\XBMC.exe"
set PDB= "..\VS2010Express\XBMC\%buildconfig%\XBMC.pdb"

rem	CONFIG END
rem -------------------------------------------------------------

echo                         :                                                  
echo                        :::                                                 
echo                        ::::                                                
echo                        ::::                                                
echo    :::::::       :::::::::::::::::        ::::::      ::::::        :::::::
echo    :::::::::   ::::::::::::::::::::     ::::::::::  ::::::::::    :::::::::
echo     ::::::::: ::::::::::::::::::::::   ::::::::::::::::::::::::  ::::::::: 
echo          :::::::::     :::      ::::: :::::    ::::::::    :::: :::::      
echo           ::::::      ::::       :::: ::::      :::::       :::::::        
echo           :::::       ::::        :::::::       :::::       ::::::         
echo           :::::       :::         ::::::         :::        ::::::         
echo           ::::        :::         ::::::        ::::        ::::::         
echo           ::::        :::        :::::::        ::::        ::::::         
echo          :::::        ::::       :::::::        ::::        ::::::         
echo         :::::::       ::::      ::::::::        :::         :::::::        
echo     :::::::::::::::    :::::  ::::: :::         :::         :::::::::      
echo  :::::::::  :::::::::  :::::::::::  :::         :::         ::: :::::::::  
echo  ::::::::    :::::::::  :::::::::   :::         :::         :::  ::::::::  
echo ::::::         :::::::    :::::     :            ::          ::    ::::::   
echo Building %buildconfig%
goto EXE_COMPILE

:EXE_COMPILE
  IF EXIST buildlog.html del buildlog.html /q
  IF %buildmode%==clean goto COMPILE_EXE
  IF %buildmode%==noclean goto COMPILE_NO_CLEAN_EXE
  rem ---------------------------------------------
  rem	check for existing exe
  rem ---------------------------------------------
  
  IF EXIST %EXE% (
    goto EXE_EXIST
  )
  goto COMPILE_EXE

:EXE_EXIST
  IF %promptlevel%==noprompt goto COMPILE_EXE
  ECHO ------------------------------------------------------------
  ECHO Found a previous Compiled WIN32 EXE!
  ECHO [1] a NEW EXE will be compiled for the BUILD_WIN32
  ECHO [2] existing EXE will be updated (quick mode compile) for the BUILD_WIN32
  ECHO ------------------------------------------------------------
  set /P APP_COMPILE_ANSWER=Compile a new EXE? [1/2]:
  if /I %APP_COMPILE_ANSWER% EQU 1 goto COMPILE_EXE
  if /I %APP_COMPILE_ANSWER% EQU 2 goto COMPILE_NO_CLEAN_EXE
  
:COMPILE_EXE
  ECHO Wait while preparing the build.
  ECHO ------------------------------------------------------------
  ECHO Cleaning Solution...
  %NET% %CLEAN_EXE%
  ECHO Compiling XBMC...
  %NET% %OPTS_EXE%
  IF NOT EXIST %EXE% (
  	set DIETEXT="XBMC.EXE failed to build!  See %WORKSPACE%\project\vs2010express\XBMC\%buildconfig%\objs\XBMC.log"
	IF %promptlevel%==noprompt (
		type "%WORKSPACE%\project\vs2010express\XBMC\%buildconfig%\objs\XBMC.log"
	)
  	goto DIE
  )
  ECHO Done!
  ECHO ------------------------------------------------------------
  set buildmode=clean
  GOTO END
  
:COMPILE_NO_CLEAN_EXE
  ECHO Wait while preparing the build.
  ECHO ------------------------------------------------------------
  ECHO Compiling Solution...
  %NET% %OPTS_EXE%
  IF NOT EXIST %EXE% (
  	set DIETEXT="XBMC.EXE failed to build!  See %WORKSPACE%\project\vs2010express\XBMC\%buildconfig%\objs\XBMC.log"
	IF %promptlevel%==noprompt (
		type "%WORKSPACE%\project\vs2010express\XBMC\%buildconfig%\objs\XBMC.log"
	)
  	goto DIE
  )
  ECHO Done!
  ECHO ------------------------------------------------------------
  GOTO END
  
  
:DIE
  ECHO ------------------------------------------------------------
  ECHO !-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-
  ECHO    ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR
  ECHO !-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-
  set DIETEXT=ERROR: %DIETEXT%
  echo %DIETEXT%
  SET exitcode=1
  ECHO ------------------------------------------------------------

:VIEWLOG_EXE
  SET log="%WORKSPACE%\project\vs2010express\XBMC\%buildconfig%\objs\XBMC.log"
  IF NOT EXIST %log% goto END
  
  copy %log% ./buildlog.html > NUL

  IF %promptlevel%==noprompt (
  goto END
  )

  set /P APP_BUILD_ANSWER=View the build log in your HTML browser? [y/n]
  if /I %APP_BUILD_ANSWER% NEQ y goto END
  
  SET log="%WORKSPACE%\project\vs2010express\XBMC\%buildconfig%\objs\" XBMC.log
  
  start /D%log%
  goto END

:END
  IF %promptlevel% NEQ noprompt (
  ECHO Press any key to exit...
  pause > NUL
  )
  EXIT /B %exitcode%