@ECHO OFF

REM This file does an interactive configuration for users of DOS and Windows.
REM It creates a config.txt file for inclusion in the Makefile. This batch
REM file should be run indirectly through the 'make config' target (or the
REM 'make' target the first time).

IF EXIST make\dumbask.exe GOTO dumbaskok
ECHO You should not be running this directly! Use 'make' or 'make config'.
GOTO end
:dumbaskok

make\dumbask.exe "Would you like to compile DUMB for DJGPP or MinGW (D/M)? " DM
IF ERRORLEVEL 1 GOTO mingw
ECHO include make/djgpp.inc>make\config.tmp
GOTO djgpp
:mingw
ECHO include make/mingw.inc>make\config.tmp
:djgpp

ECHO ALL_TARGETS := core core-examples core-headers>>make\config.tmp

make\dumbask.exe "Would you like support for Allegro (Y/N)? "
IF NOT ERRORLEVEL 1 ECHO ALL_TARGETS += allegro allegro-examples allegro-headers>>make\config.tmp

IF EXIST make\config.txt DEL make\config.txt
REN make\config.tmp config.txt

ECHO Configuration complete.
ECHO Run 'make config' to change it in the future.
PAUSE

:end
