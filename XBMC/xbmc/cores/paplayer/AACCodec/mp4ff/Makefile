CC=gcc

OBJS=drms.o mp4atom.o mp4ff.o mp4meta.o mp4sample.o mp4tagupdate.o mp4util.o

CFLAGS=-fPIC -DUSE_TAGGING -DHAVE_ERRONO_H -DHAVE_SYS_STAT_H -DHAVE_SYS_TYPES_H -DHAVE_LIMITS_H -D_LINUX
ifeq ($(findstring osx,$(ARCH)), osx)
	CFLAGS+=-isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4 -fno-common
endif

%o : %c
	$(CC) -c -o $@ $< $(CFLAGS)

libfaad: $(OBJS)

clean:
	$(RM) $(OBJS)

include ../Makefile.include
