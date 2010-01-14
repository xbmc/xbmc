INCLUDES=-I../../.. -I. -I../../ -I../../linux -I../../../guilib -I.
SRCS=XBPython.cpp XBPythonDll.cpp XBPyThread.cpp XBPythonDllFuncs.S

LIB=python.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(pathsubst %.S,,$(SRCS))))
