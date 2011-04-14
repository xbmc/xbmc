include ../Makefile.include

# lib name, version
LIBNAME=samba
VERSION=3.5.6
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) \
  --without-cluster-support --disable-swat --without-ldap \
  --without-pam --without-pam_smbpass --with-fhs

LIBDYLIB=$(SOURCE)/source3/bin/libsmbclient.dylib

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(LIBDYLIB): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE)/source3; ./autogen.sh
	cd $(SOURCE)/source3; $(CONFIGURE)
	#cp -f /Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator$(platform_sdk_version).sdk/usr/include/crt_externs.h $(PREFIX)/include/
	cd $(SOURCE)/source3; make bin/libsmbclient.dylib

.installed:
	cp -f $(SOURCE)/source3/include/libsmbclient.h $(PREFIX)/include/
	cp -f $(SOURCE)/source3/bin/libtdb.dylib.1 $(PREFIX)/lib/
	cp -f $(SOURCE)/source3/bin/libtalloc.dylib.2 $(PREFIX)/lib/
	cp -f $(SOURCE)/source3/bin/libsmbclient.dylib.0 $(PREFIX)/lib/
	cd $(PREFIX)/lib; ln -sf libsmbclient.dylib.0 libsmbclient.dylib
	install_name_tool -id $(PREFIX)/lib/libtdb.dylib.1 $(PREFIX)/lib/libtdb.dylib.1
	install_name_tool -id $(PREFIX)/lib/libtalloc.dylib.2 $(PREFIX)/lib/libtalloc.dylib.2
	install_name_tool -id $(PREFIX)/lib/libsmbclient.dylib.0 $(PREFIX)/lib/libsmbclient.dylib.0
	install_name_tool -change bin/libtdb.dylib.1 $(PREFIX)/lib/libtdb.dylib.1 \
                $(PREFIX)/lib/libsmbclient.dylib.0
	install_name_tool -change bin/libtalloc.dylib.2 $(PREFIX)/lib/libtalloc.dylib.2 \
		$(PREFIX)/lib/libsmbclient.dylib.0
	touch $@

clean:
	make -C $(SOURCE)/source3 clean
	rm -f .installed

distclean::
	rm -rf $(SOURCE) .installed
