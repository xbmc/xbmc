# Microsoft Developer Studio Project File - Name="libprojectM" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libprojectM - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libprojectM.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libprojectM.mak" CFG="libprojectM - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libprojectM - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libprojectM - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libprojectM - Win32 Release"

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
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libprojectM - Win32 Debug"

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
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "DEBUG" /YX /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libprojectM - Win32 Release"
# Name "libprojectM - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\BeatDetect.cpp
# End Source File
# Begin Source File

SOURCE=.\browser.cpp
# End Source File
# Begin Source File

SOURCE=.\builtin_funcs.cpp
# End Source File
# Begin Source File

SOURCE=.\console_interface.cpp
# End Source File
# Begin Source File

SOURCE=.\CustomShape.cpp
# End Source File
# Begin Source File

SOURCE=.\CustomWave.cpp
# End Source File
# Begin Source File

SOURCE=.\editor.cpp
# End Source File
# Begin Source File

SOURCE=.\Eval.cpp
# End Source File
# Begin Source File

SOURCE=.\Expr.cpp
# End Source File
# Begin Source File

SOURCE=.\fftsg.cpp
# End Source File
# Begin Source File

SOURCE=.\Func.cpp
# End Source File
# Begin Source File

SOURCE=.\glConsole.cpp
# End Source File
# Begin Source File

SOURCE=.\InitCond.cpp
# End Source File
# Begin Source File

SOURCE=.\menu.cpp
# End Source File
# Begin Source File

SOURCE=.\Param.cpp
# End Source File
# Begin Source File

SOURCE=.\Parser.cpp
# End Source File
# Begin Source File

SOURCE=.\pbuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\PCM.cpp
# End Source File
# Begin Source File

SOURCE=.\PerFrameEqn.cpp
# End Source File
# Begin Source File

SOURCE=.\PerPixelEqn.cpp
# End Source File
# Begin Source File

SOURCE=.\PerPointEqn.cpp
# End Source File
# Begin Source File

SOURCE=.\Preset.cpp
# End Source File
# Begin Source File

SOURCE=.\projectM.cpp
# End Source File
# Begin Source File

SOURCE=.\SplayTree.cpp
# End Source File
# Begin Source File

SOURCE=.\timer.cpp
# End Source File
# Begin Source File

SOURCE=".\win32-dirent.cpp"
# End Source File
# Begin Source File

SOURCE=.\wipemalloc.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\BeatDetect.h
# End Source File
# Begin Source File

SOURCE=.\browser.h
# End Source File
# Begin Source File

SOURCE=.\builtin_funcs.h
# End Source File
# Begin Source File

SOURCE=.\carbontoprojectM.h
# End Source File
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=.\compare.h
# End Source File
# Begin Source File

SOURCE=.\console_interface.h
# End Source File
# Begin Source File

SOURCE=.\CustomShape.h
# End Source File
# Begin Source File

SOURCE=.\CustomWave.h
# End Source File
# Begin Source File

SOURCE=.\CValue.h
# End Source File
# Begin Source File

SOURCE=.\dlldefs.h
# End Source File
# Begin Source File

SOURCE=.\editor.h
# End Source File
# Begin Source File

SOURCE=.\Eval.h
# End Source File
# Begin Source File

SOURCE=.\event.h
# End Source File
# Begin Source File

SOURCE=.\Expr.h
# End Source File
# Begin Source File

SOURCE=.\fatal.h
# End Source File
# Begin Source File

SOURCE=.\fftsg.h
# End Source File
# Begin Source File

SOURCE=.\Func.h
# End Source File
# Begin Source File

SOURCE=.\glConsole.h
# End Source File
# Begin Source File

SOURCE=.\glf.h
# End Source File
# Begin Source File

SOURCE=.\InitCond.h
# End Source File
# Begin Source File

SOURCE=.\lvtoprojectM.h
# End Source File
# Begin Source File

SOURCE=.\menu.h
# End Source File
# Begin Source File

SOURCE=.\Param.h
# End Source File
# Begin Source File

SOURCE=.\Parser.h
# End Source File
# Begin Source File

SOURCE=.\pbuffer.h
# End Source File
# Begin Source File

SOURCE=.\PCM.h
# End Source File
# Begin Source File

SOURCE=.\PerFrameEqn.h
# End Source File
# Begin Source File

SOURCE=.\PerPixelEqn.h
# End Source File
# Begin Source File

SOURCE=.\PerPointEqn.h
# End Source File
# Begin Source File

SOURCE=.\Preset.h
# End Source File
# Begin Source File

SOURCE=.\projectM.h
# End Source File
# Begin Source File

SOURCE=.\sdltoprojectM.h
# End Source File
# Begin Source File

SOURCE=.\SplayTree.h
# End Source File
# Begin Source File

SOURCE=.\timer.h
# End Source File
# Begin Source File

SOURCE=".\win32-dirent.h"
# End Source File
# Begin Source File

SOURCE=.\wipemalloc.h
# End Source File
# End Group
# End Target
# End Project
