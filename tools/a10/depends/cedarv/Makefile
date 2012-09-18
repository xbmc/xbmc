include ../depends.mk

SRCS = \
	./libcedarv/vdecoder.c \
	./libcedarv/awprintf.c \
	./fbm/fbm.c \
	./vbv/vbv.c \
	./adapter/libve_adapter.c \
	./adapter/os_adapter.c

OBJS = $(SRCS:%.c=%.o)

LIBS += -lpthread ./vdecoder/adapter/cdxalloc/libcedarxalloc.so ./vdecoder/libvecore/libvecore.so

TARGET = libcedarv.a

all: $(TARGET)

$(TARGET) : .downloaded $(OBJS)
	rm -f $(TARGET)
	$(AR) cq $(TARGET) $(OBJS)

.downloaded:
	make cleansrcs
	mkdir tmp
	( cd tmp;\
	  wget https://dl.dropbox.com/u/65312725/libve.tgz;\
	  tar xvfz libve.tgz;\
	  cd libve;\
	  mv cedardev_api.h ../..;\
	  mv render/drv_display_sun4i.h ../..;\
	  mv vdecoder/* ../..;\
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
