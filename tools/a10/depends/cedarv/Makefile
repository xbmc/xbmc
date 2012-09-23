include ../depends.mk

TARBALL=libve.tgz
LOCATION=https://dl.dropbox.com/u/65312725

SRCS = \
	libcedarv/vdecoder.c \
	libcedarv/awprintf.c \
	fbm/fbm.c \
	vbv/vbv.c \
	adapter/libve_adapter.c \
	adapter/os_adapter.c

OBJS = $(SRCS:%.c=%.o)

TARGET=libcedarv.a

all: .installed

.installed: $(TARGET)
	install -d $(XBMCPREFIX)/lib
	cp adapter/cdxalloc/libcedarxalloc.so $(XBMCPREFIX)/lib
	cp libvecore/libvecore.so $(XBMCPREFIX)/lib
	touch $@

$(TARGET) : .extracted $(OBJS)
	rm -f $(TARGET)
	$(AR) cq $(TARGET) $(OBJS)

.extracted: $(TARBALLS)/$(TARBALL)
	rm -rf tmp	
	mkdir tmp
	( cd tmp &&\
	  tar xvfz $(TARBALLS)/$(TARBALL) &&\
	  cd libve &&\
	  mv cedardev_api.h ../.. &&\
	  mv render/drv_display_sun4i.h ../.. &&\
	  mv vdecoder/* ../.. \
	)
	rm -rf tmp $(OBJS)
	(cd libcedarv; patch -p1 <../vdecoder.patch)
	touch $@
	
$(TARBALLS)/$(TARBALL):
	install -d $(TARBALLS)
	wget $(LOCATION)/$(TARBALL) -O $(TARBALLS)/$(TARBALL)

clean:
	rm -f $(TARGET) $(OBJS)
	rm -rf adapter fbm libcedarv libvecore vbv
	rm -f .installed .extracted cedardev_api.h drv_display_sun4i.h

