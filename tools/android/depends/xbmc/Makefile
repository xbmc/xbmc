-include ../Makefile.include

SOURCE=../../../../

export PYTHON_VERSION=2.6
export PYTHON_LDFLAGS=-L$(PREFIX)/lib -lpython$(PYTHON_VERSION) -lc -ldl -lm -lexpat -lffi -lintl
export PYTHON_CPPFLAGS=-I$(PREFIX)/include/python$(PYTHON_VERSION)
export PYTHON_SITE_PKG=$(PREFIX)/lib/python$(PYTHON_VERSION)/site-packages
export PYTHON_NOVERSIONCHECK=no-check
export USE_TEXTUREPACKER_NATIVE_ROOT=$(NATIVEPREFIX)
export MYSQL_CONFIG=$(PREFIX)/bin/mysql_config
export LIBS+=-lcrystax

# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) build-aux/ ;\
  ./configure --prefix=$(PREFIX) --host=$(HOST) \
  --enable-neon --enable-gles --enable-debug \
  --disable-sdl --disable-x11 --disable-xrandr \
  --disable-optical-drive --disable-joystick \
  --enable-shared-lib --disable-alsa


all: $(SOURCE)/libxbmc.so


$(SOURCE)/libxbmc.so:
	cd $(SOURCE); ./bootstrap
	cd $(SOURCE); $(CONFIGURE)
	cd $(SOURCE); $(MAKE)
	cd $(SOURCE); $(MAKE) apk

../Makefile.include:
	$(error Please run edit and run setup-sdk.sh)

clean:
	cd $(SOURCE); $(MAKE) clean

distclean:
	cd $(SOURCE); $(MAKE) clean
