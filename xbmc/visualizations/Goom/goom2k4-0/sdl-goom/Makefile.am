# the goom2 stand alone program
 
if HAVE_SDL
bin_PROGRAMS = goom2

goom2_LDADD = $(top_builddir)/src/libgoom2.la
goom2_LDFLAGS = $(SDL_LIBS) `gtk-config --libs`
goom2_SOURCES = frame_rate_tester.c gmtimer.c gtk-callbacks.c gtk-interface.c gtk-support.c pixeldoubler.c readme.c sdl_goom.c sdl_pixeldoubler.c surface.c
INCLUDES=$(SDL_CFLAGS) `gtk-config --cflags` -I../src/
endif

