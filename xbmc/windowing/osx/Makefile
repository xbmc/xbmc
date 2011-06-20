SRCS= \
      WinEventsOSX \
      WinSystemOSX.mm \
      WinSystemOSXGL.mm
     
.SUFFIXES : .m .mm
LIB=windowing_osx.a

include ../../../Makefile.include
-include $(patsubst %.mm,%.P,$(SRCS))
