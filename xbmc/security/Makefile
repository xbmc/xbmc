SRCS = KeyringManager.cpp
SRCS+= MemoryKeyringManager.cpp
SRCS+= XMLKeyringManager.cpp

LIB = security.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(patsubst %.S,,$(SRCS))))
