SRCS=Database.cpp \
     dataset.cpp \
     qry_dat.cpp \
     sqlitedataset.cpp

ifeq (@USE_MYSQL@,1)
SRCS+=mysqldataset.cpp
endif

LIB=dbwrappers.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
