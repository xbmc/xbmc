SRCS=XBPython.cpp \
     XBPythonDll.cpp \
     XBPythonDllFuncs.S \
     XBPyThread.cpp \

LIB=python.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(pathsubst %.S,,$(SRCS))))
