CFLAGS+=-O2 -fPIC -DXBMC -D_LINUX

ifeq ($(findstring osx,$(ARCH)), osx)
	CFLAGS+=-fno-common -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4
endif

OBJS=dcr.o libdcr.o

raw.a: $(OBJS)
	$(AR) rc $@ $(OBJS)

%o : %c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	$(RM) $(OBJS) raw.a

distclean: clean
