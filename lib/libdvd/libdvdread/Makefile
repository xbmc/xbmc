include config.mak

.SUFFIXES: .so

AR=ar
LD=ld
RANLIB=ranlib

VPATH+= $(SRC_PATH_BARE)/src

CFLAGS += $(USEDEBUG) -Wall -funsigned-char
CFLAGS += -I$(CURDIR) -I$(SRC_PATH)/src
CFLAGS += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE
CFLAGS += -DHAVE_CONFIG_H -DHAVE_DLFCN_H

L=libdvdread
DVDREAD_L=libdvdread
DVDREAD_LIB = $(DVDREAD_L).a
DVDREAD_SHLIB = $(DVDREAD_L).so
VPATH+= $(SRC_PATH_BARE)/src
DVDREAD_HEADERS = src/dvd_reader.h \
	src/ifo_print.h \
	src/ifo_read.h \
	src/ifo_types.h \
	src/nav_print.h \
	src/nav_read.h \
	src/dvd_udf.h \
	src/nav_types.h \
	src/bitreader.h
DVDREAD_SRCS = dvd_input.c dvd_reader.c dvd_udf.c ifo_print.c ifo_read.c \
	md5.c nav_print.c nav_read.c bitreader.c
CFLAGS += -I$(SRC_PATH)/src

LIB = $(L).a
SHLIB = $(L).so

.OBJDIR=        obj
DEPFLAG = -M

OBJS = $(patsubst %.c,%.o, $(SRCS))
DVDREAD_OBJS = $(patsubst %.c,%.o, $(DVDREAD_SRCS))
SHOBJS = $(patsubst %.c,%.so, $(SRCS))
DVDREAD_SHOBJS = $(patsubst %.c,%.so, $(DVDREAD_SRCS))
DEPS= ${OBJS:%.o=%.d}
DVDREAD_DEPS= ${DVDREAD_OBJS:%.o=%.d}

BUILDDEPS = Makefile config.mak

ifeq ($(BUILD_SHARED),yes)
all:	$(SHLIB) $(DVDREAD_SHLIB) dvdread-config pkgconfig
install: $(SHLIB) $(DVDREAD_SHLIB) install-shared install-dvdread-config install-pkgconfig
endif

ifeq ($(BUILD_STATIC),yes)
all:	$(LIB) $(DVDREAD_LIB) dvdread-config pkgconfig
install: $(LIB) $(DVDREAD_LIB) install-static install-dvdread-config install-pkgconfig
endif

install: install-headers

# Let version.sh create version.h

SVN_ENTRIES = $(SRC_PATH_BARE)/.svn/entries
ifeq ($(wildcard $(SVN_ENTRIES)),$(SVN_ENTRIES))
version.h: $(SVN_ENTRIES)
endif

version.h:
	sh $(SRC_PATH)/version.sh $(SRC_PATH) "$(SHLIB_VERSION)"
	
$(SRCS) $(DVDREAD_SRCS): version.h


# General targets

${DVDREAD_LIB}: version.h $(DVDREAD_OBJS) $(BUILDDEPS)
	cd $(.OBJDIR) && $(AR) rc $@ $(DVDREAD_OBJS)
	cd $(.OBJDIR) && $(RANLIB) $@

${DVDREAD_SHLIB}: version.h $(DVDREAD_SHOBJS) $(BUILDDEPS)
	cd $(.OBJDIR) && $(CC) $(SHLDFLAGS) $(LDFLAGS) -ldl -Wl,-soname=$(DVDREAD_SHLIB).$(SHLIB_MAJOR) -o $@ $(DVDREAD_SHOBJS)

.c.so:	$(BUILDDEPS)
	cd $(.OBJDIR) && $(CC) -fPIC -DPIC -MD $(CFLAGS) -c -o $@ $<

.c.o:	$(BUILDDEPS)
	cd $(.OBJDIR) && $(CC) -MD $(CFLAGS) -c -o $@ $<


# Install targets

install-headers:
	install -d $(DESTDIR)$(dvdread_incdir)
	install -m 644 $(DVDREAD_HEADERS) $(DESTDIR)$(dvdread_incdir)

install-shared: $(SHLIB)
	install -d $(DESTDIR)$(shlibdir)

	install $(INSTALLSTRIP) -m 755 $(.OBJDIR)/$(SHLIB) \
		$(DESTDIR)$(shlibdir)/$(SHLIB).$(SHLIB_VERSION)

	cd $(DESTDIR)$(shlibdir) && \
		ln -sf $(SHLIB).$(SHLIB_VERSION) $(SHLIB).$(SHLIB_MAJOR)
	cd $(DESTDIR)$(shlibdir) && \
		ln -sf $(SHLIB).$(SHLIB_MAJOR) $(SHLIB)

	install $(INSTALLSTRIP) -m 755 $(.OBJDIR)/$(DVDREAD_SHLIB) \
		$(DESTDIR)$(shlibdir)/$(DVDREAD_SHLIB).$(SHLIB_VERSION)
	cd $(DESTDIR)$(shlibdir) && \
		ln -sf $(DVDREAD_SHLIB).$(SHLIB_VERSION) $(DVDREAD_SHLIB).$(SHLIB_MAJOR)
	cd $(DESTDIR)$(shlibdir) && \
		ln -sf $(DVDREAD_SHLIB).$(SHLIB_MAJOR) $(DVDREAD_SHLIB)

install-static: $(LIB)
	install -d $(DESTDIR)$(libdir)

	install $(INSTALLSTRIP) -m 755 $(.OBJDIR)/$(LIB) $(DESTDIR)$(libdir)/$(LIB)
	install $(INSTALLSTRIP) -m 755 $(.OBJDIR)/$(DVDREAD_LIB) $(DESTDIR)$(libdir)/$(DVDREAD_LIB)


# Clean targets

clean:
	rm -rf  *~ $(.OBJDIR)/* 


distclean: clean
	find . -name "*~" | xargs rm -rf
	rm -rf config.mak $(.OBJDIR)

dvdread-config: $(.OBJDIR)/dvdread-config
$(.OBJDIR)/dvdread-config: $(BUILDDEPS)
	@echo '#!/bin/sh' > $(.OBJDIR)/dvdread-config
	@echo 'prefix='$(PREFIX) >> $(.OBJDIR)/dvdread-config
	@echo 'libdir='$(shlibdir) >> $(.OBJDIR)/dvdread-config
	@echo 'version='$(SHLIB_VERSION) >> $(.OBJDIR)/dvdread-config
	@echo >> $(.OBJDIR)/dvdread-config
	cat $(SRC_PATH_BARE)/misc/dvdread-config.sh >> $(.OBJDIR)/dvdread-config
	chmod 0755 $(.OBJDIR)/dvdread-config

install-dvdread-config: dvdread-config
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 $(.OBJDIR)/dvdread-config $(DESTDIR)$(PREFIX)/bin/dvdread-config

pcedit = sed \
	-e 's,@prefix@,$(PREFIX),' \
	-e 's,@exec_prefix@,$(PREFIX),' \
	-e 's,@libdir@,$(shlibdir),' \
	-e 's,@includedir@,$(PREFIX)/include,' \
	-e 's,@VERSION@,$(SHLIB_VERSION),'

pkgconfig: $(.OBJDIR)/dvdread.pc
$(.OBJDIR)/dvdread.pc: misc/dvdread.pc.in $(BUILDDEPS)
	$(pcedit) $< > $@

install-pkgconfig: $(.OBJDIR)/dvdread.pc
	install -d $(DESTDIR)$(libdir)/pkgconfig
	install -m 0644 $(.OBJDIR)/dvdread.pc $(DESTDIR)$(libdir)/pkgconfig

vpath %.so ${.OBJDIR}
vpath %.o ${.OBJDIR}
vpath ${LIB} ${.OBJDIR}

# include dependency files if they exist
$(addprefix ${.OBJDIR}/, ${DEPS}): ;
-include $(addprefix ${.OBJDIR}/, ${DEPS})
