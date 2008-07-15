AR = ar
ARFLAGS = rus
RM = rm -f

CXX=g++
CC=gcc

DEFINES += -D_LINUX

OBJS := ccbuffer.o ccutil.o ccxclient.o ccxclientconnxbox.o ccxdiscover.o ccxencode.o ccxmltrans.o

.c.o:
	rm -f ${<:.c=.o}
	$(CC) -c $(CFLAGS) $(DEFINES) $(INCLUDES) $< -o ${<:.c=.o}

libxbms.a: $(OBJS)
	$(AR) $(ARFLAGS) libxbms.a $(OBJS)

all: libxbms.a

clean:
	$(RM) $(OBJS) libxbms.a
