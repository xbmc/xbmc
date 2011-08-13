ARCH=i486-linux

OBJS=mads.o

CLEAN_FILES=mads

all: mads.o

mads.o: mads.pas
	fpc -Mdelphi -vh -O3 mads.pas

include ../../../Makefile.include
