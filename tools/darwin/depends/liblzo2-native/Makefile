include ../Makefile.buildtools
export CFLAGS=-I$(TOOLCHAIN)/include
export LDFLAGS=-L$(TOOLCHAIN)/lib
export CPPFLAGS=-I$(TOOLCHAIN)/include
export CXXFLAGS=-I$(TOOLCHAIN)/include

# lib name, version
LIBNAME=lzo
VERSION=2.06
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=./configure --prefix=$(TOOLCHAIN) \
  --enable-shared=yes

LIBDYLIB=$(SOURCE)/src/.libs/lib$(LIBNAME)2.dylib

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); $(CONFIGURE)

$(LIBDYLIB): $(SOURCE)
	make -j $(MAKE_JOBS) -C $(SOURCE)

.installed:
	make -C $(SOURCE) install
	touch $@

clean:
	make -C $(SOURCE) clean
	rm -f .installed

distclean::
	rm -rf $(SOURCE) .installed
