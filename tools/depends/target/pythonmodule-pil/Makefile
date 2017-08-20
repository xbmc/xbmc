include ../../Makefile.include
DEPS= ../../Makefile.include Makefile pillow-crosscompile.patch

VERSION.TXT := $(CMAKE_SOURCE_DIR)/version.txt
APP_NAME=$(shell awk '/APP_NAME/ {print tolower($$2)}' $(VERSION.TXT))

# lib name, version
LIBNAME=pillow
VERSION=3.0.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

CROSSFLAGS=PYTHONXCPREFIX="$(PREFIX)" CC="$(CC) $(CFLAGS)" LDSHARED="$(CC) -shared" LDFLAGS="$(LDFLAGS)" PYTHONPATH="$(PREFIX)/lib/python3.7/site-packages/"
ifeq ($(OS),android)
PILPATH=$(PREFIX)/share/$(APP_NAME)/addons/script.module.pil
CROSSFLAGS=PYTHONXCPREFIX="$(PREFIX)" CC="$(CC) $(CFLAGS)" LDSHARED="$(CC) -shared" LDFLAGS="$(LDFLAGS) -L$(PREFIX)/lib/dummy-lib$(APP_NAME)/ -l$(APP_NAME) -lm" PYTHONPATH="$(PILPATH):$(PREFIX)/lib/python3.7/site-packages/"
endif

ifeq (darwin, $(findstring darwin, $(HOST)))
ifeq ($(OS),darwin_embedded)
PYTHON_O=$(abs_top_srcdir)/target/python3/$(PLATFORM)/Programs/python.o
endif
#ensure that only our target ldflags are passed to the python build
LDSHARED:=$(CC) -bundle -undefined dynamic_lookup $(LDFLAGS)
CROSSFLAGS=PYTHONXCPREFIX="$(PREFIX)" CC="$(CC) $(CFLAGS)" CCSHARED="$(CC) $(CFLAGS) $(PYTHON_O)" LDFLAGS="$(LDFLAGS)" PYTHONPATH="$(PREFIX)/lib/python3.7/site-packages/" LDSHARED="$(LDSHARED)"
endif

LIBDYLIB=$(PLATFORM)/dist/Pillow-$(VERSION)-py3.7-$(OS)-$(CPU).egg
ifeq ($(OS),android)
LIBDYLIB=$(PREFIX)/share/$(APP_NAME)/addons/script.module.pil/lib/PIL/_imaging.so
else ifeq (darwin, $(findstring darwin, $(HOST)))
LIBDYLIB=$(PLATFORM)/dist/Pillow-$(VERSION)-py3.7-macosx-10.4-x86_64.egg
endif

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../pillow-crosscompile.patch

$(LIBDYLIB): $(PLATFORM)
	mkdir -p $(PLATFORM)/output
	cd $(PLATFORM); $(CROSSFLAGS) $(NATIVEPREFIX)/bin/python3 setup.py build_ext --plat-name $(OS)-$(CPU)

.installed-$(PLATFORM): $(LIBDYLIB)
ifeq ($(OS),android)
	mkdir -p $(PREFIX)/share/$(APP_NAME)/addons/script.module.pil/lib
	cd $(PLATFORM); $(CROSSFLAGS) $(NATIVEPREFIX)/bin/python3 setup.py install --install-lib  $(PREFIX)/share/$(APP_NAME)/addons/script.module.pil
	cd $(PREFIX)/share/$(APP_NAME)/addons/script.module.pil/lib && unzip -o ../Pillow-*.egg
	cd $(PREFIX)/share/$(APP_NAME)/addons/script.module.pil && rm -rf Pillow-*.egg
else
	cd $(PLATFORM); $(CROSSFLAGS) $(NATIVEPREFIX)/bin/python3 setup.py install --prefix=$(PREFIX)
endif
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
