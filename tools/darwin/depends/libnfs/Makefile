include ../Makefile.include
include ../config.site.mk

# lib name, version
LIBNAME=libnfs
VERSION=0977fbb
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --without-examples

RPC_HEADERS=rpcinc/auth.h rpcinc/auth_unix.h rpcinc/clnt.h rpcinc/pmap_clnt.h rpcinc/pmap_prot.h rpcinc/pmap_rmt.h rpcinc/rpc.h rpcinc/rpc_msg.h rpcinc/svc.h rpcinc/svc_auth.h rpcinc/types.h rpcinc/xdr.h

DYLIBNAME=libnfs.1.0.0.dylib
LIBDYLIB=$(SOURCE)/lib/$(DYLIBNAME)

CLEAN_FILES=$(ARCHIVE) $(SOURCE)

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	-rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	mkdir -p $(PREFIX)/include/rpc
	install -c -m 644  $(RPC_HEADERS) $(PREFIX)/include/rpc/
	cd $(SOURCE); ./autogen.sh
	cd $(SOURCE); $(CONFIGURE)
	cd $(SOURCE); patch -p0 < ../printf.patch

$(LIBDYLIB): $(SOURCE)
	make -C $(SOURCE)

.installed:
	make -C $(SOURCE) install
	rm -f $(PREFIX)/lib/libnfs.dylib
	ln -s $(PREFIX)/lib/$(DYLIBNAME) $(PREFIX)/lib/libnfs.dylib
	touch $@
clean:
	make -C $(SOURCE) clean
	rm -r .installed
	rm -rf $(SOURCE)

distclean::
	rm -rf $(SOURCE) .installed
