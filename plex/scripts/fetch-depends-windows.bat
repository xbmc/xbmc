@echo off
if "%WORKSPACE%"=="" (
	rem if we don't have a workspace let's just assume we are being executed from plex\scripts
	set WORKSPACE="%~dp0..\.."
)

set buildno=5
rem set sha1=

%WORKSPACE%\project\BuildDependencies\bin\wget.exe -O %WORKSPACE%\plex\Dependencies\windows-i386-xbmc-deps.tar.gz http://nightlies.plexapp.com/plex-dependencies/plex-home-theater-deps-windows/%buildno%/windows-i386-xbmc-deps.tar.gz
%WORKSPACE%\plex\scripts\tar.exe -C %WORKSPACE% -xzf %WORKSPACE%\plex\Dependencies\windows-i386-xbmc-deps.tar.gz