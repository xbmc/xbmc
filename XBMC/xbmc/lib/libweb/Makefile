ARCH=i486-linux#@ARCH@
DIRS=AppWeb
LIB=libAppWeb-$(ARCH).a
DEFINES += -DHAVE_APPWEB -DLINUX -D_XBMC
INCLUDES += -I. -I../../ -I../../linux -I../../../guilib -I../../utils -IAppWeb -IAppWeb/appweb -IAppWeb/http -IAppWeb/http/modules -IAppWeb/esp -IAppWeb/mpr -IAppWeb/ejs

OBJS=	WebServer.o \
			#XBMChttp.o \
			XBMCweb.o \
			XBMCConfiguration.o \

all: $(LIB)

.PHONY: compile

$(LIB): $(OBJS) AppWeb/lib/libappwebStatic.a
	$(AR) cru $@ $(OBJS) AppWeb/obj/*.o

AppWeb/lib/libappwebStatic.a: compile
	$(MAKE) -C AppWeb

test: test.o $(LIB)
	$(CXX) -o $@ $(LIB) test.o

include ../../../Makefile.include

