include ../depends.mk

TARBALL=cedarx-libs.tar.gz
URL=https://github.com/linux-sunxi/cedarx-libs/tarball/master

ifeq ($(USEARMHF), 1)
LIBVERSION=linux-armhf
else
LIBVERSION=linux-armel
endif

SRCS = \
	libcedarv/vdecoder.c \
	libcedarv/awprintf.c \
	fbm/fbm.c \
	vbv/vbv.c \
	adapter/libve_adapter.c \
	adapter/os_adapter.c

ifeq ($(USEARMHF), 1)
SRCS += adapter/avheap/avheap.c
endif

OBJS = $(SRCS:%.c=%.o)

TARGET=libcedarv.a

all: .installed

.installed: $(TARGET)
	install -d $(XBMCPREFIX)/lib
ifneq ($(USEARMHF), 1)
	cp adapter/cdxalloc/libcedarxalloc.so $(XBMCPREFIX)/lib
endif
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
	  cd linux-sunxi-*/libcedarv/$(LIBVERSION) &&\
	  rm Makefile .gitignore cedardefs.mk &&\
	  rm -rf sample &&\
	  mv * ../../../../ \
	)
	rm -rf tmp $(OBJS)
	touch $@
	
$(TARBALLS)/$(TARBALL):
	install -d $(TARBALLS)
	wget $(URL) -O $(TARBALLS)/$(TARBALL)

clean:
	rm -f $(TARGET) $(OBJS)
	rm -rf adapter fbm libcedarv libvecore vbv
	rm -f .installed .extracted cedardev_api.h drv_display_sun4i.h

