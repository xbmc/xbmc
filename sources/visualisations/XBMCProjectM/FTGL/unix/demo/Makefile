top_srcdir=../..
VPATH=$(top_srcdir)/demo
top_builddir=..
-include $(top_builddir)/Make.conf

TARGET = FTGLDemo

all: $(TARGET)

SRCS = \
	FTGLDemo.cpp			\
	tb.c				\
	trackball.c			\

HEADERS = \
	tb.h				\
	trackball.h			\

OBJS = $(patsubst %.cpp,%.$(OBJEXT),$(filter %.cpp,$(SRCS)))
OBJS += $(patsubst %.c,%.$(OBJEXT),$(filter %.c,$(SRCS)))

ALL_OBJS = $(OBJS)

CPPFLAGS += $(FT2_CFLAGS)
CPPFLAGS += -I$(top_srcdir)/src

LIBS += $(FT2_LIBS) $(GLUT_LIBS)

ifeq ($(HAVE_GLUT),yes)
FTGLDemo: $(OBJS)
	$(LIBTOOL) --mode=link $(CXX) $^ -o $@ $(top_builddir)/src/libftgl.la $(LIBGLUT) $(LIBS)

install-local: FTGLDemo
	$(INSTALL) -d -m 0755 $(bindir)
	$(LIBTOOL) --mode=install $(INSTALL) -m 0755 $(TARGET) $(bindir)
else
FTGLDemo:
	@echo GLUT not available, demo won\'t be compiled

install-local:
endif

clean-local:
	$(RM) $(ALL_OBJS)

distclean-local:
	$(RM) $(TARGET)
	$(RM) -r .libs

include $(top_builddir)/Make.rules
