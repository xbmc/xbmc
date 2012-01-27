include ../Makefile.include
include ../config.site.mk

# lib name, version
LIBNAME=libnfs
VERSION=0804e67
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX)

RPC_HEADERS=rpcinc/auth.h rpcinc/auth_unix.h rpcinc/clnt.h rpcinc/pmap_clnt.h rpcinc/pmap_prot.h rpcinc/pmap_rmt.h rpcinc/rpc.h rpcinc/rpc_msg.h rpcinc/svc.h rpcinc/svc_auth.h rpcinc/types.h rpcinc/xdr.h

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
	cd $(SOURCE); patch -p0 < ../001_fix_hanewin.patch
	cd $(SOURCE); ./bootstrap
	cd $(SOURCE); $(CONFIGURE)

$(LIBDYLIB): $(SOURCE)
	make -C $(SOURCE)

.installed:
	make -C $(SOURCE) install
	touch $@
clean:
	make -C $(SOURCE) clean
	rm -r .installed
	rm -rf $(SOURCE)

distclean::
	rm -rf $(SOURCE) .installed
