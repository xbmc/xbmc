@echo off
rem java why you so annoying?
set path=%path:"C:\Program Files (x86)\Java\jre7\bin"=%

call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat"

rd /s /q c:\tmp
rd /s /q upload
md upload

call plex\scripts\fetch-depends-windows.bat

rd /s /q build-windows-i386
md build-windows-i386
cd build-windows-i386

cmake -GNinja -DCMAKE_INSTALL_PREFIX=output -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
ninja release_package

move c:\tmp\PlexHomeTheater*exe %WORKSPACE%\upload
move PlexHomeTheater*7z %WORKSPACE%\upload
move PlexHomeTheater*symserv*zip %WORKSPACE%\upload
move c:\tmp\PlexHomeTheater*zip %WORKSPACE%\upload
move c:\tmp\PlexHomeTheater*manifest.xml %WORKSPACE%\upload
