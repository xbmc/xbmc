# @(#) $Id: Makefile.am,v 1.29 2005/11/24 20:11:20 yeti Exp $
include_HEADERS = enca.h
lib_LTLIBRARIES = libenca.la

# In following order do:
# Any code change     C:   R++: A
# Any iface change    C++: 0:   A
# Adding ifaces       C:   R:   A++
# Changing ifaces     C:   R:   0
libenca_la_LDFLAGS = -version-info 5:1:5
libenca_la_SOURCES = \
  common.c \
  ctype.c \
  enca.c \
  enca.h \
  encnames.c \
  filters.c \
  guess.c \
  internal.h \
  lang.c \
  lang_be.c \
  lang_bg.c \
  lang_cs.c \
  lang_et.c \
  lang_hr.c \
  lang_hu.c \
  lang_lt.c \
  lang_lv.c \
  lang_pl.c \
  lang_ru.c \
  lang_sk.c \
  lang_sl.c \
  lang_uk.c \
  lang_zh.c \
  multibyte.c \
  pair.c \
  unicodemap.c \
  utf8_double.c
INCLUDES = -I$(top_srcdir)

clean-local:
	rm -f core.* *~
