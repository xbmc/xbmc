include ../Makefile.include
include ../config.site.mk

LIBNAME=boost
VERSION=1_44_0
SOURCE=$(LIBNAME)_$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2

LIBDYLIB=$(PREFIX)/lib/libboost_thread.a

all: $(LIBDYLIB)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(LIBDYLIB): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	sed -e "s?@boost_flags@?$(boost_flags)?g" \
            -e "s?@platform_gcc_version@?$(platform_gcc_version)?g" \
            user-config.jam.in >> $(SOURCE)/tools/build/v2/user-config.jam
	cd $(SOURCE); ./bootstrap.sh --prefix=$(PREFIX) --with-libraries=thread
	cd $(SOURCE); ./bjam --prefix=$(PREFIX) $(bjam_args) install

clean:
	cd $(SOURCE); ./bjam --clean
	rm -rf $(PREFIX)/include/boost
	rm -f  $(PREFIX)/lib/libboost*
	rm -f .installed

distclean::
	rm -rf $(PREFIX)/include/boost
	rm -f  $(PREFIX)/lib/libboost*
	rm -rf $(SOURCE) .installed

