@echo off
echo ---+++--- Building Vorbisfile (Dynamic) ---+++---

if .%SRCROOT%==. set SRCROOT=i:\xiph
           
set OLDPATH=%PATH%
set OLDINCLUDE=%INCLUDE%
set OLDLIB=%LIB%

call "c:\program files\microsoft visual studio\vc98\bin\vcvars32.bat"
echo Setting include/lib paths for Vorbis
set INCLUDE=%INCLUDE%;%SRCROOT%\ogg\include;%SRCROOT%\vorbis\include
set LIB=%LIB%;%SRCROOT%\ogg\win32\Dynamic_Debug;%SRCROOT%\vorbis\win32\Dynamic_Debug
echo Compiling...
msdev vorbisfile_dynamic.dsp /useenv /make "vorbisfile_dynamic - Win32 Debug" /rebuild

set PATH=%OLDPATH%
set INCLUDE=%OLDINCLUDE%
set LIB=%OLDLIB%
