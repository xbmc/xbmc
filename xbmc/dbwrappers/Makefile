SRCS=Database.cpp \
     dataset.cpp \
     DynamicDatabase.cpp \
     mysqldataset.cpp \
     qry_dat.cpp \
     sqlitedataset.cpp \

LIB=dbwrappers.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
