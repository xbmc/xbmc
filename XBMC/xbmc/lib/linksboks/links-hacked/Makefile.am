# Makefile.am
# (c) 2002 Mikulas Patocka, Karel 'Clock' Kulhavy, Petr 'Brain' Kulhavy,
#          Martin 'PerM' Pergel

SUBDIRS=utils js docs

font:
	wget xray.sai.msu.ru/~karpov/links-hacked/downloads/links-fonts-new.tgz && tar xzvf links-fonts-new.tgz
        
font_include.c: font/ 
	./utils/generate_font	

arrow.inc: utils/arrow utils/arrow.png
	./utils/arrow

internal.c:	docs/included.inc

framebuffer.c:	arrow.inc
svgalib.c:      arrow.inc

man_MANS=links.1

EXTRA_DIST=atheos.cpp $(man_MANS) BUGS TODO SITES PATCH-gpm-1.20.0-smooth-cursor mailcap.pl rebuild Unicode/*.cp Unicode/[!C]* intl/[!C]* parser/[!Cy]* graphics/*.c graphics/arrow.png graphics/gen graphics/system_font/*.png graphics/font/century_school-medium-roman-serif-vari/*.png graphics/font/century_school-bold-roman-serif-vari/*.png graphics/font/symbol-medium-roman-sans-vari/*.png graphics/font/courier-medium-roman-serif-mono/*.png graphics/font/japanese-medium-roman-sans-mono/*.png graphics/font/century_school-bold-roman-serif-vari/aliases graphics/font/century_school-medium-roman-serif-vari/aliases graphics/font/symbol-medium-roman-sans-vari/aliases doc/links_cal/*.png doc/links_cal/*.html graphics/Makefile graphics/Fontmap graphics/clip.c graphics/links.xpm graphics/makefont graphics/pdf2html graphics/spacer.png

bin_PROGRAMS=links

if ATHEOS_GR
links_LDADD_ATHEOS=atheos.o
else
endif

links_SOURCES= \
	arrow.inc codepage.inc entity.inc language.inc uni_7b.inc internal.inc \
        af_unix.c auth.c beos.c bfu.c blocklist.c bookmarks.c cache.c charsets.c connect.c cookies.c date.c default.c dip.c directfb.c directfb_cursors.h dither.c dns.c download.c drivers.c encoding.c error.c file.c finger.c font_include.c font_manager.c font.c font_builtin.c font_ft.c framebuffer.c ftp.c gdbm.c gif.c globhist.c html.c html_gr.c html_r.c html_tbl.c http.c https.c internal.c img.c imgcache.c jpeg.c kbd.c language.c links_icon.c listedit.c lru.c lua.c  mailto.c main.c menu.c memory.c options.c options_hooks.c options_manager.c options_register.c objreq.c os_dep.c pmshell.c png.c print.c sched.c select.c selection.c session.c svgalib.c terminal.c tiff.c types.c url.c view.c view_gr.c win32.c x.c xbm.c \
	bits.h codepage.h cfg.h language.h links.h options_hooks.h os_dep.h os_depx.h setup.h

#links_DEPENDENCIES = jsNew/libjsengine.a jsNew/libjs.a
links_DEPENDENCIES = js/libjsengine.a
links_LDADD = $(links_LDADD_ATHEOS) $(links_DEPENDENCIES)

dist-hook:
#remove the symlinks:
	rm $(distdir)/README
	rm $(distdir)/INSTALL
	echo Links $(VERSION) > $(distdir)/README
	echo Links $(VERSION) -- How To Install > $(distdir)/INSTALL
	echo Links $(VERSION) | tr -c '\n' '-' >> $(distdir)/README
	echo Links $(VERSION) -- How To Install | tr -c '\n' '-' >> $(distdir)/INSTALL
	echo >> $(distdir)/README
	echo >> $(distdir)/INSTALL
	cat $(srcdir)/README >> $(distdir)/README
	cat $(srcdir)/INSTALL >> $(distdir)/INSTALL
	tar c $(distdir) | bzip2 -9 > links-$(VERSION).tar.bz2

CXXFLAGS=@CXXFLAGS@

atheos.o: atheos.cpp
	$(CXX) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) -c atheos.cpp

        
datadir = $(prefix)/@DATADIRNAME@
LIBS = @LIBS@

