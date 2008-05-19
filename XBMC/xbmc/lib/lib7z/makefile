PROG = 7zDec.exe

!IFDEF CPU
LIBS = $(LIBS) bufferoverflowU.lib 
CFLAGS = $(CFLAGS) -GS- -Zc:forScope -WX -GS- -Gy -W4 
!ENDIF

!IFNDEF O
!IFDEF CPU
O=$(CPU)
!ELSE
O=O
!ENDIF
!ENDIF

CFLAGS = $(CFLAGS) -nologo -c -Fo$O/ -D_LZMA_IN_CB
CFLAGS_O1 = $(CFLAGS) -O1
CFLAGS_O2 = $(CFLAGS) -O2

LFLAGS = $(LFLAGS) -nologo -OPT:NOWIN98 -OPT:REF

PROGPATH = $O\$(PROG)

COMPL_O1   = $(CPP) $(CFLAGS_O1) $**
COMPL_O2   = $(CPP) $(CFLAGS_O2) $**
COMPL      = $(CPP) $(CFLAGS_O1) $**

C_OBJS = \
  $O\7zCrc.obj \


7Z_OBJS = \
  $O\7zAlloc.obj \
  $O\7zBuffer.obj \
  $O\7zDecode.obj \
  $O\7zExtract.obj \
  $O\7zHeader.obj \
  $O\7zIn.obj \
  $O\7zItem.obj \
  $O\7zMain.obj \
  $O\7zMethodID.obj \

OBJS = \
  $(7Z_OBJS) \
  $O\LzmaDecode.obj \
  $O\BranchX86.obj \
  $O\BranchX86_2.obj \
  $(C_OBJS) \

all: $(PROGPATH) 

clean:
	-del /Q $(PROGPATH) $O\*.exe $O\*.dll $O\*.obj $O\*.lib $O\*.exp $O\*.res $O\*.pch 

$O:
	if not exist "$O" mkdir "$O"

$(PROGPATH): $O $(OBJS)
	link $(LFLAGS) -out:$(PROGPATH) $(OBJS) $(LIBS)


$(7Z_OBJS): $(*B).c
	$(COMPL)
$O\LzmaDecode.obj: ../../Compress/Lzma/$(*B).c
	$(COMPL_O2)

$O\BranchX86.obj: ../../Compress/Branch/$(*B).c
	$(COMPL_O2)

$O\BranchX86_2.obj: ../../Compress/Branch/$(*B).c
	$(COMPL_O2)

$(C_OBJS): ../../$(*B).c
	$(COMPL_O2)
