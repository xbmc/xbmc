SRCS  = AddonBuiltins.cpp
SRCS += Builtins.cpp
SRCS += LibraryBuiltins.cpp
SRCS += SkinBuiltins.cpp

LIB=builtins.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
