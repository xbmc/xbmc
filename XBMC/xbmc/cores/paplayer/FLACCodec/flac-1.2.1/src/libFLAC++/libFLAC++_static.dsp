# Microsoft Developer Studio Project File - Name="libFLAC++_static" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libFLAC++_static - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libFLAC++_static.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libFLAC++_static.mak" CFG="libFLAC++_static - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libFLAC++_static - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libFLAC++_static - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "libFLAC++"
# PROP Scc_LocalPath "..\.."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libFLAC++_static - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\obj\release\lib"
# PROP Intermediate_Dir "Release_static"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /I ".\include" /I "..\..\include" /D "FLAC__NO_DLL" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libFLAC++_static - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\obj\debug\lib"
# PROP Intermediate_Dir "Debug_static"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I ".\include" /I "..\..\include" /D "FLAC__NO_DLL" /D "WIN32" /D "_DEBUG" /D "DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
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

# Name "libFLAC++_static - Win32 Release"
# Name "libFLAC++_static - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp"
# Begin Source File

SOURCE=.\metadata.cpp
# End Source File
# Begin Source File

SOURCE=.\stream_decoder.cpp
# End Source File
# Begin Source File

SOURCE=.\stream_encoder.cpp
# End Source File
# End Group
# Begin Group "Private Header Files"

# PROP Default_Filter ""
# End Group
# Begin Group "Protected Header Files"

# PROP Default_Filter ""
# End Group
# Begin Group "Public Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\..\include\FLAC++\all.h"
# End Source File
# Begin Source File

SOURCE="..\..\include\FLAC++\decoder.h"
# End Source File
# Begin Source File

SOURCE="..\..\include\FLAC++\encoder.h"
# End Source File
# Begin Source File

SOURCE="..\..\include\FLAC++\export.h"
# End Source File
# Begin Source File

SOURCE="..\..\include\FLAC++\metadata.h"
# End Source File
# End Group
# End Target
# End Project
