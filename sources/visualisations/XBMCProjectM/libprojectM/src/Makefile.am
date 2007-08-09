lib_LTLIBRARIES = libprojectM.la

libprojectM_la_SOURCES=\
PCM.cc \
beat_detect.cc \
browser.cc \
builtin_funcs.cc \
console_interface.cc \
custom_shape.cc \
custom_wave.cc \
editor.cc \
eval.cc \
fftsg.cc \
func.cc \
glConsole.cc \
init_cond.cc \
menu.cc \
param.cc \
parser.cc \
pbuffer.cc \
per_frame_eqn.cc \
per_pixel_eqn.cc \
preset.cc \
projectM.cc \
splaytree.cc \
timer.cc \
tree_types.cc \
wipemalloc.cc

libprojectMincludedir = $(includedir)/projectM
libprojectMinclude_HEADERS = projectM.h pbuffer.h event.h console_interface.h PCM.h

libprojectM_la_LDFLAGS = -version-info 0:0:0

AM_LIBS = @GL_LIBS@ @FTGL_LIBS@
AM_CXXFLAGS=  -DLINUX -D__CPLUSPLUS @GL_CFLAGS@ @FTGL_CFLAGS@

libprojectM_la_LIBADD = $(AM_LIBS)

libprojectMconfigdir = $(datadir)/projectM
libprojectMconfig_DATA = share/config share/config.fastcomputers share/config.slowcomputers

libprojectMfontdir = $(datadir)/projectM/fonts
libprojectMfont_DATA = share/fonts/*.ttf

libprojectMpresetdir = $(datadir)/projectM/presets
libprojectMpreset_DATA= share/presets/*.milk

