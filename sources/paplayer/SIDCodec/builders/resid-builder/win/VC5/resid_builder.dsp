# Microsoft Developer Studio Project File - Name="resid_builder" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=resid_builder - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "resid_builder.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "resid_builder.mak" CFG="resid_builder - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "resid_builder - Win32 Release" (based on\
 "Win32 (x86) Static Library")
!MESSAGE "resid_builder - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe

!IF  "$(CFG)" == "resid_builder - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "resid_bu"
# PROP BASE Intermediate_Dir "resid_bu"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../include" /I "../../include/sidplay/builders" /I "../../../../libsidplay/include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "HAVE_MSWINDOWS" /YX /FD /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "resid_builder - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "resid_b0"
# PROP BASE Intermediate_Dir "resid_b0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /GX /Zi /Od /I "../../include" /I "../../include/sidplay/builders" /I "../../../../libsidplay/include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "HAVE_MSWINDOWS" /YX /FD /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "resid_builder - Win32 Release"
# Name "resid_builder - Win32 Debug"
# Begin Group "ReSID 0.15"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\resid\envelope.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\resid\envelope.h
# End Source File
# Begin Source File

SOURCE=..\..\src\resid\extfilt.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\resid\extfilt.h
# End Source File
# Begin Source File

SOURCE=..\..\src\resid\filter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\resid\filter.h
# End Source File
# Begin Source File

SOURCE=..\..\src\resid\pot.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\resid\pot.h
# End Source File
# Begin Source File

SOURCE=..\..\src\resid\sid.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\resid\sid.h
# End Source File
# Begin Source File

SOURCE=..\..\src\resid\siddefs.h
# End Source File
# Begin Source File

SOURCE=..\..\src\resid\spline.h
# End Source File
# Begin Source File

SOURCE=..\..\src\resid\version.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\resid\voice.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\resid\voice.h
# End Source File
# Begin Source File

SOURCE=..\..\src\resid\wave.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\resid\wave.h
# End Source File
# Begin Source File

SOURCE=..\..\src\resid\wave6581__ST.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\resid\wave6581_P_T.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\resid\wave6581_PS_.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\resid\wave6581_PST.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\resid\wave8580__ST.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\resid\wave8580_P_T.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\resid\wave8580_PS_.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\resid\wave8580_PST.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\include\config.h
# End Source File
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE="..\..\src\resid-builder.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\src\resid-emu.h"
# End Source File
# Begin Source File

SOURCE="..\..\src\resid-win-cfg.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\src\resid-win-cfg.h"
# End Source File
# Begin Source File

SOURCE=..\..\src\resid.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\builders\resid.h
# End Source File
# End Target
# End Project
