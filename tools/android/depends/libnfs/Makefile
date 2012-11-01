include ../Makefile.include
DEPS= ../Makefile.include 001_fix_hanewin.patch libnfs_android.patch Makefile rpcinc/*

# lib name, version
LIBNAME=libnfs
VERSION=0804e67
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --host=$(HOST)

RPC_HEADERS=rpcinc/auth.h rpcinc/auth_unix.h rpcinc/clnt.h rpcinc/pmap_clnt.h rpcinc/pmap_prot.h rpcinc/pmap_rmt.h rpcinc/rpc.h rpcinc/rpc_msg.h rpcinc/svc.h rpcinc/svc_auth.h rpcinc/types.h rpcinc/xdr.h

LIBDYLIB=$(PLATFORM)/lib/.libs/$(LIBNAME).a

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	-rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	mkdir -p $(PLATFORM)/include/rpc
	install -c -m 644  $(RPC_HEADERS) $(PLATFORM)/include/rpc/
	cd $(PLATFORM); patch -p0 < ../001_fix_hanewin.patch
	cd $(PLATFORM); patch -p0 < ../libnfs_android.patch
	cd $(PLATFORM); ./bootstrap
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	rm -f $(PREFIX)/lib/libnfs.la $(PREFIX)/lib/libnfs.so $(PREFIX)/lib/libnfs.so.0
	mv -f $(PREFIX)/lib/libnfs.so.0.0.0 $(PREFIX)/lib/libnfs.so
	mkdir -p $(PREFIX)/include/rpc
	cp -f rpcinc/* $(PREFIX)/include/rpc 
	$(RPL) -e "libnfs.so.0" "libnfs.so\x00\x00" $(PREFIX)/lib/libnfs.so
	-$(READELF) --dynamic $(PREFIX)/lib/libnfs.so | grep ibrary
	touch $@
clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -r .installed-$(PLATFORM)
	rm -rf $(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
