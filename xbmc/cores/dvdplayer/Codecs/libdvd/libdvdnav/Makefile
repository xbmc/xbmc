include config.mak

.SUFFIXES: .so

AR=ar
LD=ld
RANLIB=ranlib

VPATH+= $(SRC_PATH_BARE)/src
SRCS = dvdnav.c highlight.c navigation.c read_cache.c remap.c searching.c settings.c

VPATH+= $(SRC_PATH_BARE)/src/vm
SRCS+= decoder.c vm.c vmcmd.c

HEADERS = src/dvd_types.h src/dvdnav.h src/dvdnav_events.h

CFLAGS += $(USEDEBUG) -Wall -funsigned-char
CFLAGS += -I$(CURDIR) -I$(SRC_PATH)/src -I$(SRC_PATH)/src/vm
CFLAGS += -DDVDNAV_COMPILE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE
CFLAGS += -DHAVE_CONFIG_H -DHAVE_DLFCN_H

L=libdvdnav
MINI_L=libdvdnavmini
CFLAGS += $(DVDREAD_CFLAGS)

LIB = $(L).a
SHLIB = $(L).so
MINI_SHLIB = $(MINI_L).so

.OBJDIR=        obj
DEPFLAG = -M

OBJS = $(patsubst %.c,%.o, $(SRCS))
SHOBJS = $(patsubst %.c,%.so, $(SRCS))
DEPS= ${OBJS:%.o=%.d}

BUILDDEPS = Makefile config.mak

ifeq ($(BUILD_SHARED),yes)
all:	$(SHLIB) $(MINI_SHLIB) dvdnav-config pkgconfig
install: $(SHLIB) install-shared install-dvdnav-config install-pkgconfig
endif

ifeq ($(BUILD_STATIC),yes)
all:	$(LIB) dvdnav-config pkgconfig
install: $(LIB) install-static install-dvdnav-config install-pkgconfig
endif

install: install-headers

# Let version.sh create version.h

SVN_ENTRIES = $(SRC_PATH_BARE)/.svn/entries
ifeq ($(wildcard $(SVN_ENTRIES)),$(SVN_ENTRIES))
version.h: $(SVN_ENTRIES)
endif

version.h:
	sh $(SRC_PATH)/version.sh $(SRC_PATH) "$(SHLIB_VERSION)"
	
$(SRCS): version.h


# General targets

${LIB}: version.h $(OBJS) $(BUILDDEPS)
	cd $(.OBJDIR) && $(AR) rc $@ $(OBJS)
	cd $(.OBJDIR) && $(RANLIB) $@

${SHLIB}: version.h $(SHOBJS) $(BUILDDEPS)
	cd $(.OBJDIR) && $(CC) $(SHLDFLAGS) -Wl,-soname=$(SHLIB).$(SHLIB_MAJOR) -o $@ $(SHOBJS) $(DVDREAD_LIBS) $(THREADLIB)
${MINI_SHLIB}: version.h $(.OBJDIR) $(SHOBJS) $(BUILDDEPS)
	cd $(.OBJDIR) && $(CC) $(SHLDFLAGS) -Wl,-soname=$(MINI_SHLIB).$(SHLIB_MAJOR) -o $@ $(SHOBJS) $(THREADLIB)

.c.so:	$(BUILDDEPS)
	cd $(.OBJDIR) && $(CC) -fPIC -DPIC -MD $(CFLAGS) -c -o $@ $<

.c.o:	$(BUILDDEPS)
	cd $(.OBJDIR) && $(CC) -MD $(CFLAGS) -c -o $@ $<


# Install targets

install-headers:
	install -d $(DESTDIR)$(incdir)
	install -m 644 $(HEADERS) $(DESTDIR)$(incdir)

install-shared: $(SHLIB)
	install -d $(DESTDIR)$(shlibdir)

	install $(INSTALLSTRIP) -m 755 $(.OBJDIR)/$(SHLIB) \
		$(DESTDIR)$(shlibdir)/$(SHLIB).$(SHLIB_VERSION)
	install $(INSTALLSTRIP) -m 755 $(.OBJDIR)/$(MINI_SHLIB) \
		$(DESTDIR)$(shlibdir)/$(MINI_SHLIB).$(SHLIB_VERSION)

	cd $(DESTDIR)$(shlibdir) && \
		ln -sf $(SHLIB).$(SHLIB_VERSION) $(SHLIB).$(SHLIB_MAJOR)
	cd $(DESTDIR)$(shlibdir) && \
		ln -sf $(MINI_SHLIB).$(SHLIB_VERSION) $(MINI_SHLIB).$(SHLIB_MAJOR)
	cd $(DESTDIR)$(shlibdir) && \
		ln -sf $(SHLIB).$(SHLIB_MAJOR) $(SHLIB)
	cd $(DESTDIR)$(shlibdir) && \
		ln -sf $(MINI_SHLIB).$(SHLIB_MAJOR) $(MINI_SHLIB)


install-static: $(LIB)
	install -d $(DESTDIR)$(libdir)

	install $(INSTALLSTRIP) -m 755 $(.OBJDIR)/$(LIB) $(DESTDIR)$(libdir)/$(LIB)


# Clean targets

clean:
	rm -rf  *~ $(.OBJDIR)/* 

pcedit = sed \
	-e 's,@prefix@,$(PREFIX),' \
	-e 's,@exec_prefix@,$(PREFIX),' \
	-e 's,@libdir@,$(shlibdir),' \
	-e 's,@includedir@,$(PREFIX)/include,' \
	-e 's,@VERSION@,$(SHLIB_VERSION),' \
	-e 's,@THREAD_CFLAGS@,,' \
	-e 's,@THREAD_LIBS@,$(THREADLIB),' \
	-e 's,@DVDREAD_CFLAGS@,$(DVDREAD_CFLAGS),'

pkgconfig: $(.OBJDIR)/dvdnav.pc $(.OBJDIR)/dvdnavmini.pc
$(.OBJDIR)/dvdnav.pc: misc/dvdnav.pc.in $(BUILDDEPS)
	$(pcedit) $< > $@
$(.OBJDIR)/dvdnavmini.pc: misc/dvdnavmini.pc.in $(BUILDDEPS)
	$(pcedit) $< > $@

install-pkgconfig: $(.OBJDIR)/dvdnav.pc $(.OBJDIR)/dvdnavmini.pc
	install -d $(DESTDIR)$(libdir)/pkgconfig
	install -m 0644 $(.OBJDIR)/dvdnav.pc $(DESTDIR)$(libdir)/pkgconfig
	install -m 0644 $(.OBJDIR)/dvdnavmini.pc \
	  $(DESTDIR)$(libdir)/pkgconfig

distclean: clean
	find . -name "*~" | xargs rm -rf
	rm -rf config.mak $(.OBJDIR)

dvdnav-config: $(.OBJDIR)/dvdnav-config
$(.OBJDIR)/dvdnav-config: $(BUILDDEPS)
	@echo '#!/bin/sh' > $(.OBJDIR)/dvdnav-config
	@echo 'prefix='$(PREFIX) >> $(.OBJDIR)/dvdnav-config
	@echo 'libdir='$(shlibdir) >> $(.OBJDIR)/dvdnav-config
	@echo 'version='$(SHLIB_VERSION) >> $(.OBJDIR)/dvdnav-config
	@echo 'dvdread_cflags="'$(DVDREAD_CFLAGS)'"' >> $(.OBJDIR)/dvdnav-config
	@echo 'dvdread_libs="'$(DVDREAD_LIBS)'"' >> $(.OBJDIR)/dvdnav-config
	@echo 'threadlib='$(THREADLIB) >> $(.OBJDIR)/dvdnav-config
	@echo >> $(.OBJDIR)/dvdnav-config
	cat $(SRC_PATH_BARE)/misc/dvdnav-config2.sh >> $(.OBJDIR)/dvdnav-config
	chmod 0755 $(.OBJDIR)/dvdnav-config

install-dvdnav-config: dvdnav-config
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 $(.OBJDIR)/dvdnav-config $(DESTDIR)$(PREFIX)/bin/dvdnav-config

vpath %.so ${.OBJDIR}
vpath %.o ${.OBJDIR}
vpath ${LIB} ${.OBJDIR}

# include dependency files if they exist
$(addprefix ${.OBJDIR}/, ${DEPS}): ;
-include $(addprefix ${.OBJDIR}/, ${DEPS})
