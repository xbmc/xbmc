top_srcdir=../..
VPATH=$(top_srcdir)/src
top_builddir=..
-include $(top_builddir)/Make.conf

TARGET = libftgl.la

all: $(TARGET)

SRCS =				\
	FTBitmapGlyph.cpp	\
	FTCharmap.cpp		\
	FTContour.cpp		\
	FTExtrdGlyph.cpp	\
	FTFace.cpp		\
	FTFont.cpp		\
	FTGLBitmapFont.cpp	\
	FTGLExtrdFont.cpp	\
	FTGLOutlineFont.cpp	\
	FTGLPixmapFont.cpp	\
	FTGLPolygonFont.cpp	\
	FTGLTextureFont.cpp	\
	FTGlyph.cpp		\
	FTGlyphContainer.cpp	\
	FTLibrary.cpp		\
	FTOutlineGlyph.cpp	\
	FTPixmapGlyph.cpp	\
	FTPoint.cpp		\
	FTPolyGlyph.cpp		\
	FTSize.cpp		\
	FTTextureGlyph.cpp	\
	FTVectoriser.cpp

HEADERS =			\
	FTBBox.h		\
	FTBitmapGlyph.h		\
	FTCharToGlyphIndexMap.h	\
	FTCharmap.h		\
	FTContour.h		\
	FTExtrdGlyph.h		\
	FTFace.h		\
	FTFont.h		\
	FTGL.h			\
	FTGLBitmapFont.h	\
	FTGLExtrdFont.h		\
	FTGLOutlineFont.h	\
	FTGLPixmapFont.h	\
	FTGLPolygonFont.h	\
	FTGLTextureFont.h	\
	FTGlyph.h		\
	FTGlyphContainer.h	\
	FTLibrary.h		\
	FTList.h		\
	FTOutlineGlyph.h	\
	FTPixmapGlyph.h		\
	FTPoint.h		\
	FTPolyGlyph.h		\
	FTSize.h		\
	FTTextureGlyph.h	\
	FTVector.h		\
	FTVectoriser.h

HEADER_FILES = $(addprefix $(top_srcdir)/include/,$(HEADERS))

LOBJS = $(patsubst %.cpp,%.lo,$(SRCS))
OBJS = $(patsubst %.cpp,%.$(OBJEXT),$(SRCS))

ALL_OBJS = $(LOBJS) $(OBJS)

CXXFLAGS += $(FT2_CFLAGS) $(GL_CFLAGS)
LIBS += $(FT2_LIBS) $(GL_LIBS)

libftgl.la: $(LOBJS)

install-local: libftgl.la
	$(INSTALL) -d -m 0755 $(libdir)
	$(LIBTOOL) --mode=install $(INSTALL) -m 0644 $(TARGET) $(libdir)
	$(INSTALL) -d -m 0755 $(includedir)/FTGL
	$(INSTALL) -m 0644 $(HEADER_FILES) $(includedir)/FTGL


clean-local:
	$(RM) $(ALL_OBJS)

distclean-local:
	$(RM) $(TARGET)
	$(RM) -r .libs ii_files

include $(top_builddir)/Make.rules
