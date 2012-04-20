AM_CFLAGS = -Wall

LIBASS_LT_CURRENT = 5
LIBASS_LT_REVISION = 0
LIBASS_LT_AGE = 1

lib_LTLIBRARIES = libass.la
libass_la_SOURCES = ass.c ass_cache.c ass_font.c ass_fontconfig.c ass_render.c \
                    ass_utils.c ass_bitmap.c ass_library.c ass_bitmap.h \
                    ass_cache.h ass_fontconfig.h ass_font.h ass.h \
                    ass_library.h ass_types.h ass_utils.h ass_drawing.c \
                    ass_drawing.h ass_cache_template.h ass_render.h \
                    ass_parse.c ass_parse.h ass_render_api.c ass_shaper.c \
                    ass_shaper.h ass_strtod.c
libass_la_LDFLAGS = -version-info $(LIBASS_LT_CURRENT):$(LIBASS_LT_REVISION):$(LIBASS_LT_AGE)
libass_la_LDFLAGS += -export-symbols $(srcdir)/libass.sym

assheadersdir = $(includedir)/ass
dist_assheaders_HEADERS = ass.h ass_types.h

EXTRA_DIST = libass.sym
