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


title msys2

set msysver=20160205
set msys2=msys64
set build32=yes
set build64=no
set instdir=%CD%
set msyspackages=autoconf automake libtool m4 make gettext patch pkg-config wget p7zip coreutils
set mingwpackages=dlfcn gcc gcc-libs gettext libiconv libgpg-error libpng yasm nettle libtasn1 openssl xz
set locals32=gnutls-3.4.14-static
set locals64=gnutls-3.4.14-static
set usemirror=yes
set opt=mintty

:: if KODI_MIRROR is not set externally to this script, set it to the default mirror URL
if "%KODI_MIRROR%"=="" set KODI_MIRROR=http://mirrors.kodi.tv
if "%usemirror%"=="yes" (
    echo -------------------------------------------------------------------------------
    echo. Downloading will be performed from mirror %KODI_MIRROR%
    echo -------------------------------------------------------------------------------
    set MSYS_MIRROR=%KODI_MIRROR%/build-deps/win32/msys2
)

set downloaddir=%instdir%\downloads2
set unpack_exe=%instdir%\..\Win32BuildSetup\tools\7z\7za.exe

for %%b in (%1, %2, %3) do (
  if %%b==build64 (set build64=yes)
  if %%b==sh (set opt=sh)
)

:: use 32bit msys2 on x86 machine
if %PROCESSOR_ARCHITECTURE%=="x86" set msys2=msys32
if %msys2%==msys32 (set arch=i686) else (set arch=x86_64)
set msysfile=msys2-base-%arch%-%msysver%.tar.xz
if %opt%==mintty (
    set sh=%instdir%\%msys2%\usr\bin\mintty.exe -d -i /msys2.ico /usr/bin/bash
) else (
    set sh=%instdir%\%msys2%\usr\bin\sh.exe
)

::------------------------------------------------------------------
::download and install basic msys2 system:
::------------------------------------------------------------------
if exist "%instdir%\%msys2%\msys2_shell.bat" GOTO minttySettings
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

    set msysurl=http://sourceforge.net/projects/msys2/files/Base/%arch%/%msysfile%/download
    if %usemirror%==yes (
        ::download msys2 from our mirror
        set msysurl=%MSYS_MIRROR%/%msysfile%
    )
    %instdir%\bin\wget --tries=20 --retry-connrefused --waitretry=2 --no-check-certificate -c -O %downloaddir%\%msysfile% %msysurl%
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
    echo.- Install msys2 basic system
    echo -------------------------------------------------------------------------------
	%unpack_exe% x %downloaddir%\%msysfile% -so | %unpack_exe% x -aoa -si -ttar -o%instdir%
	)
	
if not exist %instdir%\%msys2%\usr\bin\msys-2.0.dll (
	echo -------------------------------------------------------------------------------
	echo.- Install msys2 basic system failed,
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
        echo.Font=DejaVu Sans Mono
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
                echo %%a | find /i "server = http://repo.msys2.org/">nul && (
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
if %build32%==yes (
    if not exist %instdir%\build mkdir %instdir%\build
    if not exist %instdir%\downloads2 mkdir %instdir%\downloads2
    if not exist %instdir%\local32\share (
        echo.-------------------------------------------------------------------------------
        echo.create local32 folders
        echo.-------------------------------------------------------------------------------
        mkdir %instdir%\local32
        mkdir %instdir%\local32\bin
        mkdir %instdir%\local32\etc
        mkdir %instdir%\local32\include
        mkdir %instdir%\local32\lib
        mkdir %instdir%\local32\lib\pkgconfig
        mkdir %instdir%\local32\share
        )
    )

if %build64%==yes (
    if not exist %instdir%\build mkdir %instdir%\build
    if not exist %instdir%\downloads2 mkdir %instdir%\downloads2
    if not exist %instdir%\local64\share (
        echo.-------------------------------------------------------------------------------
        echo.create local64 folders
        echo.-------------------------------------------------------------------------------
        mkdir %instdir%\local64
        mkdir %instdir%\local64\bin
        mkdir %instdir%\local64\etc
        mkdir %instdir%\local64\include
        mkdir %instdir%\local64\lib
        mkdir %instdir%\local64\lib\pkgconfig
        mkdir %instdir%\local64\share
        )
    )

if %build32%==yes (
    set searchStr=local32
    ) else (
        set searchStr=local64
        )

if not exist %instdir%\%msys2%\etc\fstab. GOTO writeFstab

for /f "tokens=2 delims=/" %%b in ('findstr /i build32 %instdir%\%msys2%\etc\fstab.') do set searchRes=oldbuild

if "%searchRes%"=="oldbuild" (
    del %instdir%\%msys2%\etc\fstab.
    GOTO writeFstab
    )

for /f "tokens=2 delims=/" %%a in ('findstr /i %searchStr% %instdir%\%msys2%\etc\fstab.') do set searchRes=%%a

if "%searchRes%"=="local32" GOTO installbase
if "%searchRes%"=="local64" GOTO installbase

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
        echo.%instdir%\build\           /build
        echo.%instdir%\local32\         /local32
        echo.%instdir%\local64\         /local64
        echo.%instdir%\%msys2%\mingw32\ /mingw32
        echo.%instdir%\%msys2%\mingw64\ /mingw64
        echo.%instdir%\downloads2\      /var/cache/pacman/pkg
        echo.%instdir%\..\..\           /xbmc
        )>>%instdir%\%msys2%\etc\fstab.

:installbase
if exist "%instdir%\%msys2%\etc\pac-base-old.pk" del "%instdir%\%msys2%\etc\pac-base-old.pk"
if exist "%instdir%\%msys2%\etc\pac-base-new.pk" ren "%instdir%\%msys2%\etc\pac-base-new.pk" pac-base-old.pk

for %%i in (%msyspackages%) do echo.%%i>>%instdir%\%msys2%\etc\pac-base-new.pk

if exist %instdir%\%msys2%\usr\bin\make.exe GOTO getmingw32
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
    %sh% --login %instdir%\pacman.sh &
    del %instdir%\pacman.sh

    for %%i in (%instdir%\%msys2%\usr\ssl\cert.pem) do (
        if %%~zi==0 (
            echo.update-ca-trust>>cert.sh
            echo.sleep ^3>>cert.sh
            echo.exit>>cert.sh
            %sh% --login %instdir%\cert.sh
            del cert.sh
            )
        )

:getmingw32
if %build32%==yes (
if exist "%instdir%\%msys2%\etc\pac-mingw32-old.pk" del "%instdir%\%msys2%\etc\pac-mingw32-old.pk"
if exist "%instdir%\%msys2%\etc\pac-mingw32-new.pk" ren "%instdir%\%msys2%\etc\pac-mingw32-new.pk" pac-mingw32-old.pk

for %%i in (%mingwpackages%) do echo.mingw-w64-i686-%%i>>%instdir%\%msys2%\etc\pac-mingw32-new.pk

if exist %instdir%\%msys2%\mingw32\bin\gcc.exe GOTO getmingw64
    echo.-------------------------------------------------------------------------------
    echo.install 32 bit compiler
    echo.-------------------------------------------------------------------------------
    if exist %instdir%\mingw32.sh del %instdir%\mingw32.sh
    (
        echo.echo -ne "\033]0;install 32 bit compiler\007"
        echo.pacman --noconfirm -S $(cat /etc/pac-mingw32-new.pk ^| sed -e 's#\\##'^)
        echo.sleep ^3
        echo.exit
        )>>%instdir%\mingw32.sh
    %sh% --login %instdir%\mingw32.sh
    del %instdir%\mingw32.sh
    )

:getmingw64
if %build64%==yes (
if exist "%instdir%\%msys2%\etc\pac-mingw64-old.pk" del "%instdir%\%msys2%\etc\pac-mingw64-old.pk"
if exist "%instdir%\%msys2%\etc\pac-mingw64-new.pk" ren "%instdir%\%msys2%\etc\pac-mingw64-new.pk" pac-mingw64-old.pk

for %%i in (%mingwpackages%) do echo.mingw-w64-x86_64-%%i>>%instdir%\%msys2%\etc\pac-mingw64-new.pk

if exist %instdir%\%msys2%\mingw64\bin\gcc.exe GOTO rebase2
    echo.-------------------------------------------------------------------------------
    echo.install 64 bit compiler
    echo.-------------------------------------------------------------------------------
    if exist %instdir%\mingw64.sh del %instdir%\mingw64.sh
        (
        echo.echo -ne "\033]0;install 64 bit compiler\007"
        echo.pacman --noconfirm -S $(cat /etc/pac-mingw64-new.pk ^| sed -e 's#\\##'^)
        echo.sleep ^3
        echo.exit
        )>>%instdir%\mingw64.sh
    %sh% --login %instdir%\mingw64.sh
    del %instdir%\mingw64.sh
    )

:rebase2
if %msys2%==msys32 (
    echo.-------------------------------------------------------------------------------
    echo.second rebase msys32 system
    echo.-------------------------------------------------------------------------------
    call %instdir%\msys32\autorebase.bat
    )

:checkdyn
echo.-------------------------------------------------------------------------------
echo.check for dynamic libs
echo.-------------------------------------------------------------------------------

Setlocal EnableDelayedExpansion

if %build32%==yes (
if exist %instdir%\%msys2%\mingw32\lib\xvidcore.dll.a (
    del %instdir%\%msys2%\mingw32\bin\xvidcore.dll
    %instdir%\%msys2%\usr\bin\mv %instdir%\%msys2%\mingw32\lib\xvidcore.a %instdir%\%msys2%\mingw32\lib\libxvidcore.a
    %instdir%\%msys2%\usr\bin\mv %instdir%\%msys2%\mingw32\lib\xvidcore.dll.a %instdir%\%msys2%\mingw32\lib\xvidcore.dll.a.dyn
    )

    FOR /R "%instdir%\%msys2%\mingw32" %%C IN (*.dll.a) DO (
        set file=%%C
        set name=!file:~0,-6!
        if exist %%C.dyn del %%C.dyn
        if exist !name!.a (
            %instdir%\%msys2%\usr\bin\mv %%C %%C.dyn
            )
        )
    )

if %build64%==yes (
if exist %instdir%\%msys2%\mingw64\lib\xvidcore.dll.a (
    del %instdir%\%msys2%\mingw64\bin\xvidcore.dll
    %instdir%\%msys2%\usr\bin\mv %instdir%\%msys2%\mingw64\lib\xvidcore.a %instdir%\%msys2%\mingw64\lib\libxvidcore.a
    %instdir%\%msys2%\usr\bin\mv %instdir%\%msys2%\mingw64\lib\xvidcore.dll.a %instdir%\%msys2%\mingw64\lib\xvidcore.dll.a.dyn
    )

    FOR /R "%instdir%\%msys2%\mingw64" %%C IN (*.dll.a) DO (
        set file=%%C
        set name=!file:~0,-6!
        if exist %%C.dyn del %%C.dyn
        if exist !name!.a (
            %instdir%\%msys2%\usr\bin\mv %%C %%C.dyn
            )
        )
    )

Setlocal DisableDelayedExpansion

::------------------------------------------------------------------
:: write config profiles:
::------------------------------------------------------------------

:writeProfile32
if %build32%==yes (
    if exist %instdir%\local32\etc\profile.local GOTO writeProfile64
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
            echo.
            echo.BITS='32bit'
            echo.export BITS
            )>>%instdir%\local32\etc\profile.local
        )

:writeProfile64
if %build64%==yes (
    if exist %instdir%\local64\etc\profile.local GOTO loginProfile
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
            echo.
            echo.BITS='64bit'
            echo.export BITS
            )>>%instdir%\local64\etc\profile.local
        )

:loginProfile
if %build32%==no GOTO loginProfile64
    %instdir%\%msys2%\usr\bin\grep -q -e 'profile.local' %instdir%\%msys2%\etc\profile || (
        echo -------------------------------------------------------------------------------
        echo.- write default profile [32 bit]
        echo -------------------------------------------------------------------------------
        (
            echo.
            echo.if [[ -z "$MSYSTEM" ^&^& -f /local32/etc/profile.local ]]; then
            echo.       source /local32/etc/profile.local
            echo.fi
            )>>%instdir%\%msys2%\etc\profile.
    )

    GOTO loadlocals32

:loginProfile64
    %instdir%\%msys2%\usr\bin\grep -q -e 'profile.local' %instdir%\%msys2%\etc\profile || (
        echo -------------------------------------------------------------------------------
        echo.- write default profile [64 bit]
        echo -------------------------------------------------------------------------------
        (
            echo.
            echo.if [[ -z "$MSYSTEM" ^&^& -f /local64/etc/profile.local ]]; then
            echo.       source /local64/etc/profile.local
            echo.fi
            )>>%instdir%\%msys2%\etc\profile.
    )

:loadlocals32
if "%MSYS_MIRROR%" == "" goto end
set pkgbaseurl=%MSYS_MIRROR%/locals

echo.-------------------------------------------------------------------------------
echo.Download precompiled libs
echo.-------------------------------------------------------------------------------

if %build32%==no goto loadlocal64
for %%i in (%locals32%) do (
    setlocal EnableDelayedExpansion
    set pkgfile=i686-%%i.tar.gz
    if not exist "%downloaddir%\!pkgfile!" (
      %instdir%\bin\wget --tries=20 --retry-connrefused --waitretry=2 --no-check-certificate -c -O %downloaddir%\!pkgfile! %pkgbaseurl%/!pkgfile!
    )
    if errorlevel == 1 del "%downloaddir%\!pkgfile!"
    if exist "%downloaddir%\!pkgfile!" (
      %unpack_exe% x %downloaddir%\!pkgfile! -so | %unpack_exe% x -aoa -si -ttar -o%instdir%
    )
    endlocal
  )

:loadlocal64
if %build64%==no goto end
for %%i in (%locals64%) do (
    setlocal EnableDelayedExpansion
    set pkgfile=x86_64-%%i.tar.gz
    if not exist "%downloaddir%\!pkgfile!" (
      %instdir%\bin\wget --tries=20 --retry-connrefused --waitretry=2 --no-check-certificate -c -O %downloaddir%\!pkgfile! %pkgbaseurl%/!pkgfile!
    )
    if errorlevel == 1 del "%downloaddir%\!pkgfile!"
    if exist "%downloaddir%\!pkgfile!" (
      %unpack_exe% x %downloaddir%\!pkgfile! -so | %unpack_exe% x -aoa -si -ttar -o%instdir%
    )
    endlocal
  )

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
