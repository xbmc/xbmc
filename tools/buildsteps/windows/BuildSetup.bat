@ECHO OFF
REM setup all paths
PUSHD %~dp0\..\..\..
SET base_dir=%CD%
POPD

REM read the version values from version.txt
FOR /f "tokens=1,*" %%i IN (%base_dir%\version.txt) DO SET %%i=%%j

SET APP_VERSION=%VERSION_MAJOR%.%VERSION_MINOR%
IF NOT [%VERSION_TAG%] == [] (
  SET APP_VERSION=%APP_VERSION%-%VERSION_TAG%
)

rem ----Usage----
rem BuildSetup [clean|noclean] [noprompt] [nobinaryaddons] [sh]
rem clean to force a full rebuild
rem noclean to force a build without clean
rem noprompt to avoid all prompts
rem nobinaryaddons to skip building binary addons
rem sh to use sh shell instead rxvt
CLS
TITLE %APP_NAME% for Windows Build Script
rem ----PURPOSE----
rem - Create a working application build with a single click
rem -------------------------------------------------------------
rem Config
rem If you get an error that Visual studio was not found, SET your path for VSNET main executable.
rem -------------------------------------------------------------
rem  CONFIG START
SET buildmode=ask
SET promptlevel=prompt
SET buildbinaryaddons=true
SET exitcode=0
SET useshell=rxvt
FOR %%b in (%*) DO (
  IF %%b==clean SET buildmode=clean
  IF %%b==noclean SET buildmode=noclean
  IF %%b==noprompt SET promptlevel=noprompt
  IF %%b==nobinaryaddons SET buildbinaryaddons=false
  IF %%b==sh SET useshell=sh
)

SET PreferredToolArchitecture=x64
SET buildconfig=Release
set WORKSPACE=%base_dir%\kodi-build.%TARGET_PLATFORM%


  :: sets the BRANCH env var
  FOR /f %%a IN ('getbranch.bat') DO SET BRANCH=%%a

  rem  CONFIG END
  rem -------------------------------------------------------------
  goto COMPILE_CMAKE_EXE

:COMPILE_CMAKE_EXE
  ECHO Wait while preparing the build.
  ECHO ------------------------------------------------------------
  ECHO Compiling %APP_NAME% branch %BRANCH%...

  IF %buildmode%==clean (
    RMDIR /S /Q %WORKSPACE%
  )
  MKDIR %WORKSPACE%
  PUSHD %WORKSPACE%

  cmake.exe -G "%cmakeGenerator%" -A %cmakeArch% -T host=x64 %cmakeProps% %base_dir%
  IF %errorlevel%==1 (
    set DIETEXT="%APP_NAME%.EXE failed to build!"
    goto DIE
  )

  cmake.exe --build . --config "%buildconfig%"
  IF %errorlevel%==1 (
    set DIETEXT="%APP_NAME%.EXE failed to build!"
    goto DIE
  )

  set EXE="%WORKSPACE%\%buildconfig%\%APP_NAME%.exe"
  set PDB="%WORKSPACE%\%buildconfig%\%APP_NAME%.pdb"

  POPD
  ECHO Done!
  ECHO ------------------------------------------------------------
  IF "%cmakeProps%" NEQ "" GOTO MAKE_APPX
  GOTO MAKE_BUILD_EXE


:MAKE_BUILD_EXE
  ECHO Copying files...
  PUSHD %base_dir%\project\Win32BuildSetup
  IF EXIST BUILD_WIN32\application rmdir BUILD_WIN32\application /S /Q
  rem Add files to exclude.txt that should not be included in the installer

  Echo Thumbs.db>>exclude.txt
  Echo Desktop.ini>>exclude.txt
  Echo dsstdfx.bin>>exclude.txt
  Echo exclude.txt>>exclude.txt
  Echo xbmc.log>>exclude.txt
  Echo xbmc.old.log>>exclude.txt
  Echo kodi.log>>exclude.txt
  Echo kodi.old.log>>exclude.txt
  Echo .so\>>exclude.txt
  Echo .h\>>exclude.txt
  Echo .cpp\>>exclude.txt
  Echo .exp\>>exclude.txt
  Echo .lib\>>exclude.txt
  rem Exclude userdata files
  Echo userdata\advancedsettings.xml>>exclude.txt
  Echo userdata\guisettings.xml>>exclude.txt
  Echo userdata\mediasources.xml>>exclude.txt
  Echo userdata\ModeLines_template.xml>>exclude.txt
  Echo userdata\passwords.xml>>exclude.txt
  Echo userdata\profiles.xml>>exclude.txt
  Echo userdata\sources.xml>>exclude.txt
  Echo userdata\upnpserver.xml>>exclude.txt
  rem Exclude userdata folders
  Echo userdata\addon_data\>>exclude.txt
  Echo userdata\cache\>>exclude.txt
  Echo userdata\database\>>exclude.txt
  Echo userdata\playlists\>>exclude.txt
  Echo userdata\thumbnails\>>exclude.txt

  rem Exclude dlls from system to avoid duplicates
  Echo .dll>>exclude_dll.txt

  md BUILD_WIN32\application

  xcopy %EXE% BUILD_WIN32\application > NUL
  xcopy %base_dir%\userdata BUILD_WIN32\application\userdata /E /Q /I /Y /EXCLUDE:exclude.txt > NUL
  copy %base_dir%\LICENSE.md BUILD_WIN32\application > NUL
  copy %base_dir%\privacy-policy.txt BUILD_WIN32\application > NUL
  copy %base_dir%\known_issues.txt BUILD_WIN32\application > NUL

  xcopy %WORKSPACE%\%buildconfig%\addons BUILD_WIN32\application\addons /E /Q /I /Y /EXCLUDE:exclude.txt > NUL
  xcopy %WORKSPACE%\%buildconfig%\*.dll BUILD_WIN32\application /Q /I /Y > NUL
  xcopy %WORKSPACE%\%buildconfig%\libbluray-*.jar BUILD_WIN32\application /Q /I /Y > NUL
  xcopy %WORKSPACE%\%buildconfig%\system BUILD_WIN32\application\system /E /Q /I /Y /EXCLUDE:exclude.txt+exclude_dll.txt  > NUL
  xcopy %WORKSPACE%\%buildconfig%\media BUILD_WIN32\application\media /E /Q /I /Y /EXCLUDE:exclude.txt  > NUL

  REM create AppxManifest.xml
  @PowerShell "(GC .\AppxManifest.xml.in)|%%{$_" ^
    " -Replace '@APP_NAME@', '%APP_NAME%'" ^
    " -Replace '@COMPANY_NAME@', '%COMPANY_NAME%'" ^
    " -Replace '@TARGET_ARCHITECTURE@', '%TARGET_ARCHITECTURE%'" ^
    " -Replace '@VERSION_CODE@', '%VERSION_CODE%'" ^
    " -Replace '@PACKAGE_IDENTITY@', '%PACKAGE_IDENTITY%'" ^
    " -Replace '@PACKAGE_PUBLISHER@', '%PACKAGE_PUBLISHER%'" ^
    " -Replace '@PACKAGE_DESCRIPTION@', '%PACKAGE_DESCRIPTION%'" ^
    "}|SC .\BUILD_WIN32\application\AppxManifest.xml"

  SET build_path=%CD%
  IF %buildbinaryaddons%==true (
    ECHO ------------------------------------------------------------
    ECHO Building addons...
    cd %base_dir%\tools\buildsteps\windows
    IF %buildmode%==clean (
      call make-addons.bat clean
    )
    call make-addons.bat
    IF %errorlevel%==1 (
      set DIETEXT="failed to build addons"
      cd %build_path%
      goto DIE
    )

    cd %build_path%
    IF EXIST error.log del error.log > NUL
  )

  rem restore title, some scripts mess these up
  TITLE %APP_NAME% for Windows Build Script

  IF EXIST exclude.txt del exclude.txt > NUL
  IF EXIST exclude_dll.txt del exclude_dll.txt > NUL
  POPD

  ECHO ------------------------------------------------------------
  ECHO Build Succeeded!
  GOTO NSIS_EXE

:NSIS_EXE
  ECHO ------------------------------------------------------------
  ECHO Generating installer includes...
  PUSHD %base_dir%\project\Win32BuildSetup
  call genNsisIncludes.bat
  ECHO ------------------------------------------------------------
  call getdeploydependencies.bat
  CALL extract_git_rev.bat > NUL
  SET APP_SETUPFILE=%APP_NAME%Setup-%GIT_REV%-%BRANCH%-%TARGET_ARCHITECTURE%.exe
  SET APP_PDBFILE=%APP_NAME%Setup-%GIT_REV%-%BRANCH%-%TARGET_ARCHITECTURE%.pdb
  ECHO Creating installer %APP_SETUPFILE%...
  IF EXIST %APP_SETUPFILE% del %APP_SETUPFILE% > NUL

  rem determine if current system is 32 or 64 bits
  SET HOST_BITS=32
  IF %PROCESSOR_ARCHITECTURE% == AMD64 SET HOST_BITS=64
  IF %PROCESSOR_ARCHITECTURE% == ARM64 SET HOST_BITS=64

  IF %HOST_BITS% == 64 (
    SET NSIS_REG_KEY=HKLM\Software\Wow6432Node\NSIS
  ) ELSE (
    SET NSIS_REG_KEY=HKLM\Software\NSIS
  )

  rem get path to makensis.exe from registry, first try tab delim
  FOR /F "tokens=2* delims=  " %%A IN ('REG QUERY "%NSIS_REG_KEY%" /ve') DO SET NSISExePath=%%B

  IF NOT EXIST "%NSISExePath%" (
    rem try with space delim instead of tab
    FOR /F "tokens=2* delims= " %%A IN ('REG QUERY "%NSIS_REG_KEY%" /ve') DO SET NSISExePath=%%B
  )

  IF NOT EXIST "%NSISExePath%" (
    rem fails on localized windows (Default) becomes (Par D�faut)
    FOR /F "tokens=3* delims=  " %%A IN ('REG QUERY "%NSIS_REG_KEY%" /ve') DO SET NSISExePath=%%B
  )

  IF NOT EXIST "%NSISExePath%" (
    FOR /F "tokens=3* delims= " %%A IN ('REG QUERY "%NSIS_REG_KEY%" /ve') DO SET NSISExePath=%%B
  )

  SET NSISExe=%NSISExePath%\makensis.exe
  "%NSISExe%" /V1 /X"SetCompressor /FINAL lzma" /Dapp_root="%CD%\BUILD_WIN32" /DAPP_NAME="%APP_NAME%" /DTARGET_ARCHITECTURE="%TARGET_ARCHITECTURE%" /DVERSION_NUMBER="%VERSION_CODE%.0" /DCOMPANY_NAME="%COMPANY_NAME%" /DWEBSITE="%WEBSITE%" /Dapp_revision="%GIT_REV%" /Dapp_branch="%BRANCH%" /D%TARGET_ARCHITECTURE% "genNsisInstaller.nsi"
  IF NOT EXIST "%APP_SETUPFILE%" (
    POPD
    set DIETEXT=Failed to create %APP_SETUPFILE%. NSIS installed?
    goto DIE
  )
  copy %PDB% %APP_PDBFILE% > nul
  ECHO ------------------------------------------------------------
  ECHO Done!
  ECHO Setup is located at %CD%\%APP_SETUPFILE%
  ECHO ------------------------------------------------------------
  POPD
  GOTO END

:MAKE_APPX
  set app_ext=msix
  set app_path=%base_dir%\project\UWPBuildSetup
  if not exist "%app_path%" mkdir %app_path%
  call %base_dir%\project\Win32BuildSetup\extract_git_rev.bat > NUL
  for /F %%a IN ('dir /B /S %WORKSPACE%\AppPackages ^| findstr /I /R "%APP_NAME%_.*_%TARGET_ARCHITECTURE%_%buildconfig%\.%app_ext%$"') DO (
    copy /Y %%a %app_path%\%APP_NAME%-%GIT_REV%-%BRANCH%-%TARGET_ARCHITECTURE%.%app_ext%
    copy /Y %%~dpna.cer %app_path%\%APP_NAME%-%GIT_REV%-%BRANCH%-%TARGET_ARCHITECTURE%.cer
    copy /Y %%~dpna.appxsym %app_path%\%APP_NAME%-%GIT_REV%-%BRANCH%-%TARGET_ARCHITECTURE%.appxsym
    goto END_APPX
  )
  rem Release builds don't have Release in it's name
  for /F %%a IN ('dir /B /S %WORKSPACE%\AppPackages ^| findstr /I /R "%APP_NAME%_.*_%TARGET_ARCHITECTURE%\.%app_ext%$"') DO (
    copy /Y %%a %app_path%\%APP_NAME%-%GIT_REV%-%BRANCH%-%TARGET_ARCHITECTURE%.%app_ext%
    copy /Y %%~dpna.cer %app_path%\%APP_NAME%-%GIT_REV%-%BRANCH%-%TARGET_ARCHITECTURE%.cer
    copy /Y %%~dpna.appxsym %app_path%\%APP_NAME%-%GIT_REV%-%BRANCH%-%TARGET_ARCHITECTURE%.appxsym
    goto END_APPX
  )

  rem apxx file has win32 instead of x86 in it's name
  if %TARGET_ARCHITECTURE%==x86 (
    for /F %%a IN ('dir /B /S %WORKSPACE%\AppPackages ^| findstr /I /R "%APP_NAME%_.*_win32_%buildconfig%\.%app_ext%$"') DO (
      copy /Y %%a %app_path%\%APP_NAME%-%GIT_REV%-%BRANCH%-%TARGET_ARCHITECTURE%.%app_ext%
      copy /Y %%~dpna.cer %app_path%\%APP_NAME%-%GIT_REV%-%BRANCH%-%TARGET_ARCHITECTURE%.cer
      copy /Y %%~dpna.appxsym %app_path%\%APP_NAME%-%GIT_REV%-%BRANCH%-%TARGET_ARCHITECTURE%.appxsym
      goto END_APPX
    )

    rem Release builds don't have Release in it's name
    for /F %%a IN ('dir /B /S %WORKSPACE%\AppPackages ^| findstr /I /R "%APP_NAME%_.*_win32\.%app_ext%$"') DO (
      copy /Y %%a %app_path%\%APP_NAME%-%GIT_REV%-%BRANCH%-%TARGET_ARCHITECTURE%.%app_ext%
      copy /Y %%~dpna.cer %app_path%\%APP_NAME%-%GIT_REV%-%BRANCH%-%TARGET_ARCHITECTURE%.cer
      copy /Y %%~dpna.appxsym %app_path%\%APP_NAME%-%GIT_REV%-%BRANCH%-%TARGET_ARCHITECTURE%.appxsym
      goto END_APPX
    )
  )

:END_APPX
  ECHO ------------------------------------------------------------
  ECHO Done!
  ECHO Setup is located at %app_path%
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
  GOTO END

:END
  IF %promptlevel% NEQ noprompt (
    ECHO Press any key to exit...
    pause > NUL
  )
  EXIT /B %exitcode%
