#
# PortAudio V19 Makefile.in
#
# Dominic Mazzoni
# Modifications by Mikael Magnusson
#

top_srcdir = .
srcdir = .

top_builddir = .
PREFIX = /usr/local
prefix = $(PREFIX)
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin
libdir = ${exec_prefix}/lib
includedir = ${prefix}/include
CC = gcc
CFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src/common -I$(top_srcdir)/src/os/unix -Werror -g -O0 -DPA_LITTLE_ENDIAN -arch x86_64 -arch ppc64 -arch i386 -arch ppc -isysroot /Developer/SDKs/MacOSX10.5.sdk -mmacosx-version-min=10.3  -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_DLFCN_H=1 -DSIZEOF_SHORT=2 -DSIZEOF_INT=4 -DSIZEOF_LONG=4 -DHAVE_NANOSLEEP=1 -DPA_USE_COREAUDIO=1 
LIBS = -framework CoreAudio -framework AudioToolbox -framework AudioUnit -framework Carbon
AR = /usr/bin/ar
RANLIB = ranlib
LIBTOOL = $(SHELL) $(top_builddir)/libtool
INSTALL = /usr/bin/install -c
INSTALL_DATA = ${INSTALL} -m 644
SHARED_FLAGS = -Werror -framework CoreAudio -framework AudioToolbox -framework AudioUnit -framework Carbon -dynamiclib -arch x86_64 -arch ppc64 -arch i386 -arch ppc -isysroot /Developer/SDKs/MacOSX10.5.sdk -mmacosx-version-min=10.3
LDFLAGS = 
DLL_LIBS = 
CXXFLAGS = -g -O0
NASM = 
NASMOPT = 
LN_S = ln -s
LT_RELEASE=@LT_RELEASE@
LT_CURRENT=2
LT_REVISION=0
LT_AGE=0

OTHER_OBJS = src/os/mac_osx/pa_mac_hostapis.o src/os/unix/pa_unix_util.o src/hostapi/coreaudio/pa_mac_core.o src/hostapi/coreaudio/pa_mac_core_utilities.o src/hostapi/coreaudio/pa_mac_core_blocking.o src/common/pa_ringbuffer.o

PALIB = libportaudio.la
PAINC = include/portaudio.h

PA_LDFLAGS = $(LDFLAGS) $(SHARED_FLAGS) -rpath $(libdir) -no-undefined -export-symbols-regex "(Pa|PaMacCore)_.*" -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
#MAKEFILE = Makefile

COMMON_OBJS = \
	src/common/pa_allocation.o \
	src/common/pa_converters.o \
	src/common/pa_cpuload.o \
	src/common/pa_dither.o \
	src/common/pa_debugprint.o \
	src/common/pa_front.o \
	src/common/pa_process.o \
	src/common/pa_skeleton.o \
	src/common/pa_stream.o \
	src/common/pa_trace.o

TESTS = \
	bin/paqa_devs \
	bin/paqa_errs \
	bin/patest1 \
	bin/patest_buffer \
	bin/patest_callbackstop \
	bin/patest_clip \
	bin/patest_dither \
	bin/patest_hang \
	bin/patest_in_overflow \
	bin/patest_latency \
	bin/patest_leftright \
	bin/patest_longsine \
	bin/patest_many \
	bin/patest_maxsines \
	bin/patest_multi_sine \
	bin/patest_out_underflow \
	bin/patest_pink \
	bin/patest_prime \
	bin/patest_read_record \
	bin/patest_read_write_wire \
	bin/patest_record \
	bin/patest_ringmix \
	bin/patest_saw \
	bin/patest_sine8 \
	bin/patest_sine \
	bin/patest_sine_channelmaps \
	bin/patest_sine_formats \
	bin/patest_sine_time \
	bin/patest_start_stop \
	bin/patest_stop \
	bin/patest_stop_playout \
	bin/patest_toomanysines \
	bin/patest_underflow \
	bin/patest_wire \
	bin/patest_write_sine \
	bin/pa_devs \
	bin/pa_fuzz \
	bin/pa_minlat

# Most of these don't compile yet.  Put them in TESTS, above, if
# you want to try to compile them...
ALL_TESTS = \
	$(TESTS) \
	bin/patest_sync \
	bin/debug_convert \
	bin/debug_dither_calc \
	bin/debug_dual \
	bin/debug_multi_in \
	bin/debug_multi_out \
	bin/debug_record \
	bin/debug_record_reuse \
	bin/debug_sine_amp \
	bin/debug_sine \
	bin/debug_sine_formats \
	bin/debug_srate \
	bin/debug_test1

OBJS = $(COMMON_OBJS) $(OTHER_OBJS)

LTOBJS:= $(OBJS:.o=.lo)

SUBDIRS =
#SUBDIRS += bindings/cpp

all: lib/$(PALIB) all-recursive tests

tests: bin-stamp $(TESTS)


lib/$(PALIB): lib-stamp $(LTOBJS) $(MAKEFILE) $(PAINC)
	$(LIBTOOL) --mode=link $(CC) $(PA_LDFLAGS) -o lib/$(PALIB) $(LTOBJS) $(DLL_LIBS)

$(ALL_TESTS): bin/%: lib/$(PALIB) $(MAKEFILE) $(PAINC) test/%.c
	$(LIBTOOL) --mode=link $(CC) -o $@ $(CFLAGS) $(top_srcdir)/test/$*.c lib/$(PALIB) $(LIBS)


install: lib/$(PALIB) portaudio-2.0.pc
	$(INSTALL) -d $(DESTDIR)$(libdir)
	$(LIBTOOL) --mode=install $(INSTALL) lib/$(PALIB) $(DESTDIR)$(libdir)
	$(INSTALL) -d $(DESTDIR)$(includedir)
	$(INSTALL_DATA) -m 644 $(top_srcdir)/$(PAINC) $(DESTDIR)$(includedir)/portaudio.h
	$(INSTALL) -d $(DESTDIR)$(libdir)/pkgconfig
	$(INSTALL) -m 644 portaudio-2.0.pc $(DESTDIR)$(libdir)/pkgconfig/portaudio-2.0.pc
	@echo ""
	@echo "------------------------------------------------------------"
	@echo "PortAudio was successfully installed."
	@echo ""
	@echo "On some systems (e.g. Linux) you should run 'ldconfig' now"
	@echo "to make the shared object available.  You may also need to"
	@echo "modify your LD_LIBRARY_PATH environment variable to include"
	@echo "the directory $(libdir)"
	@echo "------------------------------------------------------------"
	@echo ""
	$(MAKE) install-recursive

uninstall:
	$(LIBTOOL) --mode=uninstall rm -f $(DESTDIR)$(libdir)/$(PALIB)
	$(LIBTOOL) --mode=uninstall rm -f $(DESTDIR)$(includedir)/portaudio.h
	$(MAKE) uninstall-recursive

clean:
	$(LIBTOOL) --mode=clean rm -f $(LTOBJS) $(ALL_TESTS) lib/$(PALIB)
	rm -f bin-stamp lib-stamp
	-rm -rf bin lib

distclean: clean
	rm -f config.log config.status Makefile libtool portaudio-2.0.pc

%.o: %.c $(MAKEFILE) $(PAINC)
	$(CC) -c $(CFLAGS) $< -o $@

%.lo: %.c $(MAKEFILE) $(PAINC)
	$(LIBTOOL) --mode=compile $(CC) -c $(CFLAGS) $< -o $@

%.o: %.cpp $(MAKEFILE) $(PAINC)
	$(CXX) -c $(CXXFLAGS) $< -o $@

%.o: %.asm
	$(NASM) $(NASMOPT) -o $@ $<

bin-stamp:
	-mkdir bin
	touch $@

lib-stamp:
	-mkdir lib
	-mkdir -p \
		src/common \
		src/hostapi/alsa \
		src/hostapi/asihpi \
		src/hostapi/asio \
		src/hostapi/coreaudio \
		src/hostapi/dsound \
		src/hostapi/jack \
		src/hostapi/oss \
		src/hostapi/wasapi \
		src/hostapi/wdmks \
		src/hostapi/wmme \
		src/os/mac_osx \
		src/os/unix \
		src/os/win
	touch $@

Makefile: Makefile.in config.status
	$(SHELL) config.status

all-recursive:
	for dir in $(SUBDIRS); do make -C $$dir all; done

install-recursive:
	for dir in $(SUBDIRS); do make -C $$dir install; done

uninstall-recursive:
	for dir in $(SUBDIRS); do make -C $$dir uninstall; done
