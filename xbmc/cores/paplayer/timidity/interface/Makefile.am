# TiMidity++ -- MIDI to WAVE converter and player
# Copyright (C) 1999-2004 Masanao Izumo <iz@onicos.co.jp>
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
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

include $(top_srcdir)/common.makefile.in

SUFFIXES = .ptcl .tcl .@so@

SUBDIRS = motif_bitmaps bitmaps pixmaps

INCLUDES = \
	-I$(top_srcdir)/timidity \
	-I$(top_srcdir)/libarc \
	-I$(top_srcdir)/utils

noinst_LIBRARIES = libinterface.a

libinterface_a_SOURCES = \
	dumb_c.c \
	wrdt_dumb.c \
	wrdt_tty.c

EXTRA_libinterface_a_SOURCES = \
	ncurs_c.c \
	slang_c.c \
	motif.h \
	motif_c.c \
	motif_i.c \
	motif_p.c \
	tk_c.c \
	emacs_c.c \
	vt100.h \
	vt100.c \
	vt100_c.c \
	xaw.h \
	xaw_c.c \
	xaw_i.c \
	xaw_redef.c \
	xskin.h \
	xskin_c.c \
	xskin_i.c \
	xskin_loadBMP.c \
	xskin_spectrum.c \
	wrdt_x.c \
	x_sherry.c \
	x_sherry.h \
	x_wrdwindow.c \
	x_wrdwindow.h \
	VTPrsTbl.c \
	VTparse.h \
	x_mag.c \
	x_mag.h \
	dynamic_c.c \
	mac_c.c \
	mac_c.h \
	mac_mag.c \
	mac_mag.h \
	mac_trace.c \
	mac_wrdwindow.c \
	mac_wrdwindow.h \
	wrdt_mac.c \
	wrdt_wcon.c \
	gtk_c.c \
	gtk_h.h \
	gtk_i.c \
	gtk_p.c \
	soundspec.c \
	soundspec.h \
	x_sherry.c \
	x_sherry.h \
	check.xbm \
	arrow.xbm \
	on.xbm \
	off.xbm \
	server_c.c \
	server_defs.h \
	alsaseq_c.c \
	w32g.h \
	w32g_c.c \
	w32g_ini.c \
	w32g_rec.h \
	w32g_res.h \
	w32g_res.rc \
	w32g_utl.c \
	w32g_utl.h \
	w32g_ut2.c \
	w32g_ut2.h \
	w32g_i.c \
	w32g_playlist.c \
	w32g_pref.c \
	w32g_pref.h \
	w32g_subwin.c \
	w32g_subwin.h \
	w32g_syn.c \
	w32g_btn.bmp \
	w32g_icon.ico \
	w32g_sleep.bmp \
	w32g_subbtn.bmp \
	w32g_subwin2.c \
	w32g_wrd.h \
	wrdt_w32g.c \
	w32g_subwin3.c \
	w32g_tracer.h \
	w32g_tracer.bmp \
	w32g_mag.c \
	w32g_mag.h \
	w32g_dib.c \
	w32g_dib.h \
	winsyn_c.c \
	rtsyn.h \
	rtsyn_common.c \
	rtsyn_winmm.c \
	rtsyn_portmidi.c \
	portmidisyn_c.c

if ENABLE_WRD
WRD_OBJS = \
	wrdt_x.$(OBJEXT) \
	x_wrdwindow.$(OBJEXT) \
	VTPrsTbl.$(OBJEXT) \
	x_mag.$(OBJEXT) \
	x_sherry.$(OBJEXT)
endif

if CYGNUS
WRD_WINCON_OBJS = wrdt_wcon.$(OBJEXT)
endif
if MSYS
WRD_WINCON_OBJS2 = wrdt_wcon.$(OBJEXT)
endif

if NEEDDLOPEN
DYNAMIC_OBJS = dynamic_c.$(OBJEXT)
endif

if ENABLE_SOUND_SPEC
SOUND_SPEC_OBJS = \
	soundspec.$(OBJEXT)
endif

if ENABLE_W32GUI
W32GUI_DEPS = w32g_res.res
endif

if ENABLE_W32G_SYN
W32GUI_DEPS = w32g_res.res
endif

INTERFACE_OBJS = $(INTERFACE_SRCS:.c=.$(OBJEXT))

libinterface_LIBADD = \
	$(INTERFACE_OBJS) \
	$(WRD_OBJS) \
	$(WRD_WINCON_OBJS) \
	$(WRD_WINCON_OBJS2) \
	$(DYNAMIC_OBJS) \
	$(SOUND_SPEC_OBJS)
	
libinterface_a_LIBADD=$(libinterface_LIBADD)

libinterface_dep = \
	$(top_builddir)/interface.h \
	$(libinterface_a_LIBADD) \
	$(tcltk_dep) \
	$(W32GUI_DEPS)

libinterface.a: $(libinterface_a_OBJECTS) $(libinterface_a_DEPENDENCIES) $(libinterface_dep)
if VCPP
	rm -f libinterface.a
	rm -f interface.lib
	link -lib $(libinterface_a_OBJECTS) $(libinterface_LIBADD) -out:interface.lib
	if test -f interface.lib ; then touch $@ ; fi
else
if BORLANDC
	rm -f libinterface.a
	rm -f interface.lib
	for foo in   $(libinterface_a_OBJECTS) $(libinterface_LIBADD) ;do \
	  tlib interface.lib +$$foo; \
	done
	if test -f interface.lib ; then touch $@ ; fi
else
if WATCOM_C
	rm -f libinterface.a
	rm -f interface.lib
	for foo in   $(libinterface_a_OBJECTS) $(libinterface_LIBADD) ;do \
	  wlib -q interface.lib +$$foo; \
	done
	if test -f interface.lib ; then touch $@ ; fi
else
	rm -f libinterface.a
	$(AR) cru libinterface.a $(libinterface_a_OBJECTS) $(libinterface_a_LIBADD)
	$(RANLIB) libinterface.a
endif
endif
endif

# Tcl interface sources
TCLSRCS = tkmidity.ptcl tkpanel.tcl browser.tcl misc.tcl
# Tcl/Tk sources
TCLF = tkmidity.tcl tkpanel.tcl browser.tcl misc.tcl
ALLTCLF = $(TCLF) tclIndex

EXTRA_DIST = \
	escape_sjis.pl \
	tkmidity.ptcl \
	tkpanel.tcl \
	browser.tcl \
	misc.tcl \
	timidity.el \
	tclIndex \
	interface_A.txt \
	interface_T.txt \
	interface_a.txt \
	interface_e.txt \
	interface_g.txt \
	interface_i.txt \
	interface_k.txt \
	interface_m.txt \
	interface_n.txt \
	interface_s.txt

.ptcl.tcl:
	sed -e 's@%TCL_DIR%@$(TCL_DIR)@g' $< > $@
clean:
	rm -f tkmidity.tcl
	rm -f *.$(OBJEXT)
	rm -f *.$(so)
	rm -f *.res
	rm -f *.RES
	rm -f *escaped.c
	rm -f *.a
	rm -f *.lib

tclIndex: $(TCLSRCS)
	rm -f tclIndex
	echo 'auto_mkindex . *.tcl; exit' | $(WISH)

if ENABLE_TCLTK
install_tk = install.tk
endif
if ENABLE_DYNAMIC_TCLTK
install_tk = install.tk
endif

if ENABLE_XAW
install_xaw = install.xaw
endif
if ENABLE_DYNAMIC_XAW
install_xaw = install.xaw
endif

install.tk: $(ALLTCLF) install.bitmaps
	test -d $(DESTDIR)$(TCL_DIR) || mkdir -p $(DESTDIR)$(TCL_DIR)
	for f in $(ALLTCLF) ''; do case ".$$f" in .);; *) $(INSTALL_DATA) $$f $(DESTDIR)$(TCL_DIR);; esac; done

install.xaw: install.bitmaps

install.bitmaps:
	cd bitmaps; $(MAKE) install.bitmaps


if ENABLE_EMACS
# ELFILES = @ELFILES@ # autoconf already define this
install_el = install.el
endif

install.el: $(ELFILES)
	test -d $(DESTDIR)$(ELISP_DIR) || mkdir -p $(DESTDIR)$(ELISP_DIR)
	for f in $(ELFILES) ''; do case ".$$f" in .);; *) $(INSTALL_DATA) $$f $(DESTDIR)$(ELISP_DIR);; esac; done

if VCPP
w32g_res.res: w32g_res.h w32g_rec.h w32g_res.rc \
	w32g_btn.bmp w32g_icon.ico w32g_sleep.bmp w32g_subbtn.bmp 
	rc w32g_res.rc
else
if WATCOM_C
w32g_res.res: w32g_res.h w32g_rec.h w32g_res.rc \
	w32g_btn.bmp w32g_icon.ico w32g_sleep.bmp w32g_subbtn.bmp 
	rc w32g_res.rc
else
if BORLANDC
w32g_res.res: w32g_res.h w32g_rec.h w32g_res.rc \
	w32g_btn.bmp w32g_icon.ico w32g_sleep.bmp w32g_subbtn.bmp
	perl -pe 's/(^FONT.*".*").*$$/$$1/' w32g_res.rc >w32g_res_bc.rc
	brcc32 -ic:/borland/bcc55/include w32g_res_bc.rc
	mv w32g_res_bc.RES w32g_res.res
	rm w32g_res_bc.rc
else
w32g_syn_escaped.c:w32g_syn.c escape_sjis.pl
	escape_sjis.pl < w32g_syn.c > w32g_syn_escaped.c

w32g_subwin_escaped.c:w32g_subwin.c escape_sjis.pl
	escape_sjis.pl < w32g_subwin.c > w32g_subwin_escaped.c

w32g_res.res: w32g_res.h w32g_rec.h w32g_res.rc \
	w32g_btn.bmp w32g_icon.ico w32g_sleep.bmp w32g_subbtn.bmp \
	escape_sjis.pl
	escape_sjis.pl <w32g_res.rc >w32g_res_escaped.rc
	windres -o $@ -O coff w32g_res.rc 2>&1|grep escape - >/dev/null && \
	windres -o $@ -O coff w32g_res_escaped.rc || echo
	rm w32g_res_escaped.rc
endif
endif
endif

## This is another way to build japanese resource for mingw.
#BRCC32 = c:/borland/bcc55/bin/brcc32.exe
#BRCC32_FLAGS = -ic:/borland/bcc55/include
#w32g_res.res: w32g_res.h w32g_rec.h w32g_res.rc \
#	w32g_btn.bmp w32g_icon.ico w32g_sleep.bmp w32g_subbtn.bmp
#	perl -pe 's/(^FONT.*".*").*$$/$$1/' w32g_res.rc >w32g_res_bc.rc
#	$(BRCC32) $(BRCC32_FLAGS) w32g_res_bc.rc
#	windres -o tmp.res -O coff w32g_res_bc.RES
#	mv tmp.res w32g_res.res
#	rm w32g_res_bc.RES

if NEEDDLOPEN
# dynamic_targets = @dynamic_targets@ # autoconf already define this
install_dynamics = install.dynamics
#n_so_libs = @n_so_libs@
#s_so_libs = @s_so_libs@
#m_so_libs = @m_so_libs@
#k_so_libs = @k_so_libs@
#e_so_libs = @e_so_libs@
#T_so_libs = @T_so_libs@
#a_so_libs = @a_so_libs@
#i_so_libs = @i_so_libs@
#g_so_libs = @g_so_libs@

all-am: $(dynamic_targets)

install.dynamics: $(dynamic_targets)
	test -d $(DESTDIR)$(SHLIB_DIR) || mkdir -p $(DESTDIR)$(SHLIB_DIR)
	for f in $(dynamic_targets) ''; do case ".$$f" in .);; *) $(INSTALL_PROGRAM) $$f $(DESTDIR)$(SHLIB_DIR) ; $(INSTALL_DATA) $${f%%$(so)}txt $(DESTDIR)$(SHLIB_DIR);; esac; done

.c.$(so):
	$(COMPILE) $(SHCFLAGS) -c $< -o $@

bitset.$(so): $(top_srcdir)/utils/bitset.c
	$(COMPILE) $(SHCFLAGS) -o $@ -c $<

timer.$(so): $(top_srcdir)/utils/timer.c
	$(COMPILE) $(SHCFLAGS) -o $@ -c $<

url_buff.$(so): $(top_srcdir)/libarc/url_buff.c
	$(COMPILE) $(SHCFLAGS) -o $@ -c $<

support.$(so): $(top_srcdir)/utils/support.c
	$(COMPILE) $(SHCFLAGS) -o $@ -c $<

interface_n.$(so): ncurs_c.$(so) support.$(so) bitset.$(so) timer.$(so)
	$(SHLD) -o $@ ncurs_c.$(so) support.$(so) bitset.$(so) timer.$(so) $(n_so_libs)

interface_s.$(so): slang_c.$(so) support.$(so) timer.$(so)
	$(SHLD) -o $@ slang_c.$(so) support.$(so) timer.$(so) $(s_so_libs)

interface_m.$(so): motif_c.$(so) support.$(so) motif_i.$(so) motif_p.$(so)
	$(SHLD) -o $@ motif_c.$(so) support.$(so) motif_i.$(so) motif_p.$(so) $(m_so_libs)

interface_k.$(so): tk_c.$(so) support.$(so)
	$(SHLD) -o $@ tk_c.$(so) support.$(so) $(k_so_libs)

interface_e.$(so): emacs_c.$(so) support.$(so)
	$(SHLD) -o $@ emacs_c.$(so) support.$(so) $(e_so_libs)

interface_T.$(so): vt100_c.$(so) support.$(so) vt100.$(so) bitset.$(so) timer.$(so)
	$(SHLD) -o $@ vt100_c.$(so) support.$(so) vt100.$(so) bitset.$(so) timer.$(so) $(T_so_libs)

interface_a.$(so): xaw_c.$(so) support.$(so) xaw_i.$(so) timer.$(so)
	$(SHLD) -o $@ xaw_c.$(so) support.$(so) xaw_i.$(so) timer.$(so) $(a_so_libs)

interface_i.$(so): xskin_c.$(so) support.$(so) xskin_i.$(so) xskin_loadBMP.$(so) xskin_spectrum.$(so) url_buff.$(so)
	$(SHLD) -o $@ xskin_c.$(so) support.$(so) xskin_i.$(so) xskin_loadBMP.$(so) xskin_spectrum.$(so) url_buff.$(so) $(i_so_libs)

interface_g.$(so): gtk_c.$(so) support.$(so) gtk_i.$(so) gtk_p.$(so)
	$(SHLD) -o $@ gtk_c.$(so) support.$(so) gtk_i.$(so) gtk_p.$(so) $(g_so_libs)

interface_r.$(so): server_c.$(so) support.$(so)
	$(SHLD) -o $@ server_c.$(so) support.$(so)

interface_A.$(so): alsaseq_c.$(so) support.$(so)
	$(SHLD) -o $@ alsaseq_c.$(so) support.$(so)

endif

clean-compile:
	-rm -f *.$(so)
	-rm -f *.res
	-rm -f *.RES

install-exec-local: $(install_tk) $(install_xaw) $(install_el) $(install_dynamics)

########## End of Makefile
