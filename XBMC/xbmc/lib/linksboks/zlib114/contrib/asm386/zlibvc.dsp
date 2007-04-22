# Microsoft Developer Studio Project File - Name="zlibvc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102
# TARGTYPE "Win32 (ALPHA) Dynamic-Link Library" 0x0602

CFG=zlibvc - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "zlibvc.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "zlibvc.mak" CFG="zlibvc - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "zlibvc - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "zlibvc - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "zlibvc - Win32 ReleaseAxp" (based on\
 "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE "zlibvc - Win32 ReleaseWithoutAsm" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "zlibvc - Win32 ReleaseWithoutCrtdll" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "zlibvc - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_WIN32" /D "BUILD_ZLIBDLL" /D "ZLIB_DLL" /D "DYNAMIC_CRC_TABLE" /D "ASMV" /FAcs /FR /FD /c
# SUBTRACT CPP /YX
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 gvmat32.obj kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib crtdll.lib /nologo /subsystem:windows /dll /map /machine:I386 /nodefaultlib /out:".\Release\zlib.dll"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_WIN32" /D "BUILD_ZLIBDLL" /D "ZLIB_DLL" /FD /c
# SUBTRACT CPP /YX
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\Debug\zlib.dll"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseAxp"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "zlibvc__"
# PROP BASE Intermediate_Dir "zlibvc__"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "zlibvc__"
# PROP Intermediate_Dir "zlibvc__"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
CPP=cl.exe
# ADD BASE CPP /nologo /MT /Gt0 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WIN32" /D "BUILD_ZLIBDLL" /D "ZLIB_DLL" /D "DYNAMIC_CRC_TABLE" /FAcs /FR /YX /FD /c
# ADD CPP /nologo /MT /Gt0 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WIN32" /D "BUILD_ZLIBDLL" /D "ZLIB_DLL" /D "DYNAMIC_CRC_TABLE" /FAcs /FR /FD /c
# SUBTRACT CPP /YX
RSC=rc.exe
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 crtdll.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /map /machine:ALPHA /nodefaultlib /out:".\Release\zlib.dll"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 crtdll.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /map /machine:ALPHA /nodefaultlib /out:"zlibvc__\zlib.dll"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutAsm"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "zlibvc_0"
# PROP BASE Intermediate_Dir "zlibvc_0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "zlibvc_0"
# PROP Intermediate_Dir "zlibvc_0"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_WIN32" /D "BUILD_ZLIBDLL" /D "ZLIB_DLL" /D "DYNAMIC_CRC_TABLE" /FAcs /FR /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_WIN32" /D "BUILD_ZLIBDLL" /D "ZLIB_DLL" /D "DYNAMIC_CRC_TABLE" /FAcs /FR /FD /c
# SUBTRACT CPP /YX
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib crtdll.lib /nologo /subsystem:windows /dll /map /machine:I386 /nodefaultlib /out:".\Release\zlib.dll"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib crtdll.lib /nologo /subsystem:windows /dll /map /machine:I386 /nodefaultlib /out:".\zlibvc_0\zlib.dll"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutCrtdll"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "zlibvc_1"
# PROP BASE Intermediate_Dir "zlibvc_1"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "zlibvc_1"
# PROP Intermediate_Dir "zlibvc_1"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_WIN32" /D "BUILD_ZLIBDLL" /D "ZLIB_DLL" /D "DYNAMIC_CRC_TABLE" /D "ASMV" /FAcs /FR /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_WIN32" /D "BUILD_ZLIBDLL" /D "ZLIB_DLL" /D "DYNAMIC_CRC_TABLE" /D "ASMV" /FAcs /FR /FD /c
# SUBTRACT CPP /YX
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 gvmat32.obj kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib crtdll.lib /nologo /subsystem:windows /dll /map /machine:I386 /nodefaultlib /out:".\Release\zlib.dll"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 gvmat32.obj kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib crtdll.lib /nologo /subsystem:windows /dll /map /machine:I386 /nodefaultlib /out:".\zlibvc_1\zlib.dll"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "zlibvc - Win32 Release"
# Name "zlibvc - Win32 Debug"
# Name "zlibvc - Win32 ReleaseAxp"
# Name "zlibvc - Win32 ReleaseWithoutAsm"
# Name "zlibvc - Win32 ReleaseWithoutCrtdll"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\adler32.c

!IF  "$(CFG)" == "zlibvc - Win32 Release"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseAxp"

DEP_CPP_ADLER=\
	".\zconf.h"\
	".\zlib.h"\
	

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutAsm"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutCrtdll"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\compress.c

!IF  "$(CFG)" == "zlibvc - Win32 Release"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseAxp"

DEP_CPP_COMPR=\
	".\zconf.h"\
	".\zlib.h"\
	

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutAsm"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutCrtdll"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\crc32.c

!IF  "$(CFG)" == "zlibvc - Win32 Release"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseAxp"

DEP_CPP_CRC32=\
	".\zconf.h"\
	".\zlib.h"\
	

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutAsm"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutCrtdll"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\deflate.c

!IF  "$(CFG)" == "zlibvc - Win32 Release"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseAxp"

DEP_CPP_DEFLA=\
	".\deflate.h"\
	".\zconf.h"\
	".\zlib.h"\
	".\zutil.h"\
	

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutAsm"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutCrtdll"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gvmat32c.c

!IF  "$(CFG)" == "zlibvc - Win32 Release"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseAxp"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutAsm"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutCrtdll"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gzio.c

!IF  "$(CFG)" == "zlibvc - Win32 Release"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseAxp"

DEP_CPP_GZIO_=\
	".\zconf.h"\
	".\zlib.h"\
	".\zutil.h"\
	

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutAsm"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutCrtdll"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\infblock.c

!IF  "$(CFG)" == "zlibvc - Win32 Release"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseAxp"

DEP_CPP_INFBL=\
	".\infblock.h"\
	".\infcodes.h"\
	".\inftrees.h"\
	".\infutil.h"\
	".\zconf.h"\
	".\zlib.h"\
	".\zutil.h"\
	

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutAsm"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutCrtdll"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\infcodes.c

!IF  "$(CFG)" == "zlibvc - Win32 Release"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseAxp"

DEP_CPP_INFCO=\
	".\infblock.h"\
	".\infcodes.h"\
	".\inffast.h"\
	".\inftrees.h"\
	".\infutil.h"\
	".\zconf.h"\
	".\zlib.h"\
	".\zutil.h"\
	

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutAsm"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutCrtdll"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\inffast.c

!IF  "$(CFG)" == "zlibvc - Win32 Release"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseAxp"

DEP_CPP_INFFA=\
	".\infblock.h"\
	".\infcodes.h"\
	".\inffast.h"\
	".\inftrees.h"\
	".\infutil.h"\
	".\zconf.h"\
	".\zlib.h"\
	".\zutil.h"\
	

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutAsm"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutCrtdll"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\inflate.c

!IF  "$(CFG)" == "zlibvc - Win32 Release"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseAxp"

DEP_CPP_INFLA=\
	".\infblock.h"\
	".\zconf.h"\
	".\zlib.h"\
	".\zutil.h"\
	

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutAsm"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutCrtdll"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\inftrees.c

!IF  "$(CFG)" == "zlibvc - Win32 Release"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseAxp"

DEP_CPP_INFTR=\
	".\inftrees.h"\
	".\zconf.h"\
	".\zlib.h"\
	".\zutil.h"\
	

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutAsm"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutCrtdll"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\infutil.c

!IF  "$(CFG)" == "zlibvc - Win32 Release"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseAxp"

DEP_CPP_INFUT=\
	".\infblock.h"\
	".\infcodes.h"\
	".\inftrees.h"\
	".\infutil.h"\
	".\zconf.h"\
	".\zlib.h"\
	".\zutil.h"\
	

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutAsm"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutCrtdll"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\trees.c

!IF  "$(CFG)" == "zlibvc - Win32 Release"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseAxp"

DEP_CPP_TREES=\
	".\deflate.h"\
	".\zconf.h"\
	".\zlib.h"\
	".\zutil.h"\
	

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutAsm"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutCrtdll"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\uncompr.c

!IF  "$(CFG)" == "zlibvc - Win32 Release"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseAxp"

DEP_CPP_UNCOM=\
	".\zconf.h"\
	".\zlib.h"\
	

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutAsm"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutCrtdll"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\unzip.c

!IF  "$(CFG)" == "zlibvc - Win32 Release"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseAxp"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutAsm"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutCrtdll"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\zip.c

!IF  "$(CFG)" == "zlibvc - Win32 Release"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseAxp"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutAsm"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutCrtdll"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\zlib.rc
# End Source File
# Begin Source File

SOURCE=.\zlibvc.def
# End Source File
# Begin Source File

SOURCE=.\zutil.c

!IF  "$(CFG)" == "zlibvc - Win32 Release"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseAxp"

DEP_CPP_ZUTIL=\
	".\zconf.h"\
	".\zlib.h"\
	".\zutil.h"\
	

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutAsm"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 ReleaseWithoutCrtdll"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\deflate.h
# End Source File
# Begin Source File

SOURCE=.\infblock.h
# End Source File
# Begin Source File

SOURCE=.\infcodes.h
# End Source File
# Begin Source File

SOURCE=.\inffast.h
# End Source File
# Begin Source File

SOURCE=.\inftrees.h
# End Source File
# Begin Source File

SOURCE=.\infutil.h
# End Source File
# Begin Source File

SOURCE=.\zconf.h
# End Source File
# Begin Source File

SOURCE=.\zlib.h
# End Source File
# Begin Source File

SOURCE=.\zutil.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
