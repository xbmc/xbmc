include ../Makefile.include

# lib name, version
LIBNAME=tinyxml
VERSION=2.6.2
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
export CXXFLAGS += -DTIXML_USE_STL
CONFIGURE=./configure --prefix=$(PREFIX)


LIBDYLIB=$(SOURCE)/.libs/$(LIBNAME).dylib

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); patch -p2 < ../enforce-use-stl.patch
	cd $(SOURCE); autoreconf -vif
	cd $(SOURCE); $(CONFIGURE)

$(LIBDYLIB): $(SOURCE)
	make -j 1 -C $(SOURCE)/src

.installed:
	make -C $(SOURCE)/src install
	touch $@

clean:
	rm -rf $(SOURCE) .installed

distclean::
	rm -rf $(SOURCE) .installed

