AUTOMAKE_OPTIONS = subdir-objects

AM_CFLAGS = -DTARGET_POSIX -DUSE_LZO_PACKING
AM_CFLAGS += @EXTRA_DEFINES@
AM_CXXFLAGS = $(AM_CFLAGS)

AM_CPPFLAGS = \
  -I. \
  -I./decoder \
  -I@KODI_SRC_DIR@/lib \
  -I@KODI_SRC_DIR@/xbmc \
  -I@KODI_SRC_DIR@/xbmc/guilib \
  -I@KODI_SRC_DIR@/xbmc/linux \
  @CPPFLAGS@

AM_LDFLAGS = @LIBS@ @STATIC_FLAG@


bin_PROGRAMS = TexturePacker
TexturePacker_SOURCES =  md5.cpp \
  XBTFWriter.cpp \
  TexturePacker.cpp \
  DecoderManager.cpp \
  decoder/PNGDecoder.cpp \
  decoder/JPGDecoder.cpp \
  decoder/GifHelper.cpp \
  decoder/GIFDecoder.cpp \
  XBTF.cpp

XBTF.cpp:
	@cp @KODI_SRC_DIR@/xbmc/guilib/XBTF.cpp .
