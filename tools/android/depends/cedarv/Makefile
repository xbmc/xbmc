include ../Makefile.include

TARBALL=allwinner-a10-video.tar.gz
URL=https://github.com/amery/allwinner-a10-video/tarball/master

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
	  tar xvfz $(TARBALLS_LOCATION)/$(TARBALL) &&\
	  cd amery-allwinner-a10-video*/libcedarv/android &&\
	  rm Makefile .gitignore cedardefs.mk &&\
	  mv * ../../../../ \
	)
	rm -rf tmp $(OBJS)
	touch $@
	
$(TARBALLS_LOCATION)/$(TARBALL):
	install -d $(TARBALLS_LOCATION)
	wget $(URL) -O $(TARBALLS)/$(TARBALL)

clean:
	rm -f $(TARGET) $(OBJS)
	rm -rf tmp adapter fbm libcedarv libvecore vbv
	rm -f .installed .extracted cedardev_api.h drv_display_sun4i.h

