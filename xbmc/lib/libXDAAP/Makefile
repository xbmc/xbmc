AR = ar
ARFLAGS = rus
RM = rm -f

CXX=g++
CC=gcc

DEFINES := -D_LINUX 
OBJS := daap.o debug.o dmap_generics.o global.o httpClient.o ioloop.o libXDAAP.o threadpool.o Authentication/hasher.o Authentication/md5.o

.c.o:
	rm -f ${<:.c=.o}
	$(CC) -c $(CFLAGS) $(DEFINES) $(INCLUDES) $< -o ${<:.c=.o}

libxdaap-i486-linux.a: $(OBJS)
	$(AR) $(ARFLAGS) libxdaap-i486-linux.a $(OBJS)

all: libxdaap-i486-linux.a

clean:
	$(RM) $(OBJS) libxdaap-i486-linux.a
