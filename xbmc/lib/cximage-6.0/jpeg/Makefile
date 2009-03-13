CC=gcc
CFLAGS+=-O2 -fPIC -D_LINUX

ifeq ($(findstring osx,$(ARCH)), osx)
	CFLAGS+=-fno-common -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4
endif
OBJS=jcapimin.o jcapistd.o jccoefct.o jccolor.o jcdctmgr.o jchuff.o jcinit.o jcmainct.o jcmarker.o jcmaster.o jcomapi.o jcparam.o jcphuff.o jcprepct.o jcsample.o jctrans.o jdapimin.o jdapistd.o jdatadst.o jdatasrc.o jdcoefct.o jdcolor.o jddctmgr.o jdhuff.o jdinput.o jdmainct.o jdmarker.o jdmaster.o jdmerge.o jdphuff.o jdpostct.o jdsample.o jdtrans.o jerror.o jfdctflt.o jfdctfst.o jfdctint.o jidctflt.o jidctfst.o jidctint.o jidctred.o jmemmgr.o jmemnobs.o jquant1.o jquant2.o jutils.o

%o : %c
	$(CC) $(CFLAGS) -c -o $@ $< 

jpeg.a: $(OBJS)
	$(AR) rc $@ $(OBJS)

clean:
	$(RM) $(OBJS) jpeg.a
