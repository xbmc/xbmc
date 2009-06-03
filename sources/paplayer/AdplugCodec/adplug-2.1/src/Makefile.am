lib_LTLIBRARIES = libadplug.la

libadplug_la_SOURCES = adplug.cpp emuopl.cpp fmopl.c diskopl.cpp debug.c \
debug.h fprovide.cpp player.cpp database.cpp hsc.cpp sng.cpp imf.cpp \
players.cpp protrack.cpp a2m.cpp adtrack.cpp amd.cpp bam.cpp d00.cpp dfm.cpp \
hsp.cpp ksm.cpp mad.cpp mid.cpp mkj.cpp cff.cpp dmo.cpp s3m.cpp dtm.cpp \
fmc.cpp mtk.cpp rad.cpp raw.cpp sa2.cpp xad.cpp flash.cpp bmf.cpp hybrid.cpp \
hyp.cpp psi.cpp rat.cpp u6m.cpp rol.cpp mididata.h xsm.cpp adlibemu.c dro.cpp \
lds.cpp realopl.cpp analopl.cpp temuopl.cpp msc.cpp rix.cpp adl.cpp

libadplug_la_LDFLAGS = -release @VERSION@ -version-info 0 $(libbinio_LIBS)

# -Dstricmp=strcasecmp is a hack. Throughout AdPlug, stricmp() is used to do
# caseless string comparations. UNIX libcs don't support stricmp(), but do
# support the BSD strcasecmp(), so we have to replace every occurence of
# stricmp() into strcasecmp(). stricmp() seems to be Windows, but maybe
# also POSIX...
AM_CPPFLAGS = -Dstricmp=strcasecmp $(libbinio_CFLAGS)

pkginclude_HEADERS = adplug.h emuopl.h fmopl.h silentopl.h opl.h diskopl.h \
a2m.h amd.h bam.h d00.h dfm.h hsc.h hsp.h imf.h ksm.h lds.h mid.h mkj.h \
mtk.h protrack.h rad.h raw.h s3m.h sa2.h sng.h u6m.h player.h fmc.h mad.h \
xad.h bmf.h flash.h hyp.h psi.h rat.h hybrid.h rol.h adtrack.h cff.h dtm.h \
dmo.h fprovide.h database.h players.h xsm.h adlibemu.h kemuopl.h dro.h \
realopl.h analopl.h temuopl.h msc.h rix.h adl.h
