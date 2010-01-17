# Microsoft Developer Studio Project File - Name="yassl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=yassl - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "yassl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "yassl.mak" CFG="yassl - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "yassl - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "yassl - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "yassl - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /O2 /I "include" /I "taocrypt\include" /I "taocrypt\mySTL" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "yassl - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /ZI /Od /I "include" /I "taocrypt\include" /I "taocrypt\mySTL" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FR /YX /FD /GZ /c
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

# Name "yassl - Win32 Release"
# Name "yassl - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\src\buffer.cpp
# End Source File
# Begin Source File

SOURCE=.\src\cert_wrapper.cpp
# End Source File
# Begin Source File

SOURCE=.\src\crypto_wrapper.cpp
# End Source File
# Begin Source File

SOURCE=.\src\handshake.cpp
# End Source File
# Begin Source File

SOURCE=.\src\lock.cpp
# End Source File
# Begin Source File

SOURCE=.\src\log.cpp
# End Source File
# Begin Source File

SOURCE=.\src\socket_wrapper.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ssl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\timer.cpp
# End Source File
# Begin Source File

SOURCE=.\src\yassl_error.cpp
# End Source File
# Begin Source File

SOURCE=.\src\yassl_imp.cpp
# End Source File
# Begin Source File

SOURCE=.\src\yassl_int.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\include\buffer.hpp
# End Source File
# Begin Source File

SOURCE=.\include\cert_wrapper.hpp
# End Source File
# Begin Source File

SOURCE=.\include\crypto_wrapper.hpp
# End Source File
# Begin Source File

SOURCE=.\include\factory.hpp
# End Source File
# Begin Source File

SOURCE=.\include\handshake.hpp
# End Source File
# Begin Source File

SOURCE=.\include\lock.hpp
# End Source File
# Begin Source File

SOURCE=.\include\log.hpp
# End Source File
# Begin Source File

SOURCE=.\include\socket_wrapper.hpp
# End Source File
# Begin Source File

SOURCE=.\include\timer.hpp
# End Source File
# Begin Source File

SOURCE=.\include\yassl_error.hpp
# End Source File
# Begin Source File

SOURCE=.\include\yassl_imp.hpp
# End Source File
# Begin Source File

SOURCE=.\include\yassl_int.hpp
# End Source File
# Begin Source File

SOURCE=.\include\yassl_types.hpp
# End Source File
# End Group
# End Target
# End Project
