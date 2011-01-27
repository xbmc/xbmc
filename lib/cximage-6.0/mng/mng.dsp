# Microsoft Developer Studio Project File - Name="mng" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=mng - Win32 Unicode Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mng.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mng.mak" CFG="mng - Win32 Unicode Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mng - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "mng - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "mng - Win32 Unicode Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "mng - Win32 Unicode Release" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "mng - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "MNG_SUPPORT_DISPLAY" /D "MNG_SUPPORT_READ" /D "MNG_SUPPORT_WRITE" /D "MNG_ACCESS_CHUNKS" /D "MNG_STORE_CHUNKS" /D "_CRT_SECURE_NO_DEPRECATE" /D "MNG_ERROR_TELLTALE" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x410 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "mng - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "MNG_SUPPORT_DISPLAY" /D "MNG_SUPPORT_READ" /D "MNG_SUPPORT_WRITE" /D "MNG_ACCESS_CHUNKS" /D "MNG_STORE_CHUNKS" /D "_CRT_SECURE_NO_DEPRECATE" /D "MNG_ERROR_TELLTALE" /FD /GZ /c
# SUBTRACT CPP /Fr /YX
# ADD BASE RSC /l 0x410 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "mng - Win32 Unicode Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "mng___Win32_Unicode_Debug"
# PROP BASE Intermediate_Dir "mng___Win32_Unicode_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Unicode_Debug"
# PROP Intermediate_Dir "Unicode_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "MNG_SUPPORT_DISPLAY" /D "MNG_SUPPORT_READ" /D "MNG_SUPPORT_WRITE" /D "MNG_ACCESS_CHUNKS" /D "MNG_STORE_CHUNKS" /D "_AFXDLL" /FR /FD /GZ /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "MNG_SUPPORT_DISPLAY" /D "MNG_SUPPORT_READ" /D "MNG_SUPPORT_WRITE" /D "MNG_ACCESS_CHUNKS" /D "MNG_STORE_CHUNKS" /D "_UNICODE" /D "UNICODE" /D "_CRT_SECURE_NO_DEPRECATE" /D "MNG_ERROR_TELLTALE" /FD /GZ /c
# SUBTRACT CPP /Fr /YX
# ADD BASE RSC /l 0x410 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "mng - Win32 Unicode Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "mng___Win32_Unicode_Release"
# PROP BASE Intermediate_Dir "mng___Win32_Unicode_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Unicode_Release"
# PROP Intermediate_Dir "Unicode_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /D "MNG_SUPPORT_DISPLAY" /D "MNG_SUPPORT_READ" /D "MNG_SUPPORT_WRITE" /D "MNG_ACCESS_CHUNKS" /D "MNG_STORE_CHUNKS" /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "MNG_SUPPORT_DISPLAY" /D "MNG_SUPPORT_READ" /D "MNG_SUPPORT_WRITE" /D "MNG_ACCESS_CHUNKS" /D "MNG_STORE_CHUNKS" /D "_UNICODE" /D "UNICODE" /D "_CRT_SECURE_NO_DEPRECATE" /D "MNG_ERROR_TELLTALE" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x410 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "mng - Win32 Release"
# Name "mng - Win32 Debug"
# Name "mng - Win32 Unicode Debug"
# Name "mng - Win32 Unicode Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\libmng_callback_xs.c
# End Source File
# Begin Source File

SOURCE=.\libmng_chunk_descr.c
# End Source File
# Begin Source File

SOURCE=.\libmng_chunk_descr.h
# End Source File
# Begin Source File

SOURCE=.\libmng_chunk_io.c
# End Source File
# Begin Source File

SOURCE=.\libmng_chunk_prc.c
# End Source File
# Begin Source File

SOURCE=.\libmng_chunk_xs.c
# End Source File
# Begin Source File

SOURCE=.\libmng_cms.c
# End Source File
# Begin Source File

SOURCE=.\libmng_display.c
# End Source File
# Begin Source File

SOURCE=.\libmng_dither.c
# End Source File
# Begin Source File

SOURCE=.\libmng_error.c
# End Source File
# Begin Source File

SOURCE=.\libmng_filter.c
# End Source File
# Begin Source File

SOURCE=.\libmng_hlapi.c
# End Source File
# Begin Source File

SOURCE=.\libmng_jpeg.c
# End Source File
# Begin Source File

SOURCE=.\libmng_object_prc.c
# End Source File
# Begin Source File

SOURCE=.\libmng_pixels.c
# End Source File
# Begin Source File

SOURCE=.\libmng_prop_xs.c
# End Source File
# Begin Source File

SOURCE=.\libmng_read.c
# End Source File
# Begin Source File

SOURCE=.\libmng_trace.c
# End Source File
# Begin Source File

SOURCE=.\libmng_write.c
# End Source File
# Begin Source File

SOURCE=.\libmng_zlib.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\libmng.h
# End Source File
# Begin Source File

SOURCE=.\libmng_chunk_io.h
# End Source File
# Begin Source File

SOURCE=.\libmng_chunk_prc.h
# End Source File
# Begin Source File

SOURCE=.\libmng_chunks.h
# End Source File
# Begin Source File

SOURCE=.\libmng_cms.h
# End Source File
# Begin Source File

SOURCE=.\libmng_conf.h
# End Source File
# Begin Source File

SOURCE=.\libmng_data.h
# End Source File
# Begin Source File

SOURCE=.\libmng_display.h
# End Source File
# Begin Source File

SOURCE=.\libmng_dither.h
# End Source File
# Begin Source File

SOURCE=.\libmng_error.h
# End Source File
# Begin Source File

SOURCE=.\libmng_filter.h
# End Source File
# Begin Source File

SOURCE=.\libmng_jpeg.h
# End Source File
# Begin Source File

SOURCE=.\libmng_memory.h
# End Source File
# Begin Source File

SOURCE=.\libmng_object_prc.h
# End Source File
# Begin Source File

SOURCE=.\libmng_objects.h
# End Source File
# Begin Source File

SOURCE=.\libmng_pixels.h
# End Source File
# Begin Source File

SOURCE=.\libmng_read.h
# End Source File
# Begin Source File

SOURCE=.\libmng_trace.h
# End Source File
# Begin Source File

SOURCE=.\libmng_types.h
# End Source File
# Begin Source File

SOURCE=.\libmng_write.h
# End Source File
# Begin Source File

SOURCE=.\libmng_zlib.h
# End Source File
# End Group
# End Target
# End Project
