# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

!IF "$(CFG)" == ""
CFG=zlib - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to zlib - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "zlib - Win32 Release" && "$(CFG)" != "zlib - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "zlib.mak" CFG="zlib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "zlib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "zlib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "zlib - Win32 Debug"
CPP=cl.exe

!IF  "$(CFG)" == "zlib - Win32 Release"

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
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\zlib.lib"

CLEAN : 
	-@erase ".\Release\zlib.lib"
	-@erase ".\Release\Deflate.obj"
	-@erase ".\Release\Infblock.obj"
	-@erase ".\Release\Uncompr.obj"
	-@erase ".\Release\Inflate.obj"
	-@erase ".\Release\Infutil.obj"
	-@erase ".\Release\Infcodes.obj"
	-@erase ".\Release\Inftrees.obj"
	-@erase ".\Release\Adler32.obj"
	-@erase ".\Release\Trees.obj"
	-@erase ".\Release\Compress.obj"
	-@erase ".\Release\Inffast.obj"
	-@erase ".\Release\Zutil.obj"
	-@erase ".\Release\Gzio.obj"
	-@erase ".\Release\Crc32.obj"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /c
# SUBTRACT CPP /YX
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_MBCS" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/zlib.bsc" 
BSC32_SBRS=
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
LIB32_FLAGS=/nologo /out:"$(OUTDIR)/zlib.lib" 
LIB32_OBJS= \
	".\Release\Deflate.obj" \
	".\Release\Infblock.obj" \
	".\Release\Uncompr.obj" \
	".\Release\Inflate.obj" \
	".\Release\Infutil.obj" \
	".\Release\Infcodes.obj" \
	".\Release\Inftrees.obj" \
	".\Release\Adler32.obj" \
	".\Release\Trees.obj" \
	".\Release\Compress.obj" \
	".\Release\Inffast.obj" \
	".\Release\Zutil.obj" \
	".\Release\Gzio.obj" \
	".\Release\Crc32.obj"

"$(OUTDIR)\zlib.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "zlib - Win32 Debug"

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
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\zlib.lib"

CLEAN : 
	-@erase ".\Debug\zlib.lib"
	-@erase ".\Debug\Inffast.obj"
	-@erase ".\Debug\Infblock.obj"
	-@erase ".\Debug\Inftrees.obj"
	-@erase ".\Debug\Deflate.obj"
	-@erase ".\Debug\Uncompr.obj"
	-@erase ".\Debug\Adler32.obj"
	-@erase ".\Debug\Inflate.obj"
	-@erase ".\Debug\Infcodes.obj"
	-@erase ".\Debug\Crc32.obj"
	-@erase ".\Debug\Infutil.obj"
	-@erase ".\Debug\Gzio.obj"
	-@erase ".\Debug\Trees.obj"
	-@erase ".\Debug\Compress.obj"
	-@erase ".\Debug\Zutil.obj"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MDd /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /c
# SUBTRACT CPP /YX
CPP_PROJ=/nologo /MDd /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_MBCS" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/zlib.bsc" 
BSC32_SBRS=
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
LIB32_FLAGS=/nologo /out:"$(OUTDIR)/zlib.lib" 
LIB32_OBJS= \
	".\Debug\Inffast.obj" \
	".\Debug\Infblock.obj" \
	".\Debug\Inftrees.obj" \
	".\Debug\Deflate.obj" \
	".\Debug\Uncompr.obj" \
	".\Debug\Adler32.obj" \
	".\Debug\Inflate.obj" \
	".\Debug\Infcodes.obj" \
	".\Debug\Crc32.obj" \
	".\Debug\Infutil.obj" \
	".\Debug\Gzio.obj" \
	".\Debug\Trees.obj" \
	".\Debug\Compress.obj" \
	".\Debug\Zutil.obj"

"$(OUTDIR)\zlib.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "zlib - Win32 Release"
# Name "zlib - Win32 Debug"

!IF  "$(CFG)" == "zlib - Win32 Release"

!ELSEIF  "$(CFG)" == "zlib - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\Zutil.c
DEP_CPP_ZUTIL=\
	".\zutil.h"\
	".\zlib.h"\
	".\zconf.h"\
	

"$(INTDIR)\Zutil.obj" : $(SOURCE) $(DEP_CPP_ZUTIL) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Compress.c
DEP_CPP_COMPR=\
	".\zlib.h"\
	".\zconf.h"\
	

"$(INTDIR)\Compress.obj" : $(SOURCE) $(DEP_CPP_COMPR) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Crc32.c
DEP_CPP_CRC32=\
	".\zlib.h"\
	".\zconf.h"\
	

"$(INTDIR)\Crc32.obj" : $(SOURCE) $(DEP_CPP_CRC32) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Deflate.c
DEP_CPP_DEFLA=\
	".\deflate.h"\
	".\zutil.h"\
	".\zlib.h"\
	".\zconf.h"\
	

"$(INTDIR)\Deflate.obj" : $(SOURCE) $(DEP_CPP_DEFLA) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Gzio.c
DEP_CPP_GZIO_=\
	".\zutil.h"\
	".\zlib.h"\
	".\zconf.h"\
	

"$(INTDIR)\Gzio.obj" : $(SOURCE) $(DEP_CPP_GZIO_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Infblock.c
DEP_CPP_INFBL=\
	".\zutil.h"\
	".\infblock.h"\
	".\inftrees.h"\
	".\infcodes.h"\
	".\infutil.h"\
	".\zlib.h"\
	".\zconf.h"\
	

"$(INTDIR)\Infblock.obj" : $(SOURCE) $(DEP_CPP_INFBL) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Infcodes.c
DEP_CPP_INFCO=\
	".\zutil.h"\
	".\inftrees.h"\
	".\infblock.h"\
	".\infcodes.h"\
	".\infutil.h"\
	".\inffast.h"\
	".\zlib.h"\
	".\zconf.h"\
	

"$(INTDIR)\Infcodes.obj" : $(SOURCE) $(DEP_CPP_INFCO) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Inffast.c
DEP_CPP_INFFA=\
	".\zutil.h"\
	".\inftrees.h"\
	".\infblock.h"\
	".\infcodes.h"\
	".\infutil.h"\
	".\inffast.h"\
	".\zlib.h"\
	".\zconf.h"\
	

"$(INTDIR)\Inffast.obj" : $(SOURCE) $(DEP_CPP_INFFA) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Inflate.c
DEP_CPP_INFLA=\
	".\zutil.h"\
	".\infblock.h"\
	".\zlib.h"\
	".\zconf.h"\
	

"$(INTDIR)\Inflate.obj" : $(SOURCE) $(DEP_CPP_INFLA) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Inftrees.c
DEP_CPP_INFTR=\
	".\zutil.h"\
	".\inftrees.h"\
	".\zlib.h"\
	".\zconf.h"\
	

"$(INTDIR)\Inftrees.obj" : $(SOURCE) $(DEP_CPP_INFTR) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Infutil.c
DEP_CPP_INFUT=\
	".\zutil.h"\
	".\infblock.h"\
	".\inftrees.h"\
	".\infcodes.h"\
	".\infutil.h"\
	".\zlib.h"\
	".\zconf.h"\
	

"$(INTDIR)\Infutil.obj" : $(SOURCE) $(DEP_CPP_INFUT) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Trees.c
DEP_CPP_TREES=\
	".\deflate.h"\
	".\zutil.h"\
	".\zlib.h"\
	".\zconf.h"\
	

"$(INTDIR)\Trees.obj" : $(SOURCE) $(DEP_CPP_TREES) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Uncompr.c
DEP_CPP_UNCOM=\
	".\zlib.h"\
	".\zconf.h"\
	

"$(INTDIR)\Uncompr.obj" : $(SOURCE) $(DEP_CPP_UNCOM) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Adler32.c
DEP_CPP_ADLER=\
	".\zlib.h"\
	".\zconf.h"\
	

"$(INTDIR)\Adler32.obj" : $(SOURCE) $(DEP_CPP_ADLER) "$(INTDIR)"


# End Source File
# End Target
# End Project
################################################################################
