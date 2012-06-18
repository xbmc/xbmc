SRCS= \
	Exception.cpp \
	ilog.cpp

LIB=commons.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(patsubst %.S,,$(SRCS))))
