# TiMidity++ -- MIDI to WAVE converter and player
# Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
# Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

include $(top_srcdir)/common.makefile.in

INCLUDES = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libarc \
	-I$(top_srcdir)/libunimod \
	-I$(top_srcdir)/interface \
	-I$(top_srcdir)/utils \
	$(EXTRAINCS)

bin_PROGRAMS = timidity

noinst_PROGRAMS = calcnewt

timidity_SOURCES = \
	aenc.h \
	aiff_a.c \
	aq.c \
	aq.h \
	au_a.c \
	audio_cnv.c \
	audio_cnv.h \
	common.c \
	common.h \
	controls.c \
	controls.h \
	dlutils.h \
	effect.c \
	filter.c \
	filter.h \
	freq.c \
	freq.h \
	instrum.c \
	instrum.h \
	list_a.c \
	loadtab.c \
	m2m.c \
	mfi.c \
	mid-j.defs \
	mid.defs \
	miditrace.c \
	miditrace.h \
	mix.c \
	mix.h \
	mod.c \
	mod.h \
	mod2midi.c \
	mod2midi.h \
	modmid_a.c \
	mt19937ar.c \
	mt19937ar.h \
	optcode.c \
	optcode.h \
	output.c \
	output.h \
	playmidi.c \
	playmidi.h \
	quantity.c \
	quantity.h \
	raw_a.c \
	rcp.c \
	readmidi.c \
	readmidi.h \
	recache.c \
	recache.h \
	resample.c \
	resample.h \
	reverb.c \
	reverb.h \
	sbkconv.c \
	sffile.c \
	sffile.h \
	sfitem.c \
	sfitem.h \
	sflayer.h \
	smfconv.c \
	smfconv.h \
	smplfile.c \
	sndfont.c \
	sysdep.h \
	tables.c \
	tables.h \
	timidity.c \
	timidity.h \
	version.c \
	wave_a.c \
	wrd.h \
	wrd_read.c \
	wrdt.c

EXTRA_timidity_SOURCES = \
	alsa_a.c \
	ao_a.c \
	aRts_a.c \
	audriv.h \
	audriv_a.c \
	audriv_al.c \
	audriv_mme.c \
	audriv_none.c \
	darwin_a.c \
	dl_w32.c \
	dl_dld.c \
	dl_dlopen.c \
	dl_dyld.c \
	dl_hpux.c \
	bsd20_a.c \
	esd_a.c \
	hpux_a.c \
	hpux_d_a.c \
	jack_a.c \
	oss_a.c \
	mac_a.c \
	mac_qt_a.c \
	mac_com.h \
	mac_dlog.c \
	mac_main.c \
	mac_main.h \
	mac_soundspec.c \
	mfnode.c \
	mfnode.h \
	nas_a.c \
	portaudio_a.c \
	sun_a.c \
	vorbis_a.c \
	flac_a.c \
	speex_a.c \
	w32_a.c \
	w32_gogo.c \
	w32_gogo.h \
	gogo_a.h \
	gogo_a.c \
	w32g_vorbisenc_dll.c \
	w32g_vorbis_dll.c \
	w32g_ogg_dll.c \
	w32_portaudio.h \
	w32_portaudio_dll.c \
	w32_libFLAC_dll.c \
	w32_libFLAC_dll.h \
	w32_libFLAC_dll_g.h \
	w32_libFLAC_dll_i.h \
	w32_libOggFLAC_dll.c \
	w32_libOggFLAC_dll.h \
	w32_libOggFLAC_dll_g.h \
	w32_libOggFLAC_dll_i.h

calcnewt_SOURCES = calcnewt.c

if BORLANDC

if ENABLE_W32GUI
W32GUI_RES = w32g_res.res
else
if ENABLE_W32G_SYN
W32GUI_RES = w32g_res.res
endif
endif

else

CLEANFILES = newton_table.c

if ENABLE_W32GUI
W32GUI_RES = $(top_builddir)/interface/w32g_res.res
else
if ENABLE_W32G_SYN
W32GUI_RES = $(top_builddir)/interface/w32g_res.res
endif
endif

endif

if VCPP

timidity_LDADD = \
	$(SYSEXTRAS:.c=.$(OBJEXT)) \
	$(top_builddir)/interface/interface.lib \
	$(top_builddir)/libarc/arc.lib \
	$(top_builddir)/libunimod/unimod.lib \
	$(top_builddir)/utils/utils.lib \
	$(W32GUI_RES)

timidity_dep = $(SYSEXTRAS:.c=.$(OBJEXT)) \
	$(top_builddir)/interface/interface.lib \
	$(top_builddir)/libarc/arc.lib \
	$(top_builddir)/libunimod/unimod.lib \
	$(top_builddir)/utils/utils.lib \
	$(W32GUI_RES)

else

if BORLANDC
timidity_LDADD = \
      interface.lib \
      arc.lib \
      unimod.lib \
      utils.lib
BORLANDC_LINK_OPTIONS = \
      -L..\\interface \
      -L..\\libarc \
      -L..\\libunimod \
      -L..\\utils
timidity_dep = $(SYSEXTRAS:.c=.$(OBJEXT)) \
	$(top_builddir)/interface/interface.lib \
	$(top_builddir)/libarc/arc.lib \
	$(top_builddir)/libunimod/unimod.lib \
	$(top_builddir)/utils/utils.lib \
	$(W32GUI_RES)
else
if WATCOM_C
timidity_LDADD = \
      interface.lib \
      arc.lib \
      unimod.lib \
      utils.lib
WATCOM_LINK_OPTIONS = \
      LIBPath ..\\interface \
      LibPath ..\\libarc \
      LibPath ..\\libunimod \
      LibPath ..\\utils
timidity_dep = $(SYSEXTRAS:.c=.$(OBJEXT)) \
	$(top_builddir)/interface/interface.lib \
	$(top_builddir)/libarc/arc.lib \
	$(top_builddir)/libunimod/unimod.lib \
	$(top_builddir)/utils/utils.lib \
	$(W32GUI_RES)
else
timidity_LDADD = \
	$(SYSEXTRAS:.c=.$(OBJEXT)) \
	$(top_builddir)/interface/libinterface.a \
	$(top_builddir)/libarc/libarc.a \
	$(top_builddir)/libunimod/libunimod.a \
	$(top_builddir)/utils/libutils.a \
	$(W32GUI_RES)

timidity_dep = $(SYSEXTRAS:.c=.$(OBJEXT)) \
	$(top_builddir)/interface/libinterface.a \
	$(top_builddir)/libarc/libarc.a \
	$(top_builddir)/libunimod/libunimod.a \
	$(top_builddir)/utils/libutils.a \
	$(W32GUI_RES)
endif
endif
endif

if VCPP

VC_LINK = link.exe
VCC_LINK = $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS)

timidity$(EXEEXT): $(timidity_OBJECTS) $(timidity_DEPENDENCIES) $(timidity_dep) $(W32GUI_RES)
	@rm -f timidity$(EXEEXT)
	$(VC_LINK) $(VCPP_LDFLAGS) $(timidity_OBJECTS) $(timidity_LDADD) $(LIBS) -out:timidity.exe

calcnewt$(EXEEXT): $(calcnewt_OBJECTS) $(calcnewt_DEPENDENCIES)
	@rm -f calcnewt$(EXEEXT)
	$(VCC_LINK) $(calcnewt_LDFLAGS) $(calcnewt_OBJECTS) $(calcnewt_LDADD) $(LIBS)

else

if BORLANDC
BORLANDC_LINK = ilink32.exe -C -Gn

timidity$(EXEEXT): $(timidity_OBJECTS) $(SYSEXTRAS:.c=.$(OBJEXT)) $(timidity_dep) $(W32GUI_RES)
	@rm -f timidity$(EXEEXT)
	$(BORLANDC_LINK) $(BORLANDC_LINK_OPTIONS) $(BORLANDC_START) $(timidity_OBJECTS) $(SYSEXTRAS:.c=.$(OBJEXT)), $@, ,$(timidity_LDADD) $(BORLANDC_LDFLAGS) $(LIBS), ,$(W32GUI_RES)
calcnewt.$(OBJEXT): calcnewt.c
	$(CC)  -tWC  -c calcnewt.c
calcnewt$(EXEEXT): $(calcnewt_OBJECTS) $(calcnewt_DEPENDENCIES)
	@rm -f calcnewt$(EXEEXT)
	$(CCLD) $(calcnewt_LDFLAGS) $(calcnewt_OBJECTS) $(calcnewt_LDADD) $(LIBS)

else

if WATCOM_C
WATCOM_LINK = wlink.exe 
timidity$(EXEEXT): $(timidity_OBJECTS) $(SYSEXTRAS:.c=.$(OBJEXT)) $(timidity_dep) $(W32GUI_RES)
	@rm -f timidity$(EXEEXT)
	$(WATCOM_LINK) $(WATCOM_LDFLAGS) `for foo in $(timidity_OBJECTS) $(SYSEXTRAS:.c=.$(OBJEXT)); do echo "File $$foo "; done;` Name $@ `for foo in  $(timidity_LDADD) $(LIBS); do echo "Library $$foo"; done;` $(WATCOM_LINK_OPTIONS)  `if test x$(W32GUI_RES) != x ; then  echo Option RESource=$(W32GUI_RES); fi;`
calcnewt.$(OBJEXT): calcnewt.c
	$(CCLD)  -c calcnewt.c
calcnewt$(EXEEXT): $(calcnewt_OBJECTS) $(calcnewt_DEPENDENCIES)
	@rm -f calcnewt$(EXEEXT)
	$(WATCOM_LINK) File calcnewt.$(OBJEXT) Name $@
endif

endif

endif

version.$(OBJEXT): version.c ../configure
	$(CC) $(DEFS) $(DEFAULT_INCLUDES) $(CFLAGS) -c $(srcdir)/version.c

resample.c: newton_table.c

if VCPP
newton_table.c: calcnewt$(EXEEXT)
	./calcnewt $@

else
if WATCOM_C
newton_table.c: calcnewt$(EXEEXT)
	./calcnewt > $@

else
newton_table.c: calcnewt$(EXEEXT)
	./calcnewt > $@

endif
endif

$(top_builddir)/interface/libinterface.a $(W32GUI_RES):
	cd $(top_builddir)/interface; $(MAKE)

$(top_builddir)/libarc/libarc.a:
	cd $(top_builddir)/libarc; $(MAKE)

$(top_builddir)/libunimod/libunimod.a:
	cd $(top_builddir)/libunimod; $(MAKE)

$(top_builddir)/utils/libutils.a:
	cd $(top_builddir)/utils; $(MAKE)

objs: $(timidity_OBJECTS) $(timidity_DEPENDENCIES)
	@:
