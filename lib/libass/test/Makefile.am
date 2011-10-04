AM_CFLAGS = -Wall

noinst_PROGRAMS = test
test_SOURCES = test.c
test_CPPFLAGS = -I../libass
test_LDADD = ../libass/libass.la
test_LDFLAGS = $(AM_LDFLAGS) $(LIBPNG_LIBS)
