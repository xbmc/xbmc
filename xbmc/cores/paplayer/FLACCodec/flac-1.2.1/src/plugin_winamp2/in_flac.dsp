# Microsoft Developer Studio Project File - Name="in_flac" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=in_flac - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "in_flac.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "in_flac.mak" CFG="in_flac - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "in_flac - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "in_flac - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "$/Studio/in_flac"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "in_flac - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "in_flac_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /WX /GX /Ox /Og /Oi /Os /Op /Gf /Gy /I "include" /I ".." /I "..\..\include" /D "NDEBUG" /D VERSION=\"1.2.1\" /D "in_flac_EXPORTS" /D "FLAC__NO_DLL" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TAGZ_UNICODE" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 plugin_common_static.lib grabbag_static.lib libFLAC_static.lib replaygain_analysis_static.lib replaygain_synthesis_static.lib kernel32.lib user32.lib /nologo /dll /machine:I386 /out:"../../obj/release/bin/in_flac.dll" /libpath:"../../obj/release/lib" ..\..\obj\release\lib\ogg_static.lib /opt:nowin98
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "in_flac - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "in_flac_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "include" /I ".." /I "..\..\include" /D "_DEBUG" /D "DEBUG" /D "REAL_STDIO" /D VERSION=\"1.2.1\" /D "in_flac_EXPORTS" /D "FLAC__NO_DLL" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TAGZ_UNICODE" /YX /FD /GZ /c
# SUBTRACT CPP /WX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 plugin_common_static.lib grabbag_static.lib libFLAC_static.lib replaygain_analysis_static.lib replaygain_synthesis_static.lib kernel32.lib user32.lib /nologo /dll /debug /machine:I386 /out:"../../obj/debug/bin/in_flac.dll" /pdbtype:sept /libpath:"../../obj/debug/lib" ..\..\obj\release\lib\ogg_static.lib
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "in_flac - Win32 Release"
# Name "in_flac - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Winamp2 SDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\include\winamp2\in2.h
# End Source File
# Begin Source File

SOURCE=.\include\winamp2\out.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\configure.c
# End Source File
# Begin Source File

SOURCE=.\configure.h
# End Source File
# Begin Source File

SOURCE=.\in_flac.c
# End Source File
# Begin Source File

SOURCE=.\infobox.c
# End Source File
# Begin Source File

SOURCE=.\infobox.h
# End Source File
# Begin Source File

SOURCE=.\playback.c
# End Source File
# Begin Source File

SOURCE=.\playback.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\resource.rc
# End Source File
# Begin Source File

SOURCE=.\tagz.cpp
# End Source File
# Begin Source File

SOURCE=.\tagz.h
# End Source File
# End Group
# End Target
# End Project
