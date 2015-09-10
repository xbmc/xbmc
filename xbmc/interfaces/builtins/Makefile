SRCS  = AddonBuiltins.cpp
SRCS += Builtins.cpp
SRCS += GUIBuiltins.cpp
SRCS += LibraryBuiltins.cpp
SRCS += ProfileBuiltins.cpp
SRCS += SkinBuiltins.cpp
SRCS += SystemBuiltins.cpp

LIB=builtins.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
