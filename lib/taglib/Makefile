# lib name, version
LIBNAME=taglib
VERSION=1.8
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(LIBNAME)-$(VERSION).tar.gz
TARBALLS_LOCATION=.
BASE_URL=http://mirrors.xbmc.org/build-deps/darwin-libs
RETRIEVE_TOOL=/usr/bin/curl
RETRIEVE_TOOL_FLAGS=-Ls --create-dirs --output $(TARBALLS_LOCATION)/$(ARCHIVE)
ARCHIVE_TOOL=tar
ARCHIVE_TOOL_FLAGS=zxf

SO_NAME=libtaglib.so

all: $(SO_NAME)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); rm -rf build; mkdir -p build
	cd $(SOURCE)/build; cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local -DENABLE_STATIC=1
$(SO_NAME): $(SOURCE)
	make -C $(SOURCE)/build

install:
	make -C $(SOURCE)/build install

clean:
	make -C $(SOURCE)/build clean
	rm -f .installed

distclean::
	rm -rf $(SOURCE) .installed
