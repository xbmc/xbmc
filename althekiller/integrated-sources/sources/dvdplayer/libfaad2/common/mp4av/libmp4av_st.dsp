# Microsoft Developer Studio Project File - Name="libmp4av_st" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libmp4av_st - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libmp4av_st.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libmp4av_st.mak" CFG="libmp4av_st - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libmp4av_st - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libmp4av_st - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=xicl6.exe
RSC=rc.exe

!IF  "$(CFG)" == "libmp4av_st - Win32 Release"

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
MTL=midl.exe
F90=df.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "..\mp4v2" /I "." /I "..\..\include" /I "..\..\lib\mp4v2" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libmp4av_st - Win32 Debug"

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
MTL=midl.exe
F90=df.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\mp4v2" /I "." /I "..\..\include" /I "..\..\lib\mp4v2" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
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

# Name "libmp4av_st - Win32 Release"
# Name "libmp4av_st - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\aac.cpp
# End Source File
# Begin Source File

SOURCE=.\adts.cpp
# End Source File
# Begin Source File

SOURCE=.\audio_hinters.cpp
# End Source File
# Begin Source File

SOURCE=.\l16.cpp
# End Source File
# Begin Source File

SOURCE=.\mbs.cpp
# End Source File
# Begin Source File

SOURCE=.\mp3.cpp
# End Source File
# Begin Source File

SOURCE=.\mpeg3.cpp
# End Source File
# Begin Source File

SOURCE=.\mpeg4.cpp
# End Source File
# Begin Source File

SOURCE=.\rfc2250.cpp
# End Source File
# Begin Source File

SOURCE=.\rfc3016.cpp
# End Source File
# Begin Source File

SOURCE=.\rfc3119.cpp
# End Source File
# Begin Source File

SOURCE=.\rfcisma.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\audio_hinters.h
# End Source File
# Begin Source File

SOURCE=.\mbs.h
# End Source File
# Begin Source File

SOURCE=.\mp4av.h
# End Source File
# Begin Source File

SOURCE=.\mp4av_aac.h
# End Source File
# Begin Source File

SOURCE=.\mp4av_adts.h
# End Source File
# Begin Source File

SOURCE=.\mp4av_common.h
# End Source File
# Begin Source File

SOURCE=.\mp4av_hinters.h
# End Source File
# Begin Source File

SOURCE=.\mp4av_mp3.h
# End Source File
# Begin Source File

SOURCE=.\mp4av_mpeg4.h
# End Source File
# End Group
# End Target
# End Project
