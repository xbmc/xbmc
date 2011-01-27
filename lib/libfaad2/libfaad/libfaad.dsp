# Microsoft Developer Studio Project File - Name="libfaad" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libfaad - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libfaad.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libfaad.mak" CFG="libfaad - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libfaad - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libfaad - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=xicl6.exe
RSC=rc.exe

!IF  "$(CFG)" == "libfaad - Win32 Release"

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
# ADD CPP /nologo /G6 /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x413 /d "NDEBUG"
# ADD RSC /l 0x413 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libfaad - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /ZI /Od /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x413 /d "_DEBUG"
# ADD RSC /l 0x413 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libfaad - Win32 Release"
# Name "libfaad - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
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

SOURCE=.\drm_dec.c
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

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "codebook"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\codebook\hcb_1.h
# End Source File
# Begin Source File

SOURCE=.\codebook\hcb_10.h
# End Source File
# Begin Source File

SOURCE=.\codebook\hcb_11.h
# End Source File
# Begin Source File

SOURCE=.\codebook\hcb_2.h
# End Source File
# Begin Source File

SOURCE=.\codebook\hcb_3.h
# End Source File
# Begin Source File

SOURCE=.\codebook\hcb_4.h
# End Source File
# Begin Source File

SOURCE=.\codebook\hcb_5.h
# End Source File
# Begin Source File

SOURCE=.\codebook\hcb_6.h
# End Source File
# Begin Source File

SOURCE=.\codebook\hcb_7.h
# End Source File
# Begin Source File

SOURCE=.\codebook\hcb_8.h
# End Source File
# Begin Source File

SOURCE=.\codebook\hcb_9.h
# End Source File
# Begin Source File

SOURCE=.\codebook\hcb_sf.h
# End Source File
# End Group
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

SOURCE=.\cfft_tab.h
# End Source File
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=.\decoder.h
# End Source File
# Begin Source File

SOURCE=.\drc.h
# End Source File
# Begin Source File

SOURCE=.\drm_dec.h
# End Source File
# Begin Source File

SOURCE=.\error.h
# End Source File
# Begin Source File

SOURCE=.\filtbank.h
# End Source File
# Begin Source File

SOURCE=.\fixed.h
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

SOURCE=.\iq_table.h
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

SOURCE=.\mdct_tab.h
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

SOURCE=.\ps_dec.h
# End Source File
# Begin Source File

SOURCE=.\ps_tables.h
# End Source File
# Begin Source File

SOURCE=.\pulse.h
# End Source File
# Begin Source File

SOURCE=.\rvlc.h
# End Source File
# Begin Source File

SOURCE=.\sbr_dct.h
# End Source File
# Begin Source File

SOURCE=.\sbr_dec.h
# End Source File
# Begin Source File

SOURCE=.\sbr_e_nf.h
# End Source File
# Begin Source File

SOURCE=.\sbr_fbt.h
# End Source File
# Begin Source File

SOURCE=.\sbr_hfadj.h
# End Source File
# Begin Source File

SOURCE=.\sbr_hfgen.h
# End Source File
# Begin Source File

SOURCE=.\sbr_huff.h
# End Source File
# Begin Source File

SOURCE=.\sbr_noise.h
# End Source File
# Begin Source File

SOURCE=.\sbr_qmf.h
# End Source File
# Begin Source File

SOURCE=.\sbr_qmf_c.h
# End Source File
# Begin Source File

SOURCE=.\sbr_syntax.h
# End Source File
# Begin Source File

SOURCE=.\sbr_tf_grid.h
# End Source File
# Begin Source File

SOURCE=.\sine_win.h
# End Source File
# Begin Source File

SOURCE=.\specrec.h
# End Source File
# Begin Source File

SOURCE=.\ssr.h
# End Source File
# Begin Source File

SOURCE=.\structs.h
# End Source File
# Begin Source File

SOURCE=.\syntax.h
# End Source File
# Begin Source File

SOURCE=.\tns.h
# End Source File
# End Group
# End Target
# End Project
