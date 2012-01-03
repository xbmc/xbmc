include ../Makefile.include

# lib name, version
LIBNAME=libbluray
VERSION=0.2.1
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --exec-prefix=$(PREFIX) \
  --disable-examples --disable-doxygen-doc

LIBDYLIB=$(SOURCE)/src/.libs/$(LIBNAME).dylib

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	# libbluray has borked Makefile.am with respect to CFLAGS, remove the offending line
	sed -i -e "s|CFLAGS=|#CFLAGS=|" $(SOURCE)/src/Makefile.am
	sed -i -e "s|CFLAGS=|#CFLAGS=|" $(SOURCE)/src/examples/Makefile.am
	cd $(SOURCE); ./bootstrap
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
