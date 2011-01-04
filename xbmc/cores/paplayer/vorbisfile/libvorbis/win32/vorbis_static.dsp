# Microsoft Developer Studio Project File - Name="vorbis_static" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=vorbis_static - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vorbis_static.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vorbis_static.mak" CFG="vorbis_static - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vorbis_static - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "vorbis_static - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vorbis_static - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Vorbis_Static_Release"
# PROP Intermediate_Dir "Vorbis_Static_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\include" /I "..\..\ogg\include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FR /YX /FD /O3 /QaxK /Qsox- /Qip /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "vorbis_static - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Vorbis_Static_Debug"
# PROP Intermediate_Dir "Vorbis_Static_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\include" /I "..\..\ogg\include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Vorbis_Static_Debug\vorbis_static_d.lib"

!ENDIF 

# Begin Target

# Name "vorbis_static - Win32 Release"
# Name "vorbis_static - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\lib\analysis.c
# End Source File
# Begin Source File

SOURCE=..\lib\bitrate.c
# End Source File
# Begin Source File

SOURCE=..\lib\block.c
# End Source File
# Begin Source File

SOURCE=..\lib\codebook.c
# End Source File
# Begin Source File

SOURCE=..\lib\envelope.c
# End Source File
# Begin Source File

SOURCE=..\lib\floor0.c
# End Source File
# Begin Source File

SOURCE=..\lib\floor1.c
# End Source File
# Begin Source File

SOURCE=..\lib\info.c
# End Source File
# Begin Source File

SOURCE=..\lib\lookup.c
# End Source File
# Begin Source File

SOURCE=..\lib\lpc.c
# End Source File
# Begin Source File

SOURCE=..\lib\lsp.c
# End Source File
# Begin Source File

SOURCE=..\lib\mapping0.c
# End Source File
# Begin Source File

SOURCE=..\lib\mdct.c
# End Source File
# Begin Source File

SOURCE=..\lib\psy.c
# End Source File
# Begin Source File

SOURCE=..\lib\registry.c
# End Source File
# Begin Source File

SOURCE=..\lib\res0.c
# End Source File
# Begin Source File

SOURCE=..\lib\sharedbook.c
# End Source File
# Begin Source File

SOURCE=..\lib\smallft.c
# End Source File
# Begin Source File

SOURCE=..\lib\synthesis.c
# End Source File
# Begin Source File

SOURCE=..\lib\window.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\include\vorbis\backends.h
# End Source File
# Begin Source File

SOURCE=..\lib\bitbuffer.h
# End Source File
# Begin Source File

SOURCE=..\lib\bitrate.h
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\codebook.h
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\codec.h
# End Source File
# Begin Source File

SOURCE=..\lib\codec_internal.h
# End Source File
# Begin Source File

SOURCE=..\lib\envelope.h
# End Source File
# Begin Source File

SOURCE=..\lib\highlevel.h
# End Source File
# Begin Source File

SOURCE=..\lib\lookup.h
# End Source File
# Begin Source File

SOURCE=..\lib\lookup_data.h
# End Source File
# Begin Source File

SOURCE=..\lib\lpc.h
# End Source File
# Begin Source File

SOURCE=..\lib\lsp.h
# End Source File
# Begin Source File

SOURCE=..\lib\masking.h
# End Source File
# Begin Source File

SOURCE=..\lib\mdct.h
# End Source File
# Begin Source File

SOURCE=..\lib\misc.h
# End Source File
# Begin Source File

SOURCE=..\lib\os.h
# End Source File
# Begin Source File

SOURCE=..\lib\psy.h
# End Source File
# Begin Source File

SOURCE=..\lib\registry.h
# End Source File
# Begin Source File

SOURCE=..\lib\scales.h
# End Source File
# Begin Source File

SOURCE=..\lib\smallft.h
# End Source File
# Begin Source File

SOURCE=..\lib\window.h
# End Source File
# End Group
# End Target
# End Project
