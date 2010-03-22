@ECHO OFF
rem ----Usage----
rem BuildSetup [gl|dx] [clean|noclean]
rem gl for opengl build (default)
rem dx for directx build
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
SET target=dx
SET buildmode=ask
SET promptlevel=prompt
FOR %%b in (%1, %2, %3, %4) DO (
	IF %%b==dx SET target=dx
	IF %%b==gl SET target=gl
	IF %%b==clean SET buildmode=clean
	IF %%b==noclean SET buildmode=noclean
	IF %%b==noprompt SET promptlevel=noprompt
)
SET buildconfig=Release (OpenGL)
IF %target%==dx SET buildconfig=Release (DirectX)

	IF "%VS90COMNTOOLS%"=="" (
		set NET="%ProgramFiles%\Microsoft Visual Studio 9.0 Express\Common7\IDE\VCExpress.exe"
	) ELSE IF EXIST "%VS90COMNTOOLS%\..\IDE\VCExpress.exe" (
		set NET="%VS90COMNTOOLS%\..\IDE\VCExpress.exe"
	) ELSE IF EXIST "%VS90COMNTOOLS%\..\IDE\devenv.exe" (
		set NET="%VS90COMNTOOLS%\..\IDE\devenv.exe"
	)
  
	IF NOT EXIST %NET% (
	  set DIETEXT=Visual Studio .NET 2008 Express was not found.
	  goto DIE
	) 
    set OPTS_EXE="..\VS2008Express\XBMC for Windows.sln" /build "%buildconfig%"
	set CLEAN_EXE="..\VS2008Express\XBMC for Windows.sln" /clean "%buildconfig%"
	set EXE= "..\VS2008Express\XBMC\%buildconfig%\XBMC.exe"
	
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
  echo.
  goto EXE_COMPILE

:EXE_COMPILE
  IF %buildmode%==clean goto COMPILE_EXE
  rem ---------------------------------------------
  rem	check for existing xbe
  rem ---------------------------------------------
  IF EXIST %EXE% (
    goto EXE_EXIST
  )
  goto COMPILE_EXE

:EXE_EXIST
  IF %buildmode%==noclean goto COMPILE_NO_CLEAN_EXE
  ECHO ------------------------------------------------------------
  ECHO Found a previous Compiled WIN32 EXE!
  ECHO [1] a NEW EXE will be compiled for the BUILD_WIN32
  ECHO [2] existing EXE will be updated (quick mode compile) for the BUILD_WIN32
  ECHO ------------------------------------------------------------
  set /P XBMC_COMPILE_ANSWER=Compile a new EXE? [1/2]:
  if /I %XBMC_COMPILE_ANSWER% EQU 1 goto COMPILE_EXE
  if /I %XBMC_COMPILE_ANSWER% EQU 2 goto COMPILE_NO_CLEAN_EXE
  
:COMPILE_EXE
  ECHO Wait while preparing the build.
  ECHO ------------------------------------------------------------
  ECHO Cleaning Solution...
  %NET% %CLEAN_EXE%
  ECHO Compiling Solution...
  %NET% %OPTS_EXE%
  IF NOT EXIST %EXE% (
  	set DIETEXT="XBMC.EXE failed to build!  See ..\vs2008express\XBMC\%buildconfig%\BuildLog.htm for details."
  	goto DIE
  )
  ECHO Done!
  ECHO ------------------------------------------------------------
  GOTO MAKE_BUILD_EXE
  
:COMPILE_NO_CLEAN_EXE
  ECHO Wait while preparing the build.
  ECHO ------------------------------------------------------------
  ECHO Compiling Solution...
  %NET% %OPTS_EXE%
  IF NOT EXIST %EXE% (
  	set DIETEXT="XBMC.EXE failed to build!  See ..\vs2008express\XBMC\%buildconfig%\BuildLog\BuildLog.htm for details."
  	goto DIE
  )
  ECHO Done!
  ECHO ------------------------------------------------------------
  GOTO MAKE_BUILD_EXE

:MAKE_BUILD_EXE
  ECHO Copying files...
  rmdir BUILD_WIN32 /S /Q
  md BUILD_WIN32\Xbmc

  Echo .svn>exclude.txt
  Echo CVS>>exclude.txt
  Echo .so>>exclude.txt
  Echo Thumbs.db>>exclude.txt
  Echo Desktop.ini>>exclude.txt
  Echo dsstdfx.bin>>exclude.txt
  Echo exclude.txt>>exclude.txt
  rem and exclude potential leftovers
  Echo mediasources.xml>>exclude.txt
  Echo advancedsettings.xml>>exclude.txt
  Echo guisettings.xml>>exclude.txt
  Echo profiles.xml>>exclude.txt
  Echo sources.xml>>exclude.txt
  Echo userdata\cache\>>exclude.txt
  Echo userdata\database\>>exclude.txt
  Echo userdata\playlists\>>exclude.txt
  Echo userdata\script_data\>>exclude.txt
  Echo userdata\thumbnails\>>exclude.txt
  rem UserData\visualisations contains currently only xbox visualisationfiles
  Echo userdata\visualisations\>>exclude.txt
  rem other platform stuff
  Echo lib-osx>>exclude.txt
  Echo players\mplayer>>exclude.txt
  Echo FileZilla Server.xml>>exclude.txt
  Echo asound.conf>>exclude.txt
  Echo voicemasks.xml>>exclude.txt
  Echo Lircmap.xml>>exclude.txt
  Echo getdeps.bat>>exclude.txt

  xcopy %EXE% BUILD_WIN32\Xbmc > NUL
  xcopy ..\..\userdata BUILD_WIN32\Xbmc\userdata /E /Q /I /Y /EXCLUDE:exclude.txt > NUL
  copy ..\..\copying.txt BUILD_WIN32\Xbmc > NUL
  copy ..\..\LICENSE.GPL BUILD_WIN32\Xbmc > NUL
  copy ..\..\known_issues.txt BUILD_WIN32\Xbmc > NUL
  call dependencies\getdeps.bat > NUL
  xcopy dependencies\*.* BUILD_WIN32\Xbmc /Q /I /Y /EXCLUDE:exclude.txt  > NUL
  copy sources.xml BUILD_WIN32\Xbmc\userdata > NUL
  
  xcopy ..\..\language BUILD_WIN32\Xbmc\language /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL
  rem screensavers currently are xbox only
  rem xcopy ..\..\screensavers BUILD_WIN32\Xbmc\screensavers /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL
  xcopy ..\..\addons BUILD_WIN32\Xbmc\addons /E /Q /I /Y /EXCLUDE:exclude.txt > NUL
  xcopy ..\..\system BUILD_WIN32\Xbmc\system /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL
  xcopy ..\..\media BUILD_WIN32\Xbmc\media /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL
  xcopy ..\..\sounds BUILD_WIN32\Xbmc\sounds /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL
  xcopy "..\..\web\poc_jsonrpc" BUILD_WIN32\Xbmc\web /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL
  
  SET skinpath=%CD%\Add_skins
  SET scriptpath=%CD%\Add_scripts
  SET pluginpath=%CD%\Add_plugins
  rem override skin/script/pluginpaths from config.ini if there's a config.ini
  IF EXIST config.ini FOR /F "tokens=* DELIMS=" %%a IN ('FINDSTR/R "=" config.ini') DO SET %%a
  
  IF EXIST error.log del error.log > NUL
  call buildskins.bat "%skinpath%"
  call buildscripts.bat "%scriptpath%"
  call buildplugins.bat "%pluginpath%"
  rem reset variables
  SET skinpath=
  SET scriptpath=
  SET pluginpath=
  rem restore color and title, some scripts mess these up
  COLOR 1B
  TITLE XBMC for Windows Build Script

  IF EXIST exclude.txt del exclude.txt  > NUL
  ECHO ------------------------------------------------------------
  ECHO Build Succeeded!
  GOTO NSIS_EXE

:NSIS_EXE
  ECHO ------------------------------------------------------------
  ECHO Generating installer includes...
  call genNsisIncludes.bat
  ECHO ------------------------------------------------------------
  FOR /F "Tokens=2* Delims=]" %%R IN ('FIND /v /n "&_&_&_&" "..\..\.svn\entries" ^| FIND "[11]"') DO SET XBMC_REV=%%R
  SET XBMC_SETUPFILE=XBMCSetup-Rev%XBMC_REV%-%target%.exe
  ECHO Creating installer %XBMC_SETUPFILE%...
  IF EXIST %XBMC_SETUPFILE% del %XBMC_SETUPFILE% > NUL
  rem get path to makensis.exe from registry, first try tab delim
  FOR /F "tokens=2* delims=	" %%A IN ('REG QUERY "HKLM\Software\NSIS" /ve') DO SET NSISExePath=%%B
  IF NOT EXIST "%NSISExePath%" (
    rem try with space delim instead of tab
    FOR /F "tokens=2* delims= " %%A IN ('REG QUERY "HKLM\Software\NSIS" /ve') DO SET NSISExePath=%%B
    IF NOT EXIST "%NSISExePath%" (
      rem on win 7 x86, the previous fails
      FOR /F "tokens=3* delims= " %%A IN ('REG QUERY "HKLM\Software\NSIS" /ve') DO SET NSISExePath=%%B
    )
  )
  rem proper x64 registry checks
  IF NOT EXIST "%NSISExePath%" (
    ECHO using x64 registry entries
    FOR /F "tokens=2* delims=	" %%A IN ('REG QUERY "HKLM\Software\Wow6432Node\NSIS" /ve') DO SET NSISExePath=%%B
  )
  IF NOT EXIST "%NSISExePath%" (
    rem try with space delim instead of tab
    FOR /F "tokens=2* delims= " %%A IN ('REG QUERY "HKLM\Software\Wow6432Node\NSIS" /ve') DO SET NSISExePath=%%B
  )

  SET NSISExe=%NSISExePath%\makensis.exe
  "%NSISExe%" /V1 /X"SetCompressor /FINAL lzma" /Dxbmc_root="%CD%\BUILD_WIN32" /Dxbmc_revision="%XBMC_REV%" /Dxbmc_target="%target%" "XBMC for Windows.nsi"
  IF NOT EXIST "%XBMC_SETUPFILE%" (
	  set DIETEXT=Failed to create %XBMC_SETUPFILE%.
	  goto DIE
  )
  ECHO ------------------------------------------------------------
  ECHO Done!
  ECHO Setup is located at %CD%\%XBMC_SETUPFILE%
  ECHO ------------------------------------------------------------
  GOTO VIEWLOG_EXE
  
:DIE
  ECHO ------------------------------------------------------------
  ECHO !-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-
  ECHO    ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR
  ECHO !-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-
  set DIETEXT=ERROR: %DIETEXT%
  echo %DIETEXT%
  ECHO ------------------------------------------------------------

:VIEWLOG_EXE
  IF %promptlevel%==noprompt (
  goto END
  )
  IF NOT EXIST "%CD%\..\vs2008express\XBMC\%buildconfig%\" BuildLog.htm" goto END
  set /P XBMC_BUILD_ANSWER=View the build log in your HTML browser? [y/n]
  if /I %XBMC_BUILD_ANSWER% NEQ y goto END
  start /D"%CD%\..\vs2008express\XBMC\%buildconfig%\" BuildLog.htm"
  goto END

:END
  IF %promptlevel% NEQ noprompt (
  ECHO Press any key to exit...
  pause > NUL
  )