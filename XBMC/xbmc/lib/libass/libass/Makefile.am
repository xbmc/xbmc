LIBASS_LT_CURRENT = 1
LIBASS_LT_REVISION = 0
LIBASS_LT_AGE = 0

lib_LTLIBRARIES = libass.la
libass_la_SOURCES = ass.c ass_cache.c ass_font.c ass_fontconfig.c ass_render.c \
                    ass_utils.c ass_bitmap.c ass_library.c mputils.c \
                    ass_bitmap.h ass_cache.h ass_fontconfig.h ass_font.h ass.h \
                    ass_library.h ass_types.h ass_utils.h help_mp.h mputils.h
libass_la_LDFLAGS = -version-info $(LIBASS_LT_CURRENT):$(LIBASS_LT_REVISION):$(LIBASS_LT_AGE)

assheadersdir = $(includedir)/ass
dist_assheaders_HEADERS = ass.h ass_types.h

