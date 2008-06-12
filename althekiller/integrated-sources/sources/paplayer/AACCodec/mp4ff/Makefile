CC=gcc
OBJS=drms.o mp4atom.o mp4ff.o mp4meta.o mp4sample.o mp4tagupdate.o mp4util.o
CFLAGS=-fPIC -DUSE_TAGGING -DHAVE_ERRONO_H -DHAVE_SYS_STAT_H -DHAVE_SYS_TYPES_H -DHAVE_LIMITS_H -D_LINUX

%o : %c
	$(CC) -c -o $@ $< $(CFLAGS)

libfaad: $(OBJS)

clean:
	$(RM) $(OBJS)

include ../Makefile.include
