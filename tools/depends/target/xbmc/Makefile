-include ../../Makefile.include

SOURCE=../../../../

export CXXFLAGS+=-O3
export CFLAGS+=-O3

# configuration settings
CONFIGURE = cp -f $(CONFIG_SUB) $(CONFIG_GUESS) build-aux/ ;\
  ./configure --prefix=$(PREFIX)

ifeq ($(OS),android)
CONFIGURE += --enable-codec=libstagefright,amcodec
endif

CONFIGURE += $(CONFIG_EXTRA)

all: $(SOURCE)/libxbmc.so


$(SOURCE)/libxbmc.so:
	cd $(SOURCE); ./bootstrap
	cd $(SOURCE); $(CONFIGURE)

../../Makefile.include:
	$(error Please run configure)

clean:
	cd $(SOURCE); $(MAKE) clean

distclean:
	cd $(SOURCE); $(MAKE) clean
