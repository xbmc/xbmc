include ../Makefile.include

SRCS = \
	./libcedarv/vdecoder.c \
	./libcedarv/awprintf.c \
	./fbm/fbm.c \
	./vbv/vbv.c \
	./adapter/libve_adapter.c \
	./adapter/os_adapter.c

OBJS = $(SRCS:%.c=%.o)

TARGET = libcedarv.a

all: $(TARGET)

$(TARGET) : .downloaded $(OBJS)
	rm -f $(TARGET)
	$(AR) cq $(TARGET) $(OBJS)

.downloaded:
	make cleansrcs
	rm -rf tmp
	mkdir tmp
	( cd tmp;\
	  wget https://github.com/amery/allwinner-a10-video/tarball/master -O tgz;\
	  tar xvfz tgz;\
	  cd amery*/a10_video_decode/libvecoredemo_android_A10_with_display;\
	  cp ../libvecoredemo_android_A10/vdecoder/adapter/cdxalloc/*.a vdecoder/adapter/cdxalloc;\
	  mv cedardev_api.h ../../../..;\
	  mv drv_display_sun4i.h ../../../..;\
	  mv vdecoder/* ../../../..;\
	)
	rm -rf tmp
	make cleanobjs
	(cd libcedarv; patch -p1 <../vdecoder.patch )
	touch .downloaded

clean: cleansrcs cleanobjs
	@rm -f .downloaded

cleansrcs:
	@rm -rf tmp adapter fbm libcedarv libvecore vbv
	@rm -f cedardev_api.h drv_display_sun4i.h

cleanobjs:
	@rm -f $(TARGET)
	@rm -f $(OBJS)
