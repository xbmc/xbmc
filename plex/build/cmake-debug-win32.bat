@set VSINSTALLDIR=C:\Program Files (x86)\Microsoft Visual Studio 10.0\
@set VCINSTALLDIR=%VSINSTALLDIR%VC\
@set WindowsSdkDir=%ProgramFiles(x86)%\Microsoft SDKs\Windows\v7.0A\
@set DxFramework=%ProgramFiles(x86)%\Microsoft DirectX SDK (June 2010)\

@rem
@rem Root of Visual Studio IDE installed files.
@rem
@set DevEnvDir=%VSINSTALLDIR%Common7\IDE\


@if exist "%ProgramFiles%\HTML Help Workshop" set PATH=%ProgramFiles%\HTML Help Workshop;%PATH%
@if exist "%ProgramFiles(x86)%\HTML Help Workshop" set PATH=%ProgramFiles(x86)%\HTML Help Workshop;%PATH%
@if exist "%VCINSTALLDIR%VCPackages" set PATH=%VCINSTALLDIR%VCPackages;%PATH%
@set PATH=%FrameworkDir%%Framework35Version%;%PATH%
@set PATH=%FrameworkDir%%FrameworkVersion%;%PATH%
@set PATH=%VSINSTALLDIR%Common7\Tools;%PATH%
@if exist "%VCINSTALLDIR%BIN" set PATH=%VCINSTALLDIR%BIN;%PATH%
@set PATH=%DevEnvDir%;%PATH%

@if exist "%VSINSTALLDIR%VSTSDB\Deploy" set PATH=%VSINSTALLDIR%VSTSDB\Deploy;%PATH%

@if not "%FSHARPINSTALLDIR%" == "" set PATH=%FSHARPINSTALLDIR%;%PATH%

@rem INCLUDE
@rem -------
@if exist "%VCINSTALLDIR%ATLMFC\INCLUDE" set INCLUDE=%VCINSTALLDIR%ATLMFC\INCLUDE;%INCLUDE%
@if exist "%VCINSTALLDIR%INCLUDE" set INCLUDE=%VCINSTALLDIR%INCLUDE;%INCLUDE%
@if exist "%DxFramework%INCLUDE" set INCLUDE=%DxFramework%INCLUDE;%INCLUDE%

@rem LIB
@rem ---
@if exist "%VCINSTALLDIR%ATLMFC\LIB" set LIB=%VCINSTALLDIR%ATLMFC\LIB;%LIB%
@if exist "%VCINSTALLDIR%LIB" set LIB=%VCINSTALLDIR%LIB;%LIB%
@if exist "%DxFramework%LIB" set LIB=%DxFramework%LIB;%LIB%

@rem LIBPATH
@rem -------
@if exist "%VCINSTALLDIR%ATLMFC\LIB" set LIBPATH=%VCINSTALLDIR%ATLMFC\LIB;%LIBPATH%
@if exist "%VCINSTALLDIR%LIB" set LIBPATH=%VCINSTALLDIR%LIB;%LIBPATH%
@set LIBPATH=%FrameworkDir%%Framework35Version%;%LIBPATH%
@set LIBPATH=%FrameworkDir%%FrameworkVersion%;%LIBPATH%

@set PATH=%WindowsSdkDir%bin\NETFX 4.0 Tools;%WindowsSdkDir%bin;%PATH%
@set INCLUDE=%WindowsSdkDir%include;%INCLUDE%
@set LIB=%WindowsSdkDir%lib;%LIB%

@rem CMake variables
@set PATH=C:\Plex;%PATH%
