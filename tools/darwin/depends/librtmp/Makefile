include ../Makefile.include
include ../config.site.mk

# lib name, version
LIBNAME=rtmpdump
VERSION=2.3
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tgz

# configuration settings

LIBDYLIB=$(SOURCE)/.libs/$(LIBNAME).dylib

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); patch -p1 < ../01-make_shared_lib_for_darwin-tag2.3.patch

$(LIBDYLIB): $(SOURCE)
	make SYS=darwin prefix=$(PREFIX) XCFLAGS="$(CFLAGS)" XLDFLAGS="$(LDFLAGS)" -j $(MAKE_JOBS) -C $(SOURCE)/librtmp

.installed:
	make SYS=darwin prefix=$(PREFIX) -C $(SOURCE)/librtmp install
	touch $@

clean:
	make SYS=darwin -C $(SOURCE) clean
	rm -f .installed

distclean::
	rm -rf $(SOURCE) .installed
