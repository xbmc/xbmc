# Microsoft Developer Studio Project File - Name="libsidutils" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=libsidutils - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libsidutils.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libsidutils.mak" CFG="libsidutils - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libsidutils - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libsidutils - Win32 Debug" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libsidutils - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../include" /I "../../include/sidplay/utils" /I "../../../libsidplay/include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "DLL_EXPORT" /D "HAVE_MSWINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /out:"../../../bin_vc5/Release/libsidutils.dll"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "libsidutils - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "../../include" /I "../../include/sidplay/utils" /I "../../../libsidplay/include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "DLL_EXPORT" /D "HAVE_MSWINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"../../../bin_vc5/Debug/libsidutils.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "libsidutils - Win32 Release"
# Name "libsidutils - Win32 Debug"
# Begin Source File

SOURCE=..\..\include\config.h
# End Source File
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ini\headings.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ini\headings.i
# End Source File
# Begin Source File

SOURCE=..\..\src\ini\ini.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\ini\ini.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ini\keys.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ini\keys.i
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\utils\libini.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ini\list.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ini\list.i
# End Source File
# Begin Source File

SOURCE=..\..\src\Md5\MD5.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\Md5\MD5.h
# End Source File
# Begin Source File

SOURCE=..\..\src\Md5\MD5_Defs.h
# End Source File
# Begin Source File

SOURCE=..\..\src\SidDatabase.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\utils\SidDatabase.h
# End Source File
# Begin Source File

SOURCE=..\..\src\SidFilter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\utils\SidFilter.h
# End Source File
# Begin Source File

SOURCE=..\..\src\SidTuneMod.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\utils\SidTuneMod.h
# End Source File
# Begin Source File

SOURCE=..\..\src\SidUsage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\utils\SidUsage.h
# End Source File
# Begin Source File

SOURCE=..\..\src\smm.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\smm0.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ini\types.i
# End Source File
# End Target
# End Project
