include ../Makefile.buildtools
export CFLAGS=-I$(TOOLCHAIN)/include
export LDFLAGS=-L$(TOOLCHAIN)/lib
export CPPFLAGS=-I$(TOOLCHAIN)/include
export CXXFLAGS=-I$(TOOLCHAIN)/include

# lib name, version
LIBNAME=SDL
VERSION=1.2.14
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=./configure --prefix=$(TOOLCHAIN) \
  --without-x --disable-video-x11

LIBDYLIB=$(SOURCE)/.libs/$(LIBNAME).dylib

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)
	echo $(ARCHIVE) > .gitignore

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); patch -p0 < ../01-SDL_SetWidthHeight.patch
	cd $(SOURCE); patch -p0 < ../02-mmx.patch
	cd $(SOURCE); $(CONFIGURE)

$(LIBDYLIB): $(SOURCE)
	make -j 1 -C $(SOURCE)

.installed:
	make -C $(SOURCE) install
	touch $@

clean:
	make -C $(SOURCE) clean
	rm -f .installed

distclean::
	rm -rf $(SOURCE) .installed
