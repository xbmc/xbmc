SRCS     = rand_r.c
SRCS    += getdelim.c
LIB      = bionic_supplement.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
