#  A quick and dirty Makefile to download/build and install
#
#  Usage:
#    make
#    sudo make install

include ../config.mk

# lib name, version
LIBNAME=samba
VERSION=3.5.6
SOURCE=$(LIBNAME)-$(VERSION)
# download location and format
BASE_URL=http://mirrors.xbmc.org/build-deps/darwin-libs
#BASE_URL=http://www.samba.org/samba/ftp/stable
ARCHIVE=$(SOURCE).tar.gz

RETRIEVE_TOOL=/usr/bin/curl
RETRIEVE_TOOL_FLAGS=-Ls --output $(ARCHIVE)
ARCHIVE_TOOL=tar
ARCHIVE_TOOL_FLAGS=xf

# configuration settings
export CFLAGS+=-undefined dynamic_lookup
export CXXFLAGS+=-undefined dynamic_lookup
export LDFLAGS+=-Wl,-read_only_relocs,suppress -Wl,-unexported_symbol,_MD5* -lc
# believe it or not, this is how you setup samba for cross-compile
CONFIGURE=./configure --prefix=$(PREFIX) --host=$(HOST) --enable-shared=yes \
  --without-cluster-support --disable-swat --without-ldap --with-fhs \
  samba_cv_CC_NEGATIVE_ENUM_VALUES=yes samba_cv_HAVE_OFF64_T=no \
  samba_cv_HAVE_LINUX_READAHEAD=no \
  samba_cv_HAVE_STRUCT_FLOCK64=no \
  samba_cv_HAVE_LINUX_SPLICE=no \
  samba_cv_SIZEOF_DEV_T=no \
  samba_cv_SIZEOF_INO_T=yes \
  samba_cv_SIZEOF_OFF_T=yes \
  samba_cv_SIZEOF_TIME_T=no \
  samba_cv_have_longlong=yes \
  samba_cv_have_setresgid=no \
  samba_cv_have_setresuid=no \
  ac_cv_header_rpcsvc_ypclnt_h=no \
  ac_cv_func_creat64=no \
  ac_cv_func_fseek64=no \
  ac_cv_func_fseeko64=no \
  ac_cv_func_ftell64=no \
  ac_cv_func_ftello64=no \
  ac_cv_func_open64=no \
  ac_cv_func_fopen64=no \
  ac_cv_func_opendir64=no \
  ac_cv_func_readdir64=no \
  ac_cv_func_seekdir64=no \
  ac_cv_func_telldir64=no \
  ac_cv_func_rewinddir64=no \
  ac_cv_func_closedir64=no \
  ac_cv_func_getpwanam=no \
  ac_cv_func_getprpwnam=no \
  ac_cv_func_getpwent_r=no \
  ac_cv_func_getpassphrase=no \
  ac_cv_func_llistxattr=no \
  ac_cv_func_lremovexattr=no \
  ac_cv_func_lsetxattr=no \
  ac_cv_func_lgetxattr=no \
  ac_cv_func_extattr_delete_fd=no \
  ac_cv_func_extattr_delete_file=no \
  ac_cv_func_extattr_delete_link=no \
  ac_cv_func_extattr_get_fd=no \
  ac_cv_func_extattr_get_file=no \
  ac_cv_func_extattr_get_link=no \
  ac_cv_func_extattr_list_fd=no \
  ac_cv_func_extattr_list_file=no \
  ac_cv_func_extattr_list_link=no \
  ac_cv_func_extattr_set_fd=no \
  ac_cv_func_extattr_set_file=no \
  ac_cv_func_extattr_set_link=no \
  ac_cv_func_strndup=no \
  ac_cv_func_strnlen=no \
  ac_cv_have_splice_decl=no \
  ac_cv_func_yp_get_default_domain=no \
  fu_cv_sys_stat_statvfs64=no \
  libreplace_cv_HAVE_C99_VSNPRINTF=yes

LIBDYLIB=$(SOURCE)/source3/bin/libsmbclient.dylib

CLEAN_FILES=$(ARCHIVE) $(SOURCE)

all: $(LIBDYLIB) .installed

$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)
	echo $(ARCHIVE) > .gitignore

$(LIBDYLIB): $(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(ARCHIVE)
	echo $(SOURCE) >> .gitignore
	cd $(SOURCE)/source3; ./autogen.sh
	cd $(SOURCE)/source3; $(CONFIGURE)
	cp -f /Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator$(platform_sdk_version).sdk/usr/include/crt_externs.h $(PREFIX)/include/
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
	rm -rf $(SOURCE) .installed .gitignore
