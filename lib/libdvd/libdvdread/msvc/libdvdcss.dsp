# Microsoft Developer Studio Project File - Name="libdvdcss" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=libdvdcss - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libdvdcss.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libdvdcss.mak" CFG="libdvdcss - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libdvdcss - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libdvdcss - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libdvdcss - Win32 Release"

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
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_USRDLL" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "." /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_USRDLL" /D PATH_MAX=2048 /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /machine:IX86
# ADD LINK32 /machine:IX86
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Create libdvdcss Install
PostBuild_Cmds=scripts\libdvdcss_intstall.bat Release
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libdvdcss - Win32 Debug"

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
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_USRDLL" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "." /I "../libdvdcss/dvdcss" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_USRDLL" /D MAX_PATH=2048 /YX /FD /GZ ./ "../libdvdcss" /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /machine:IX86
# ADD LINK32 /dll /machine:IX86 /out:"Debug/bin/libdvdcss.dll"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Create libdvdcss Install
PostBuild_Cmds=scripts\libdvdcss_intstall.bat Debug
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "libdvdcss - Win32 Release"
# Name "libdvdcss - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\libdvdcss\css.c
# End Source File
# Begin Source File

SOURCE=..\..\libdvdcss\device.c
# End Source File
# Begin Source File

SOURCE=..\..\libdvdcss\error.c
# End Source File
# Begin Source File

SOURCE=..\..\libdvdcss\ioctl.c
# End Source File
# Begin Source File

SOURCE=..\..\libdvdcss\libdvdcss.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "DLL Defs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\libdvdcss.def
# End Source File
# End Group
# End Target
# End Project
