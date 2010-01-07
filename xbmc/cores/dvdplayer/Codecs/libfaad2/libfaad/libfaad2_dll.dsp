# Microsoft Developer Studio Project File - Name="libfaad2_dll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=libfaad2_dll - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libfaad2_dll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libfaad2_dll.mak" CFG="libfaad2_dll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libfaad2_dll - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libfaad2_dll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libfaad2_dll - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libfaad2_dll___Win32_Release"
# PROP BASE Intermediate_Dir "libfaad2_dll___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseDLL"
# PROP Intermediate_Dir "ReleaseDLL"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "libfaad2_dll_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "fftw" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "libfaad2_dll_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 /nologo /dll /machine:I386 /out:"ReleaseDLL/libfaad2.dll"

!ELSEIF  "$(CFG)" == "libfaad2_dll - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "libfaad2_dll___Win32_Debug"
# PROP BASE Intermediate_Dir "libfaad2_dll___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugDLL"
# PROP Intermediate_Dir "DebugDLL"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "libfaad2_dll_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "fftw" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "libfaad2_dll_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /dll /debug /machine:I386 /out:"DebugDLL/libfaad2.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "libfaad2_dll - Win32 Release"
# Name "libfaad2_dll - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\bits.c
# End Source File
# Begin Source File

SOURCE=.\cfft.c
# End Source File
# Begin Source File

SOURCE=.\common.c
# End Source File
# Begin Source File

SOURCE=.\decoder.c
# End Source File
# Begin Source File

SOURCE=.\drc.c
# End Source File
# Begin Source File

SOURCE=.\error.c
# End Source File
# Begin Source File

SOURCE=.\filtbank.c
# End Source File
# Begin Source File

SOURCE=.\hcr.c
# End Source File
# Begin Source File

SOURCE=.\huffman.c
# End Source File
# Begin Source File

SOURCE=.\ic_predict.c
# End Source File
# Begin Source File

SOURCE=.\is.c
# End Source File
# Begin Source File

SOURCE=.\lt_predict.c
# End Source File
# Begin Source File

SOURCE=.\mdct.c
# End Source File
# Begin Source File

SOURCE=.\mp4.c
# End Source File
# Begin Source File

SOURCE=.\ms.c
# End Source File
# Begin Source File

SOURCE=.\output.c
# End Source File
# Begin Source File

SOURCE=.\pns.c
# End Source File
# Begin Source File

SOURCE=.\ps_dec.c
# End Source File
# Begin Source File

SOURCE=.\ps_syntax.c
# End Source File
# Begin Source File

SOURCE=.\pulse.c
# End Source File
# Begin Source File

SOURCE=.\rvlc.c
# End Source File
# Begin Source File

SOURCE=.\sbr_dct.c
# End Source File
# Begin Source File

SOURCE=.\sbr_dec.c
# End Source File
# Begin Source File

SOURCE=.\sbr_e_nf.c
# End Source File
# Begin Source File

SOURCE=.\sbr_fbt.c
# End Source File
# Begin Source File

SOURCE=.\sbr_hfadj.c
# End Source File
# Begin Source File

SOURCE=.\sbr_hfgen.c
# End Source File
# Begin Source File

SOURCE=.\sbr_huff.c
# End Source File
# Begin Source File

SOURCE=.\sbr_qmf.c
# End Source File
# Begin Source File

SOURCE=.\sbr_syntax.c
# End Source File
# Begin Source File

SOURCE=.\sbr_tf_grid.c
# End Source File
# Begin Source File

SOURCE=.\specrec.c
# End Source File
# Begin Source File

SOURCE=.\ssr.c
# End Source File
# Begin Source File

SOURCE=.\ssr_fb.c
# End Source File
# Begin Source File

SOURCE=.\ssr_ipqf.c
# End Source File
# Begin Source File

SOURCE=.\syntax.c
# End Source File
# Begin Source File

SOURCE=.\tns.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\analysis.h
# End Source File
# Begin Source File

SOURCE=.\bits.h
# End Source File
# Begin Source File

SOURCE=.\cfft.h
# End Source File
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=.\data.h
# End Source File
# Begin Source File

SOURCE=.\decoder.h
# End Source File
# Begin Source File

SOURCE=.\dither.h
# End Source File
# Begin Source File

SOURCE=.\drc.h
# End Source File
# Begin Source File

SOURCE=.\error.h
# End Source File
# Begin Source File

SOURCE=.\filtbank.h
# End Source File
# Begin Source File

SOURCE=.\codebook\hcb.h
# End Source File
# Begin Source File

SOURCE=.\huffman.h
# End Source File
# Begin Source File

SOURCE=.\ic_predict.h
# End Source File
# Begin Source File

SOURCE=.\is.h
# End Source File
# Begin Source File

SOURCE=.\kbd_win.h
# End Source File
# Begin Source File

SOURCE=.\lt_predict.h
# End Source File
# Begin Source File

SOURCE=.\mdct.h
# End Source File
# Begin Source File

SOURCE=.\mp4.h
# End Source File
# Begin Source File

SOURCE=.\ms.h
# End Source File
# Begin Source File

SOURCE=.\output.h
# End Source File
# Begin Source File

SOURCE=.\pns.h
# End Source File
# Begin Source File

SOURCE=.\pulse.h
# End Source File
# Begin Source File

SOURCE=.\rvlc.h
# End Source File
# Begin Source File

SOURCE=.\specrec.h
# End Source File
# Begin Source File

SOURCE=.\ssr.h
# End Source File
# Begin Source File

SOURCE=.\syntax.h
# End Source File
# Begin Source File

SOURCE=.\Tns.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\libfaad2.def
# End Source File
# End Target
# End Project
