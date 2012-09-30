include ../Makefile.include

TARBALL=A10-MEDIA-SDK-2012-04.tar.bz2
LOCATION=http://dl.dropbox.com/u/24940371/A10

SRCS = \
	libcedarv/vdecoder.c \
	libcedarv/awprintf.c \
	fbm/fbm.c \
	vbv/vbv.c \
	adapter/libve_adapter.c \
	adapter/os_adapter.c

OBJS = $(SRCS:%.c=%.o)

TARGET=libcedarv.a

all: $(TARGET)

$(TARGET) : .extracted $(OBJS)
	rm -f $(TARGET)
	$(AR) cq $(TARGET) $(OBJS)

.extracted: $(TARBALLS_LOCATION)/$(TARBALL)
	rm -rf tmp	
	mkdir tmp
	( cd tmp &&\
	  tar xvfj $(TARBALLS_LOCATION)/$(TARBALL) --wildcards "./libvecoredemo_android_A10_yuv2rgb-2012.1.13*" &&\
	  7z x *.rar &&\
	  cd libvecoredemo_android_A10_yuv2rgb &&\
	  mv cedardev_api.h ../.. &&\
	  mv drv_display.h ../../drv_display_sun4i.h &&\
	  mv vdecoder/* ../.. \
	)
	rm -rf tmp $(OBJS)
	touch $@
	
$(TARBALLS_LOCATION)/$(TARBALL):
	install -d $(TARBALLS_LOCATION)
	wget $(LOCATION)/$(TARBALL) -O $(TARBALLS_LOCATION)/$(TARBALL)

clean:
	rm -f $(TARGET) $(OBJS)
	rm -rf tmp adapter fbm libcedarv libvecore vbv
	rm -f .installed .extracted cedardev_api.h drv_display_sun4i.h

