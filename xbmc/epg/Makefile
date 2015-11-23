INCLUDES=-I. -I.. -I../../ -I../linux -I../cores -I../../guilib -I../posix 

SRCS=EpgInfoTag.cpp \
	EpgSearchFilter.cpp \
	Epg.cpp \
	EpgContainer.cpp \
	EpgDatabase.cpp \
	GUIEPGGridContainer.cpp

LIB=epg.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
