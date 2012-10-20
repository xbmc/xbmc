include ../../Makefile.include
include ../depends.mk

# lib name, version
LIBNAME=libshairport
VERSION=1.2.0.20310_lib
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --host=$(HOST)


LIBDYLIB=$(SOURCE)/.libs/$(LIBNAME).so

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); patch -p0 < ../001_add_ao.patch
	cd $(SOURCE); patch -p0 < ../002_fix_install_header.patch
	cd $(SOURCE); patch -p0 < ../003_fix_deadlock.patch
	cd $(SOURCE); patch -p0 < ../004_fix_bad_access.patch
	cd $(SOURCE); patch -p0 < ../005_fix_shutdown.patch
	cd $(SOURCE); patch -p0 < ../006_no_printf.patch
	cd $(SOURCE); patch -p0 < ../007_fix_syslog_defines.patch
	cd $(SOURCE); patch -p0 < ../008-add-missing-libs.patch
	cd $(SOURCE); patch -p0 < ../009_fix_ipv6.patch
	cd $(SOURCE); patch -p0 < ../010_handle_metadata.patch
	cd $(SOURCE); patch -p0 < ../011_fix_ipv4_fallback.patch
	cd $(SOURCE); autoreconf -vif
	cd $(SOURCE); $(CONFIGURE)

$(LIBDYLIB): $(SOURCE)
	make -j 1 -C $(SOURCE)

.installed:
	make -C $(SOURCE) install
	touch $@

clean:
	rm -rf $(SOURCE) .installed

distclean::
	rm -rf $(SOURCE) .installed

