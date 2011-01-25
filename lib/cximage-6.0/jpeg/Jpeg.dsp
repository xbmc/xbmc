# Microsoft Developer Studio Project File - Name="jpeg" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=jpeg - Win32 Unicode Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Jpeg.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Jpeg.mak" CFG="jpeg - Win32 Unicode Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "jpeg - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "jpeg - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "jpeg - Win32 Unicode Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "jpeg - Win32 Unicode Release" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "jpeg - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_CRT_SECURE_NO_DEPRECATE" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x410
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "jpeg - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_CRT_SECURE_NO_DEPRECATE" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x410
# ADD RSC /l 0x809
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "jpeg - Win32 Unicode Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "jpeg___Win32_Unicode_Debug"
# PROP BASE Intermediate_Dir "jpeg___Win32_Unicode_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Unicode_Debug"
# PROP Intermediate_Dir "Unicode_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MDd /W3 /GX /ZI /Od /D "_WINDOWS" /D "WIN32" /D "_DEBUG" /D "_UNICODE" /D "UNICODE" /D "_CRT_SECURE_NO_DEPRECATE" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x410 /d "_AFXDLL"
# ADD RSC /l 0x809
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "jpeg - Win32 Unicode Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "jpeg___Win32_Unicode_Release"
# PROP BASE Intermediate_Dir "jpeg___Win32_Unicode_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Unicode_Release"
# PROP Intermediate_Dir "Unicode_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MD /W3 /GX /O2 /D "_WINDOWS" /D "WIN32" /D "NDEBUG" /D "_UNICODE" /D "UNICODE" /D "_CRT_SECURE_NO_DEPRECATE" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x410 /d "_AFXDLL"
# ADD RSC /l 0x809
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "jpeg - Win32 Debug"
# Name "jpeg - Win32 Release"
# Name "jpeg - Win32 Unicode Debug"
# Name "jpeg - Win32 Unicode Release"
# Begin Source File

SOURCE=.\Jcapimin.c
# End Source File
# Begin Source File

SOURCE=.\Jcapistd.c
# End Source File
# Begin Source File

SOURCE=.\Jccoefct.c
# End Source File
# Begin Source File

SOURCE=.\Jccolor.c
# End Source File
# Begin Source File

SOURCE=.\Jcdctmgr.c
# End Source File
# Begin Source File

SOURCE=.\Jchuff.c
# End Source File
# Begin Source File

SOURCE=.\Jchuff.h
# End Source File
# Begin Source File

SOURCE=.\Jcinit.c
# End Source File
# Begin Source File

SOURCE=.\Jcmainct.c
# End Source File
# Begin Source File

SOURCE=.\Jcmarker.c
# End Source File
# Begin Source File

SOURCE=.\Jcmaster.c
# End Source File
# Begin Source File

SOURCE=.\Jcomapi.c
# End Source File
# Begin Source File

SOURCE=.\Jconfig.h
# End Source File
# Begin Source File

SOURCE=.\Jcparam.c
# End Source File
# Begin Source File

SOURCE=.\Jcphuff.c
# End Source File
# Begin Source File

SOURCE=.\Jcprepct.c
# End Source File
# Begin Source File

SOURCE=.\Jcsample.c
# End Source File
# Begin Source File

SOURCE=.\Jctrans.c
# End Source File
# Begin Source File

SOURCE=.\Jdapimin.c
# End Source File
# Begin Source File

SOURCE=.\Jdapistd.c
# End Source File
# Begin Source File

SOURCE=.\Jdatadst.c
# End Source File
# Begin Source File

SOURCE=.\Jdatasrc.c
# End Source File
# Begin Source File

SOURCE=.\Jdcoefct.c
# End Source File
# Begin Source File

SOURCE=.\Jdcolor.c
# End Source File
# Begin Source File

SOURCE=.\Jdct.h
# End Source File
# Begin Source File

SOURCE=.\Jddctmgr.c
# End Source File
# Begin Source File

SOURCE=.\Jdhuff.c
# End Source File
# Begin Source File

SOURCE=.\Jdhuff.h
# End Source File
# Begin Source File

SOURCE=.\Jdinput.c
# End Source File
# Begin Source File

SOURCE=.\Jdmainct.c
# End Source File
# Begin Source File

SOURCE=.\Jdmarker.c
# End Source File
# Begin Source File

SOURCE=.\Jdmaster.c
# End Source File
# Begin Source File

SOURCE=.\Jdmerge.c
# End Source File
# Begin Source File

SOURCE=.\Jdphuff.c
# End Source File
# Begin Source File

SOURCE=.\Jdpostct.c
# End Source File
# Begin Source File

SOURCE=.\Jdsample.c
# End Source File
# Begin Source File

SOURCE=.\Jdtrans.c
# End Source File
# Begin Source File

SOURCE=.\Jerror.c
# End Source File
# Begin Source File

SOURCE=.\Jerror.h
# End Source File
# Begin Source File

SOURCE=.\Jfdctflt.c
# End Source File
# Begin Source File

SOURCE=.\Jfdctfst.c
# End Source File
# Begin Source File

SOURCE=.\Jfdctint.c
# End Source File
# Begin Source File

SOURCE=.\Jidctflt.c
# End Source File
# Begin Source File

SOURCE=.\Jidctfst.c
# End Source File
# Begin Source File

SOURCE=.\Jidctint.c
# End Source File
# Begin Source File

SOURCE=.\Jidctred.c
# End Source File
# Begin Source File

SOURCE=.\Jinclude.h
# End Source File
# Begin Source File

SOURCE=.\Jmemmgr.c
# End Source File
# Begin Source File

SOURCE=.\Jmemnobs.c
# End Source File
# Begin Source File

SOURCE=.\Jmemsys.h
# End Source File
# Begin Source File

SOURCE=.\Jmorecfg.h
# End Source File
# Begin Source File

SOURCE=.\Jpegint.h
# End Source File
# Begin Source File

SOURCE=.\Jpeglib.h
# End Source File
# Begin Source File

SOURCE=.\Jquant1.c
# End Source File
# Begin Source File

SOURCE=.\Jquant2.c
# End Source File
# Begin Source File

SOURCE=.\Jutils.c
# End Source File
# Begin Source File

SOURCE=.\Jversion.h
# End Source File
# End Target
# End Project
