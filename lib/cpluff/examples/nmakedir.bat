@REM C-Pluff examples build system for MSVC
@REM Copyright 2007 Johannes Lehtinen
@REM This file is free software; Johannes Lehtinen gives unlimited permission
@REM to copy, distribute and modify it.

@echo Making %2 in %1
@cd %1
@for /f "" %%d in ('cd') do @echo Entering directory %%d
@nmake /nologo /f Makefile.nmake %2
@for /f "" %%d in ('cd') do @echo Leaving directory %%d
@cd ..
