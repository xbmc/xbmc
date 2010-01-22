AM_CPPFLAGS = -I$(top_srcdir)/src/libmodplug

if MINGW32
  AM_LDFLAGS = -no-undefined
endif
lib_LTLIBRARIES = libmodplug.la
libmodplug_la_LIBADD = -lm
libmodplug_la_SOURCES = tables.h         \
                        sndmix.cpp         \
                        sndfile.cpp        \
                        snd_fx.cpp         \
                        snd_flt.cpp        \
                        snd_dsp.cpp        \
                        fastmix.cpp        \
                        mmcmp.cpp          \
                        load_xm.cpp        \
                        load_wav.cpp       \
                        load_umx.cpp       \
                        load_ult.cpp       \
                        load_stm.cpp       \
                        load_s3m.cpp       \
                        load_ptm.cpp       \
                        load_okt.cpp       \
                        load_mtm.cpp       \
                        load_mod.cpp       \
                        load_med.cpp       \
                        load_mdl.cpp       \
                        load_it.cpp        \
                        load_far.cpp       \
                        load_dsm.cpp       \
                        load_dmf.cpp       \
                        load_dbm.cpp       \
                        load_ams.cpp       \
                        load_amf.cpp       \
                        load_669.cpp       \
                        load_j2b.cpp       \
                        load_mt2.cpp       \
                        load_psm.cpp       \
                        load_abc.cpp       \
                        load_mid.cpp       \
                        load_pat.cpp       \
                        modplug.cpp

libmodplugincludedir = $(includedir)/libmodplug


libmodpluginclude_HEADERS = libmodplug/stdafx.h libmodplug/sndfile.h libmodplug/it_defs.h modplug.h
noinst_HEADERS = load_pat.h
