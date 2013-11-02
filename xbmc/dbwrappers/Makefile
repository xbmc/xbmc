SRCS=Database.cpp \
     DatabaseQuery.cpp \
     dataset.cpp \
     mysqldataset.cpp \
     qry_dat.cpp \
     sqlitedataset.cpp \

LIB=dbwrappers.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
