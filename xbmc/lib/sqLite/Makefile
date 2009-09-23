INCLUDES=-I. -I../../ -I../../linux -I../../../guilib
SRCS=dataset.cpp qry_dat.cpp sqlitedataset.cpp
LIB=sqllite.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
