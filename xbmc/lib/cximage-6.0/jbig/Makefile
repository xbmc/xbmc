<<<<<<< .mine
CFLAGS=-fPIC -D_LINUX 
ifeq ($(ARCH), osx)
CFLAGS+=-fno-common -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4
endif
=======
CFLAGS+=-fPIC -D_LINUX 
>>>>>>> .r15155
OBJS=jbig.o jbig_tab.o 

%o : %c
	$(CC) -c -o $@ $< $(CFLAGS)

jbig.a: $(OBJS)
	$(AR) rc $@ $(OBJS)

clean:
	$(RM) $(OBJS) jbig.a

distclean: clean

include ../Makefile.include
