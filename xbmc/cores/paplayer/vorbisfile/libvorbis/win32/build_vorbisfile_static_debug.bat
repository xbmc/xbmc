@echo off
echo ---+++--- Building Vorbisfile (Static) ---+++---

if .%SRCROOT%==. set SRCROOT=i:\xiph

set OLDPATH=%PATH%
set OLDINCLUDE=%INCLUDE%
set OLDLIB=%LIB%

call "c:\program files\microsoft visual studio\vc98\bin\vcvars32.bat"
echo Setting include/lib paths for Vorbis
set INCLUDE=%INCLUDE%;%SRCROOT%\ogg\include;%SRCROOT%\vorbis\include
set LIB=%LIB%;%SRCROOT%\ogg\win32\Static_Debug;%SRCROOT%\vorbis\win32\Dynamic_Debug
echo Compiling...
msdev vorbisfile_static.dsp /useenv /make "vorbisfile_static - Win32 Debug" /rebuild

set PATH=%OLDPATH%
set INCLUDE=%OLDINCLUDE%
set LIB=%OLDLIB%
