# Microsoft Developer Studio Project File - Name="projectM" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=projectM - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "projectM.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "projectM.mak" CFG="projectM - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "projectM - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "projectM - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "projectM - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "projectM - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "DEBUG" /FD /GZ /c
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

# Name "projectM - Win32 Release"
# Name "projectM - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\beat_detect.c
# End Source File
# Begin Source File

SOURCE=.\browser.c
# End Source File
# Begin Source File

SOURCE=.\builtin_funcs.c
# End Source File
# Begin Source File

SOURCE=.\console_interface.c
# End Source File
# Begin Source File

SOURCE=.\custom_shape.c
# End Source File
# Begin Source File

SOURCE=.\custom_wave.c
# End Source File
# Begin Source File

SOURCE=.\editor.c
# End Source File
# Begin Source File

SOURCE=.\eval.c
# End Source File
# Begin Source File

SOURCE=.\fftsg.c
# End Source File
# Begin Source File

SOURCE=.\func.c
# End Source File
# Begin Source File

SOURCE=.\glConsole.c
# End Source File
# Begin Source File

SOURCE=.\glf.c
# End Source File
# Begin Source File

SOURCE=.\init_cond.c
# End Source File
# Begin Source File

SOURCE=.\menu.c
# End Source File
# Begin Source File

SOURCE=.\param.c
# End Source File
# Begin Source File

SOURCE=.\parser.c
# End Source File
# Begin Source File

SOURCE=.\PCM.c
# End Source File
# Begin Source File

SOURCE=.\per_frame_eqn.c
# End Source File
# Begin Source File

SOURCE=.\per_pixel_eqn.c
# End Source File
# Begin Source File

SOURCE=.\preset.c
# End Source File
# Begin Source File

SOURCE=.\projectm.c
# End Source File
# Begin Source File

SOURCE=.\pbuffer.c
# End Source File
# Begin Source File

SOURCE=.\splaytree.c
# End Source File
# Begin Source File

SOURCE=.\timer.c
# End Source File
# Begin Source File

SOURCE=.\tree_types.c
# End Source File
# Begin Source File

SOURCE=".\win32-dirent.c"
# End Source File
# Begin Source File

SOURCE=.\wipemalloc.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\beat_detect.h
# End Source File
# Begin Source File

SOURCE=.\browser.h
# End Source File
# Begin Source File

SOURCE=.\builtin_funcs.h
# End Source File
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=.\compare.h
# End Source File
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=.\console_interface.h
# End Source File
# Begin Source File

SOURCE=.\custom_shape.h
# End Source File
# Begin Source File

SOURCE=.\custom_shape_types.h
# End Source File
# Begin Source File

SOURCE=.\custom_wave.h
# End Source File
# Begin Source File

SOURCE=.\custom_wave_types.h
# End Source File
# Begin Source File

SOURCE=.\editor.h
# End Source File
# Begin Source File

SOURCE=.\eval.h
# End Source File
# Begin Source File

SOURCE=.\event.h
# End Source File
# Begin Source File

SOURCE=.\expr_types.h
# End Source File
# Begin Source File

SOURCE=.\fatal.h
# End Source File
# Begin Source File

SOURCE=.\fftsg.h
# End Source File
# Begin Source File

SOURCE=.\func.h
# End Source File
# Begin Source File

SOURCE=.\func_types.h
# End Source File
# Begin Source File

SOURCE=.\glConsole.h
# End Source File
# Begin Source File

SOURCE=.\glf.h
# End Source File
# Begin Source File

SOURCE=.\idle_preset.h
# End Source File
# Begin Source File

SOURCE=.\init_cond.h
# End Source File
# Begin Source File

SOURCE=.\init_cond_types.h
# End Source File
# Begin Source File

SOURCE=.\interface_types.h
# End Source File
# Begin Source File

SOURCE=.\menu.h
# End Source File
# Begin Source File

SOURCE=.\param.h
# End Source File
# Begin Source File

SOURCE=.\param_types.h
# End Source File
# Begin Source File

SOURCE=.\parser.h
# End Source File
# Begin Source File

SOURCE=.\PCM.h
# End Source File
# Begin Source File

SOURCE=.\per_frame_eqn.h
# End Source File
# Begin Source File

SOURCE=.\per_frame_eqn_types.h
# End Source File
# Begin Source File

SOURCE=.\per_pixel_eqn.h
# End Source File
# Begin Source File

SOURCE=.\per_pixel_eqn_types.h
# End Source File
# Begin Source File

SOURCE=.\per_point_types.h
# End Source File
# Begin Source File

SOURCE=.\preset.h
# End Source File
# Begin Source File

SOURCE=.\preset_types.h
# End Source File
# Begin Source File

SOURCE=.\projectM.h
# End Source File
# Begin Source File

SOURCE=.\pbuffer.h
# End Source File
# Begin Source File

SOURCE=.\splaytree.h
# End Source File
# Begin Source File

SOURCE=.\splaytree_types.h
# End Source File
# Begin Source File

SOURCE=.\timer.h
# End Source File
# Begin Source File

SOURCE=.\tree_types.h
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
