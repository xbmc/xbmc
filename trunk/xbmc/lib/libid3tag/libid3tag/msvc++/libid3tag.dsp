# Microsoft Developer Studio Project File - Name="libid3tag" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libid3tag - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libid3tag.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libid3tag.mak" CFG="libid3tag - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libid3tag - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libid3tag - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libid3tag - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /GX /O2 /I "." /I "..\..\libz" /I "..\..\libz-1.1.4" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "HAVE_CONFIG_H" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libid3tag - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /Gm /GX /ZI /Od /I "." /I "..\..\libz" /I "..\..\libz-1.1.4" /D "_LIB" /D "HAVE_CONFIG_H" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "DEBUG" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libid3tag - Win32 Release"
# Name "libid3tag - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "c"
# Begin Source File

SOURCE=..\compat.c
# End Source File
# Begin Source File

SOURCE=..\crc.c
# End Source File
# Begin Source File

SOURCE=..\debug.c
# End Source File
# Begin Source File

SOURCE=..\field.c
# End Source File
# Begin Source File

SOURCE=..\file.c
# End Source File
# Begin Source File

SOURCE=..\frame.c
# End Source File
# Begin Source File

SOURCE=..\frametype.c
# End Source File
# Begin Source File

SOURCE=..\genre.c
# End Source File
# Begin Source File

SOURCE=..\latin1.c
# End Source File
# Begin Source File

SOURCE=..\parse.c
# End Source File
# Begin Source File

SOURCE=..\render.c
# End Source File
# Begin Source File

SOURCE=..\tag.c
# End Source File
# Begin Source File

SOURCE=..\ucs4.c
# End Source File
# Begin Source File

SOURCE=..\utf16.c
# End Source File
# Begin Source File

SOURCE=..\utf8.c
# End Source File
# Begin Source File

SOURCE=..\util.c
# End Source File
# Begin Source File

SOURCE=..\version.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=..\compat.h
# End Source File
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=..\crc.h
# End Source File
# Begin Source File

SOURCE=..\debug.h
# End Source File
# Begin Source File

SOURCE=..\field.h
# End Source File
# Begin Source File

SOURCE=..\file.h
# End Source File
# Begin Source File

SOURCE=..\frame.h
# End Source File
# Begin Source File

SOURCE=..\frametype.h
# End Source File
# Begin Source File

SOURCE=..\genre.h
# End Source File
# Begin Source File

SOURCE=..\global.h
# End Source File
# Begin Source File

SOURCE=..\id3tag.h
# End Source File
# Begin Source File

SOURCE=..\latin1.h
# End Source File
# Begin Source File

SOURCE=..\parse.h
# End Source File
# Begin Source File

SOURCE=..\render.h
# End Source File
# Begin Source File

SOURCE=..\tag.h
# End Source File
# Begin Source File

SOURCE=..\ucs4.h
# End Source File
# Begin Source File

SOURCE=..\utf16.h
# End Source File
# Begin Source File

SOURCE=..\utf8.h
# End Source File
# Begin Source File

SOURCE=..\util.h
# End Source File
# Begin Source File

SOURCE=..\version.h
# End Source File
# End Group
# Begin Group "Data Files"

# PROP Default_Filter "dat"
# Begin Source File

SOURCE=..\genre.dat
# End Source File
# End Group
# End Target
# End Project
