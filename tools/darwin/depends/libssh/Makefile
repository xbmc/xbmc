include ../Makefile.include
include ../config.site.mk

# lib name, version
LIBNAME=libssh
VERSION=0.5.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

BASE_URL=http://www.libssh.org/files/0.5

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) \

LIBDYLIB=$(SOURCE)/build/src/$(LIBNAME).dylib

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); rm -rf build; mkdir -p build
	cd $(SOURCE)/build; CC=$(CC) CXX=$(CXX) cmake        \
	  -DCMAKE_C_FLAGS:STRING="$(CPPFLAGS) $(CFLAGS)"     \
	  -DCMAKE_CXX_FLAGS:STRING="$(CPPFLAGS) $(CXXFLAGS)" \
	  -DCMAKE_INCLUDE_PATH=$(PREFIX)/include             \
	  -DCMAKE_LIBRARY_PATH=$(PREFIX)/lib                 \
	  -DCMAKE_INSTALL_NAME_DIR=$(PREFIX)/lib             \
      	  -DCMAKE_INSTALL_PREFIX=$(PREFIX) ..

$(LIBDYLIB): $(SOURCE)
	make -j 1 -C $(SOURCE)/build

.installed:
	make -C $(SOURCE)/build install
	touch $@

clean:
	rm -rf $(SOURCE) .installed

distclean::
	rm -rf $(SOURCE) .installed
