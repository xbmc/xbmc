# Microsoft Developer Studio Project File - Name="simpleModule" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=simpleModule - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "simpleModule.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "simpleModule.mak" CFG="simpleModule.mak - Win32"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "simpleModule - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library"
!MESSAGE "simpleModule - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library"
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "simpleModule - Win32 Release" 

# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""

# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409

# ADD BASE CPP 
# ADD CPP  -W3  -nologo -MDd -FD -DWIN -D_DLL -D_MT -D_WINDOWS -DWIN32 -D_WIN32_WINNT=0x500 -D_X86_=1 -D_CRT_SECURE_NO_DEPRECATE -D_USRDLL -I../../../include -I../../..  -O1 -D_NDEBUG /c
LINK32=link.exe
# ADD BASE LINK32 
# ADD LINK32 -out:.\Release\simpleModule.dll -dll  -entry:_DllMainCRTStartup@12 -def:./Release/simpleModule.def   -machine:x86 -nodefaultlib -incremental:no -nologo    -libpath:"../../../bin" -libpath:"../../../bin/Release"  libappweb.lib ws2_32.lib advapi32.lib user32.lib kernel32.lib oldnames.lib msvcrt.lib

# Begin Special Build Tool
PreLink_Desc=Export symbols for .\Release\simpleModule.dll
PreLink_Cmds=..\..\..\bin\dumpext -o ./Release/simpleModule.def simpleModule.dll   ./Release/simpleModule.obj
# End Special Build Tool

!ENDIF

!IF  "$(CFG)" == "simpleModule - Win32 Debug" 

# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""

# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409

# ADD BASE CPP 
# ADD CPP  -W3  -nologo -MDd -FD -DWIN -D_DLL -D_MT -D_WINDOWS -DWIN32 -D_WIN32_WINNT=0x500 -D_X86_=1 -D_CRT_SECURE_NO_DEPRECATE -D_USRDLL -I../../../include -I../../..  -Zi -Od -D_DEBUG /c
LINK32=link.exe
# ADD BASE LINK32 
# ADD LINK32 -out:.\Debug\simpleModule.dll -dll  -entry:_DllMainCRTStartup@12 -def:./Debug/simpleModule.def   -machine:x86 -nodefaultlib -incremental:no -nologo -debug   -libpath:"../../../bin" -libpath:"../../../bin/Debug"  libappweb.lib ws2_32.lib advapi32.lib user32.lib kernel32.lib oldnames.lib msvcrt.lib

# Begin Special Build Tool
PreLink_Desc=Export symbols for .\Debug\simpleModule.dll
PreLink_Cmds=..\..\..\bin\dumpext -o ./Debug/simpleModule.def simpleModule.dll   ./Debug/simpleModule.obj
# End Special Build Tool

!ENDIF


# Begin Target

# Name "simpleModule - Win32 Release"
# Name "simpleModule - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"

# Begin Source File
SOURCE=simpleModule.cpp
# End Source File

# End Group

# Begin Group "Header Files"
# PROP Default_Filter "h;"
# Begin Source File
SOURCE=.\simpleModule.h
# End Source File

# End Group
# End Target
# End Project
