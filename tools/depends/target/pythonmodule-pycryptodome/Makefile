include ../../Makefile.include
DEPS= ../../Makefile.include Makefile 01-nosetuptool.patch 02-android-dlopen.patch 03-obey-crosscompileflags.patch

VERSION.TXT := $(CMAKE_SOURCE_DIR)/version.txt
APP_NAME=$(shell awk '/APP_NAME/ {print tolower($$2)}' $(VERSION.TXT))

# lib name, version
LIBNAME=pycryptodome
VERSION=3.4.3
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

CROSSFLAGS=PYTHONXCPREFIX="$(PREFIX)" CC="$(CC) $(CFLAGS)" LDSHARED="$(CC) -shared" LDFLAGS="$(LDFLAGS)" PYTHONPATH="$(PREFIX)/lib/python3.8/site-packages/"
ifeq ($(OS),android)
CROSSFLAGS=PYTHONXCPREFIX="$(PREFIX)" CC="$(CC) $(CFLAGS)" LDSHARED="$(CC) -shared" LDFLAGS="$(LDFLAGS) -L$(PREFIX)/lib/dummy-lib$(APP_NAME)/ -l$(APP_NAME) -lm" PYTHONPATH="$(PREFIX)/lib/python3.8/site-packages/"
endif

ifeq (darwin, $(findstring darwin, $(HOST)))
ifeq ($(OS),darwin_embedded)
PYTHON_O=$(abs_top_srcdir)/target/python3/$(PLATFORM)/Programs/python.o
endif
#ensure that only our target ldflags are passed to the python build
LDSHARED:=$(CC) -bundle -undefined dynamic_lookup $(LDFLAGS)
CROSSFLAGS=PYTHONXCPREFIX="$(PREFIX)" CC="$(CC) $(CFLAGS)" CCSHARED="$(CC) $(CFLAGS) $(PYTHON_O)" LDFLAGS="$(LDFLAGS)" PYTHONPATH="$(PREFIX)/lib/python3.8/site-packages/" LDSHARED="$(LDSHARED)"
endif

LIBDYLIB=$(PLATFORM)/build/lib.$(OS)-$(CPU)-3.8/Cryptodome
ifeq ($(NATIVE_OS), osx)
	LIBDYLIB=$(PLATFORM)/build/lib.macosx-10.4-x86_64-3.8/Cryptodome
endif

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-nosetuptool.patch
ifeq ($(OS),android)
	cd $(PLATFORM); patch -p1 -i ../02-android-dlopen.patch
endif
	cd $(PLATFORM); patch -p1 -i ../03-obey-crosscompileflags.patch

$(LIBDYLIB): $(PLATFORM)
	mkdir -p $(PLATFORM)/output
	cd $(PLATFORM); touch .separate_namespace && $(CROSSFLAGS) $(NATIVEPREFIX)/bin/python3 setup.py build_ext --plat-name $(OS)-$(TARGET_ARCH)

.installed-$(PLATFORM): $(LIBDYLIB)
	cd $(PLATFORM); $(CROSSFLAGS) $(NATIVEPREFIX)/bin/python3 setup.py install --prefix=$(PREFIX)
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
