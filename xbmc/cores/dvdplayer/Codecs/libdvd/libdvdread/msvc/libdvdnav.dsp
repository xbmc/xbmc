# Microsoft Developer Studio Project File - Name="libdvdnav" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libdvdnav - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libdvdnav.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libdvdnav.mak" CFG="libdvdnav - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libdvdnav - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libdvdnav - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libdvdnav - Win32 Release"

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
LINK32=link.exe
# ADD BASE LINK32 /machine:IX86
# ADD LINK32 /machine:IX86
MTL=midl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\..\libdvdcss\src" /I "." /I "include" /I "contrib/dirent" /I "include/pthreads" /I "../../libdvdcss" /I ".." /I "../src" /I "../src/dvdread" /I "../src/vm" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "DVDNAV_COMPILE" /D "HAVE_CONFIG_H" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Release\libdvdnav\libdvdnav.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Create libdvdnav Install Files
PostBuild_Cmds=scripts\libdvdnav_install.bat Release
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libdvdnav - Win32 Debug"

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
LINK32=link.exe
# ADD BASE LINK32 /machine:IX86
# ADD LINK32 /debug /machine:IX86 /out:"Debug/libdvdnav.lib" /implib:"Debug/libdvdnav.lib"
# SUBTRACT LINK32 /pdb:none /nodefaultlib
MTL=midl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "include" /I "contrib/dirent" /I "include/pthreads" /I "../../libdvdcss" /I "../src" /I "." /I ".." /I "../src/dvdread" /I "../src/vm" /D "WIN32" /D "_DEBUG" /D "_LIB" /D "DVDNAV_COMPILE" /D "HAVE_CONFIG_H" /FR"Debug/libdvdnav/" /Fp"Debug/libdvdnav/libdvdnav.pch" /YX /Fo"Debug/libdvdnav/" /Fd"Debug/libdvdnav/" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 libwin32utils.lib /nologo /out:"Debug\libdvdnav\libdvdnav.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Create libdvdnav Install Files
PostBuild_Cmds=scripts\libdvdnav_install.bat Debug
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "libdvdnav - Win32 Release"
# Name "libdvdnav - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\vm\decoder.c
# End Source File
# Begin Source File

SOURCE=..\src\dvdread\dvd_input.c
# End Source File
# Begin Source File

SOURCE=..\src\dvdread\dvd_reader.c
# End Source File
# Begin Source File

SOURCE=..\src\dvdread\dvd_udf.c
# End Source File
# Begin Source File

SOURCE=..\src\dvdnav.c
# End Source File
# Begin Source File

SOURCE=..\src\highlight.c
# End Source File
# Begin Source File

SOURCE=..\src\dvdread\ifo_print.c
# End Source File
# Begin Source File

SOURCE=..\src\dvdread\ifo_read.c
# End Source File
# Begin Source File

SOURCE=..\src\dvdread\md5.c
# End Source File
# Begin Source File

SOURCE=..\src\dvdread\nav_print.c
# End Source File
# Begin Source File

SOURCE=..\src\dvdread\nav_read.c
# End Source File
# Begin Source File

SOURCE=..\src\navigation.c
# End Source File
# Begin Source File

SOURCE=..\src\read_cache.c
# End Source File
# Begin Source File

SOURCE=..\src\remap.c
# End Source File
# Begin Source File

SOURCE=..\src\searching.c
# End Source File
# Begin Source File

SOURCE=..\src\settings.c
# End Source File
# Begin Source File

SOURCE=..\src\vm\vm.c
# End Source File
# Begin Source File

SOURCE=..\src\vm\vmcmd.c
# End Source File
# End Group
# Begin Group "DLL Defs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\libdvdnav.def
# End Source File
# End Group
# End Target
# End Project
