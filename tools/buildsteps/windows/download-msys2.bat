::-------------------------------------------------------------------------------------
:: LICENSE -------------------------------------------------------------------------
::-------------------------------------------------------------------------------------
::  This Windows Batchscript is for setup a compiler environment for building ffmpeg and other media tools under Windows.
::
::    Copyright (C) 2013  jb_alvarado
::
::    This program is free software: you can redistribute it and/or modify
::    it under the terms of the GNU General Public License as published by
::    the Free Software Foundation, either version 3 of the License, or
::    (at your option) any later version.
::
::    This program is distributed in the hope that it will be useful,
::    but WITHOUT ANY WARRANTY; without even the implied warranty of
::    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
::    GNU General Public License for more details.
::
::    You should have received a copy of the GNU General Public License
::    along with this program.  If not, see <http://www.gnu.org/licenses/>.
::-------------------------------------------------------------------------------------

@echo off
title msys2

SETLOCAL EnableDelayedExpansion

PUSHD %~dp0\..\..\..
SET WORKSPACE=%CD%
POPD

set msysver=20231026
set msys2=msys64
set arch=x86_64
set instdir=%WORKSPACE%\project\BuildDependencies
set msyspackages=diffutils gcc make patch perl tar yasm
set gaspreprocurl=https://github.com/FFmpeg/gas-preprocessor/archive/master.tar.gz
set usemirror=yes
set opt=mintty

:: if KODI_MIRROR is not set externally to this script, set it to the default mirror URL
if "%KODI_MIRROR%"=="" set KODI_MIRROR=https://mirrors.kodi.tv
if "%usemirror%"=="yes" (
    echo -------------------------------------------------------------------------------
    echo. Downloading will be performed from mirror %KODI_MIRROR%
    echo -------------------------------------------------------------------------------
    set MSYS_MIRROR=%KODI_MIRROR%/build-deps/win32/msys2
)

set downloaddir=%instdir%\downloads2
set unpack_exe=%instdir%\..\Win32BuildSetup\tools\7z\7za.exe

for %%b in (%*) do (
  if %%b==sh (set opt=sh)
)

:: msys2 announced end of 32bit active support on 2020-05-17
if %PROCESSOR_ARCHITECTURE%=="x86" (
	echo ERROR: msys2 is not available for 32bit OS
        exit /B 1
)
set msysfile=msys2-base-%arch%-%msysver%.tar.xz
if %opt%==mintty (
    set sh=%instdir%\%msys2%\usr\bin\mintty.exe -d -i /msys2.ico /usr/bin/bash
) else (
    set sh=%instdir%\%msys2%\usr\bin\sh.exe
)

::------------------------------------------------------------------
::download and install basic msys2 system:
::------------------------------------------------------------------
if exist "%instdir%\%msys2%\msys2_shell.cmd" GOTO minttySettings
	if not exist %downloaddir% mkdir %downloaddir%

:download
if exist "%downloaddir%\%msysfile%" (
    setlocal EnableDelayedExpansion
    for /F "tokens=*" %%A in ("%downloaddir%\%msysfile%") do set fileSize=%%~zA
    if !fileSize!==0 del %downloaddir%\%msysfile%
    endlocal
    )

if exist "%downloaddir%\%msysfile%" GOTO unpack
    echo -------------------------------------------------------------------------------
    echo.- Download msys2 basic system (Kodi mirrors: %usemirror%)
    echo -------------------------------------------------------------------------------

    set msysurl=https://repo.msys2.org/distrib/%arch%/%msysfile%
    if %usemirror%==yes (
        ::download msys2 from our mirror
        set msysurl=%MSYS_MIRROR%/%msysfile%
    )
    curl --retry 5 --retry-all-errors --retry-connrefused --retry-delay 5 --location --output %downloaddir%\%msysfile% %msysurl%
    if errorlevel == 1 (
        if exist "%downloaddir%\%msysfile%" del %downloaddir%\%msysfile%
        if %usemirror%==yes (
            set usemirror=no
            goto download
        )
        echo ERROR: Unable to download msys2!
        exit /B 1
    )

:unpack
if exist "%downloaddir%\%msysfile%" (
    echo -------------------------------------------------------------------------------
    echo.- Installing msys2 basic system
    echo -------------------------------------------------------------------------------
	%unpack_exe% x %downloaddir%\%msysfile% -so 2>NUL | %unpack_exe% x -aoa -si -ttar -o%instdir% >NUL 2>NUL
	)

if not exist %instdir%\%msys2%\usr\bin\msys-2.0.dll (
	echo -------------------------------------------------------------------------------
	echo.- Installing msys2 basic system failed,
	echo -------------------------------------------------------------------------------
	exit /B 1
	)

:minttySettings
if exist "%instdir%\%msys2%\home\%USERNAME%\.minttyrc" GOTO updatemirrors
if not exist "%instdir%\%msys2%\home\%USERNAME%" mkdir "%instdir%\%msys2%\home\%USERNAME%"
    (
        echo.BoldAsFont=no
        echo.BackgroundColour=57,57,57
        echo.ForegroundColour=221,221,221
        echo.Transparency=medium
        echo.FontHeight=^9
        echo.FontSmoothing=full
        echo.AllowBlinking=yes
        echo.Columns=120
        echo.Rows=30
        echo.Term=xterm-256color
        echo.CursorType=block
        echo.ClicksPlaceCursor=yes
        echo.Black=38,39,41
        echo.Red=249,38,113
        echo.Green=166,226,46
        echo.Yellow=253,151,31
        echo.Blue=102,217,239
        echo.Magenta=158,111,254
        echo.Cyan=94,113,117
        echo.White=248,248,242
        echo.BoldBlack=85,68,68
        echo.BoldRed=249,38,113
        echo.BoldGreen=166,226,46
        echo.BoldYellow=253,151,31
        echo.BoldBlue=102,217,239
        echo.BoldMagenta=158,111,254
        echo.BoldCyan=163,186,191
        echo.BoldWhite=248,248,242
        )>>"%instdir%\%msys2%\home\%USERNAME%\.minttyrc"

:updatemirrors
if not "%usemirror%"=="yes" GOTO rebase
    echo.-------------------------------------------------------------------------------
    echo.update pacman mirrors
    echo.-------------------------------------------------------------------------------
    setlocal EnableDelayedExpansion

    for %%f in (msys,mingw32,mingw64) do (
        set filename=%instdir%\%msys2%\etc\pacman.d\mirrorlist.%%f
        set oldfile=!filename!.old
        if not exist !oldfile! if exist !filename! (
            set mirror=%MSYS_MIRROR%/repos/%%f
            if %%f==msys set mirror=!mirror!2/$arch
            move !filename! !oldfile!>nul
            for /F "usebackq delims=" %%a in (!oldfile!) do (
                echo %%a | find /i "server = https://mirror.msys2.org/">nul && (
                    echo.Server = !mirror!
                    )>>!filename!
                echo %%a>>!filename!
                )
            )
        )
    endlocal

:rebase
if %msys2%==msys32 (
    echo.-------------------------------------------------------------------------------
    echo.rebase msys32 system
    echo.-------------------------------------------------------------------------------
    call %instdir%\msys32\autorebase.bat
    )

:preparedirs
if not exist %instdir%\build mkdir %instdir%\build
if not exist %instdir%\downloads2 mkdir %instdir%\downloads2
if not exist %instdir%\locals mkdir %instdir%\locals
if not exist %instdir%\locals\win32 mkdir %instdir%\locals\win32
if not exist %instdir%\locals\x64 mkdir %instdir%\locals\x64

if not exist %instdir%\locals\win32\share (
    echo.-------------------------------------------------------------------------------
    echo.create local win32 folders
    echo.-------------------------------------------------------------------------------
    mkdir %instdir%\locals\win32\bin
    mkdir %instdir%\locals\win32\etc
    mkdir %instdir%\locals\win32\include
    mkdir %instdir%\locals\win32\lib
    mkdir %instdir%\locals\win32\lib\pkgconfig
    mkdir %instdir%\locals\win32\share
    )

if not exist %instdir%\locals\x64\share (
    echo.-------------------------------------------------------------------------------
    echo.create local x64 folders
    echo.-------------------------------------------------------------------------------
    mkdir %instdir%\locals\x64\bin
    mkdir %instdir%\locals\x64\etc
    mkdir %instdir%\locals\x64\include
    mkdir %instdir%\locals\x64\lib
    mkdir %instdir%\locals\x64\lib\pkgconfig
    mkdir %instdir%\locals\x64\share
    )

if not exist %instdir%\%msys2%\etc\fstab. GOTO writeFstab

set searchRes=
for /f "tokens=2 delims=/" %%a in ('findstr /i xbmc %instdir%\%msys2%\etc\fstab.') do set searchRes=%%a
if "%searchRes%"=="xbmc" GOTO installbase

:writeFstab
echo -------------------------------------------------------------------------------
echo.- write fstab mount file
echo -------------------------------------------------------------------------------
set cygdrive=no
if exist %instdir%\%msys2%\etc\fstab. (
    for /f %%b in ('findstr /i binary %instdir%\%msys2%\etc\fstab.') do set cygdrive=yes
    )
if "%cygdrive%"=="no" echo.none / cygdrive binary,posix=0,noacl,user 0 ^0>>%instdir%\%msys2%\etc\fstab.
(
    echo.
    echo.%instdir%\build\            /build
    echo.%instdir%\downloads\        /downloads
    echo.%instdir%\locals\win32\     /local32
    echo.%instdir%\locals\x64\       /local64
    echo.%instdir%\%msys2%\mingw32\  /mingw32
    echo.%instdir%\%msys2%\mingw64\  /mingw64
    echo.%instdir%\downloads2\       /var/cache/pacman/pkg
    echo.%instdir%\win32\            /depends/win32
    echo.%instdir%\x64\              /depends/x64
    echo.%instdir%\win10-arm\        /depends/win10-arm
    echo.%instdir%\win10-win32\      /depends/win10-win32
    echo.%instdir%\win10-x64\        /depends/win10-x64
    echo.%instdir%\..\..\            /xbmc
)>>%instdir%\%msys2%\etc\fstab.

:installbase
if exist "%instdir%\%msys2%\etc\pac-base-old.pk" del "%instdir%\%msys2%\etc\pac-base-old.pk"
if exist "%instdir%\%msys2%\etc\pac-base-new.pk" ren "%instdir%\%msys2%\etc\pac-base-new.pk" pac-base-old.pk

for %%i in (%msyspackages%) do echo.%%i>>%instdir%\%msys2%\etc\pac-base-new.pk

if exist %instdir%\%msys2%\usr\bin\make.exe GOTO rebase2
    echo.-------------------------------------------------------------------------------
    echo.install msys2 base system
    echo.-------------------------------------------------------------------------------
    if exist %instdir%\pacman.sh del %instdir%\pacman.sh
    (
    echo.echo -ne "\033]0;install base system\007"
    echo.pacman --noconfirm -S $(cat /etc/pac-base-new.pk ^| sed -e 's#\\##'^)
    echo.sleep ^3
    echo.exit
        )>>%instdir%\pacman.sh
:: Unconventional msys2 post install steps:
:: %sh% -lc ' ' for first-run msys2 actions is replaced by --login for the first script execution.
:: To control the used versions, %sh% pacman --noconfirm -Syuu is not run twice for system update.
    %sh% --login %instdir%\pacman.sh &
    del %instdir%\pacman.sh

:rebase2
if %msys2%==msys32 (
    echo.-------------------------------------------------------------------------------
    echo.second rebase msys32 system
    echo.-------------------------------------------------------------------------------
    call %instdir%\msys32\autorebase.bat
    )

::------------------------------------------------------------------
:: write config profiles:
::------------------------------------------------------------------

:writeProfile32
if exist %instdir%\locals\win32\etc\profile.local GOTO writeProfile64
    echo -------------------------------------------------------------------------------
    echo.- write profile for 32 bit compiling
    echo -------------------------------------------------------------------------------
    (
        echo.#
        echo.# /local32/etc/profile.local
        echo.#
        echo.
        echo.MSYSTEM=MINGW32
        echo.
        echo.alias dir='ls -la --color=auto'
        echo.alias ls='ls --color=auto'
        echo.export CC=gcc
        echo.export python=/usr/bin/python
        echo.
        echo.MSYS2_PATH="/usr/local/bin:/usr/bin"
        echo.MANPATH="/usr/share/man:/mingw32/share/man:/local32/man:/local32/share/man"
        echo.INFOPATH="/usr/local/info:/usr/share/info:/usr/info:/mingw32/share/info"
        echo.MINGW_PREFIX="/mingw32"
        echo.MINGW_CHOST="i686-w64-mingw32"
        echo.export MSYSTEM MINGW_PREFIX MINGW_CHOST
        echo.
        echo.DXSDK_DIR="/mingw32/i686-w64-mingw32"
        echo.ACLOCAL_PATH="/mingw32/share/aclocal:/usr/share/aclocal"
        echo.PKG_CONFIG_LOCAL_PATH="/local32/lib/pkgconfig"
        echo.PKG_CONFIG_PATH="/local32/lib/pkgconfig:/mingw32/lib/pkgconfig"
        echo.CPPFLAGS="-I/local32/include -D_FORTIFY_SOURCE=2"
        echo.CFLAGS="-I/local32/include -mms-bitfields -mthreads -mtune=generic -pipe"
        echo.CXXFLAGS="-I/local32/include -mms-bitfields -mthreads -mtune=generic -pipe"
        echo.LDFLAGS="-L/local32/lib -mthreads -pipe"
        echo.export DXSDK_DIR ACLOCAL_PATH PKG_CONFIG_PATH PKG_CONFIG_LOCAL_PATH CPPFLAGS CFLAGS CXXFLAGS LDFLAGS MSYSTEM
        echo.
        echo.PYTHONHOME=/usr
        echo.PYTHONPATH="/usr/lib/python2.7:/usr/lib/python2.7/Tools/Scripts"
        echo.
        echo.PATH=".:/local32/bin:/mingw32/bin:${MSYS2_PATH}:${INFOPATH}:${PYTHONHOME}:${PYTHONPATH}:${PATH}"
        echo.PS1='\[\033[32m\]\u@\h \[\e[33m\]\w\[\e[0m\]\n\$ '
        echo.export PATH PS1
        echo.
        echo.# package build directory
        echo.LOCALBUILDDIR=/build
        echo.# package installation prefix
        echo.LOCALDESTDIR=/local32
        echo.export LOCALBUILDDIR LOCALDESTDIR
        )>>%instdir%\locals\win32\etc\profile.local
    )

:writeProfile64
if exist %instdir%\locals\x64\etc\profile.local GOTO loadGasPreproc
    echo -------------------------------------------------------------------------------
    echo.- write profile for 64 bit compiling
    echo -------------------------------------------------------------------------------
    (
        echo.#
        echo.# /local64/etc/profile.local
        echo.#
        echo.
        echo.MSYSTEM=MINGW64
        echo.
        echo.alias dir='ls -la --color=auto'
        echo.alias ls='ls --color=auto'
        echo.export CC=gcc
        echo.export python=/usr/bin/python
        echo.
        echo.MSYS2_PATH="/usr/local/bin:/usr/bin"
        echo.MANPATH="/usr/share/man:/mingw64/share/man:/local64/man:/local64/share/man"
        echo.INFOPATH="/usr/local/info:/usr/share/info:/usr/info:/mingw64/share/info"
        echo.MINGW_PREFIX="/mingw64"
        echo.MINGW_CHOST="x86_64-w64-mingw32"
        echo.export MSYSTEM MINGW_PREFIX MINGW_CHOST
        echo.
        echo.DXSDK_DIR="/mingw64/x86_64-w64-mingw32"
        echo.ACLOCAL_PATH="/mingw64/share/aclocal:/usr/share/aclocal"
        echo.PKG_CONFIG_LOCAL_PATH="/local64/lib/pkgconfig"
        echo.PKG_CONFIG_PATH="/local64/lib/pkgconfig:/mingw64/lib/pkgconfig"
        echo.CPPFLAGS="-I/local64/include -D_FORTIFY_SOURCE=2"
        echo.CFLAGS="-I/local64/include -mms-bitfields -mthreads -mtune=generic -pipe"
        echo.CXXFLAGS="-I/local64/include -mms-bitfields -mthreads -mtune=generic -pipe"
        echo.LDFLAGS="-L/local64/lib -pipe"
        echo.export DXSDK_DIR ACLOCAL_PATH PKG_CONFIG_PATH PKG_CONFIG_LOCAL_PATH CPPFLAGS CFLAGS CXXFLAGS LDFLAGS MSYSTEM
        echo.
        echo.PYTHONHOME=/usr
        echo.PYTHONPATH="/usr/lib/python2.7:/usr/lib/python2.7/Tools/Scripts"
        echo.
        echo.PATH=".:/local64/bin:/mingw64/bin:${MSYS2_PATH}:${INFOPATH}:${PYTHONHOME}:${PYTHONPATH}:${PATH}"
        echo.PS1='\[\033[32m\]\u@\h \[\e[33m\]\w\[\e[0m\]\n\$ '
        echo.export PATH PS1
        echo.
        echo.# package build directory
        echo.LOCALBUILDDIR=/build
        echo.# package installation prefix
        echo.LOCALDESTDIR=/local64
        echo.export LOCALBUILDDIR LOCALDESTDIR
        )>>%instdir%\locals\x64\etc\profile.local
    )

:loadGasPreproc
set gaspreprocfile=gas-preprocessor.tar.gz
if exist %downloaddir%\%gaspreprocfile% goto extractGasPreproc
    echo -------------------------------------------------------------------------------
    echo.- Downloading gas-preprocessor.pl
    echo -------------------------------------------------------------------------------
    curl --retry 5 --retry-all-errors --retry-connrefused --retry-delay 5 --location --output %downloaddir%\%gaspreprocfile% %gaspreprocurl%

:extractGasPreproc
if exist %instdir%\%msys2%\usr\bin\gas-preprocessor.pl goto end
    echo -------------------------------------------------------------------------------
    echo.- Installing gas-preprocessor.pl
    echo -------------------------------------------------------------------------------
    %unpack_exe% x %downloaddir%\%gaspreprocfile% -so 2>NUL | %unpack_exe% e -si -ttar -o%instdir%\%msys2%\usr\bin *.pl -r >NUL 2>NUL

:end
cd %instdir%
IF ERRORLEVEL == 1 (
    ECHO Something goes wrong...
    exit /B 1
  )

echo.-------------------------------------------------------------------------------
echo.install msys2 system done
echo.-------------------------------------------------------------------------------

@echo on
