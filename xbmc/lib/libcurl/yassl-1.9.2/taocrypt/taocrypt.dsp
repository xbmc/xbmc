# Microsoft Developer Studio Project File - Name="taocrypt" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=taocrypt - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "taocrypt.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "taocrypt.mak" CFG="taocrypt - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "taocrypt - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "taocrypt - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "taocrypt - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "taocrypt___Win32_Release"
# PROP BASE Intermediate_Dir "taocrypt___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /G6 /MD /W3 /O2 /I "include" /I "mySTL" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "taocrypt - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "taocrypt___Win32_Debug"
# PROP BASE Intermediate_Dir "taocrypt___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /ZI /Od /I "include" /I "mySTL" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /Fr /YX /FD /GZ /c
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

# Name "taocrypt - Win32 Release"
# Name "taocrypt - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\src\aes.cpp
# End Source File
# Begin Source File

SOURCE=.\src\aestables.cpp
# End Source File
# Begin Source File

SOURCE=.\src\algebra.cpp
# End Source File
# Begin Source File

SOURCE=.\src\arc4.cpp
# End Source File
# Begin Source File

SOURCE=.\src\asn.cpp
# End Source File
# Begin Source File

SOURCE=.\src\bftables.cpp
# End Source File
# Begin Source File

SOURCE=.\src\blowfish.cpp
# End Source File
# Begin Source File

SOURCE=.\src\coding.cpp
# End Source File
# Begin Source File

SOURCE=.\src\des.cpp
# End Source File
# Begin Source File

SOURCE=.\src\dh.cpp
# End Source File
# Begin Source File

SOURCE=.\src\dsa.cpp
# End Source File
# Begin Source File

SOURCE=.\src\file.cpp
# End Source File
# Begin Source File

SOURCE=.\src\hash.cpp
# End Source File
# Begin Source File

SOURCE=.\src\integer.cpp
# End Source File
# Begin Source File

SOURCE=.\src\md2.cpp
# End Source File
# Begin Source File

SOURCE=.\src\md4.cpp
# End Source File
# Begin Source File

SOURCE=.\src\md5.cpp
# End Source File
# Begin Source File

SOURCE=.\src\misc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\random.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ripemd.cpp
# End Source File
# Begin Source File

SOURCE=.\src\rsa.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sha.cpp
# End Source File
# Begin Source File

SOURCE=.\src\tftables.cpp
# End Source File
# Begin Source File

SOURCE=.\src\twofish.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\include\aes.hpp
# End Source File
# Begin Source File

SOURCE=.\include\algebra.hpp
# End Source File
# Begin Source File

SOURCE=.\include\arc4.hpp
# End Source File
# Begin Source File

SOURCE=.\include\asn.hpp
# End Source File
# Begin Source File

SOURCE=.\include\block.hpp
# End Source File
# Begin Source File

SOURCE=.\include\blowfish.hpp
# End Source File
# Begin Source File

SOURCE=.\include\coding.hpp
# End Source File
# Begin Source File

SOURCE=.\include\des.hpp
# End Source File
# Begin Source File

SOURCE=.\include\dh.hpp
# End Source File
# Begin Source File

SOURCE=.\include\dsa.hpp
# End Source File
# Begin Source File

SOURCE=.\include\error.hpp
# End Source File
# Begin Source File

SOURCE=.\include\file.hpp
# End Source File
# Begin Source File

SOURCE=.\include\hash.hpp
# End Source File
# Begin Source File

SOURCE=.\include\hmac.hpp
# End Source File
# Begin Source File

SOURCE=.\include\integer.hpp
# End Source File
# Begin Source File

SOURCE=.\include\md2.hpp
# End Source File
# Begin Source File

SOURCE=.\include\md4.hpp
# End Source File
# Begin Source File

SOURCE=.\include\md5.hpp
# End Source File
# Begin Source File

SOURCE=.\include\misc.hpp
# End Source File
# Begin Source File

SOURCE=.\include\modarith.hpp
# End Source File
# Begin Source File

SOURCE=.\include\modes.hpp
# End Source File
# Begin Source File

SOURCE=.\include\pwdbased.hpp
# End Source File
# Begin Source File

SOURCE=.\include\random.hpp
# End Source File
# Begin Source File

SOURCE=.\include\ripemd.hpp
# End Source File
# Begin Source File

SOURCE=.\include\rsa.hpp
# End Source File
# Begin Source File

SOURCE=.\include\sha.hpp
# End Source File
# Begin Source File

SOURCE=.\include\twofish.hpp
# End Source File
# Begin Source File

SOURCE=.\include\type_traits.hpp
# End Source File
# Begin Source File

SOURCE=.\include\types.hpp
# End Source File
# End Group
# End Target
# End Project
