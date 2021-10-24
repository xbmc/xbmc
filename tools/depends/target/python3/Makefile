include ../../Makefile.include
DEPS= ../../Makefile.include Makefile apple.patch crosscompile.patch android.patch \
       modules.setup darwin_embedded.patch 01-GH-27394-move-crypt-h.patch

# lib name, version
LIBNAME=Python
VERSION=3.9.7
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz
BASE_URL=https://www.python.org/ftp/python/$(VERSION)

ifeq ($(findstring apple-darwin, $(HOST)), apple-darwin)
  HOSTPLATFORM=_PYTHON_HOST_PLATFORM="darwin"
  LINK_ICONV=-liconv
  ifeq ($(OS), darwin_embedded)
    EXTRA_CONFIGURE=ac_cv_func_wait3=no ac_cv_func_wait4=no ac_cv_func_waitpid=no \
                    ac_cv_func_execv=no ac_cv_func_fexecv=no ac_cv_func_getentropy=no \
                    ac_cv_func_setpriority=no ac_cv_func_sendfile=no ac_cv_header_sched_h=no \
                    ac_cv_func_posix_spawn=no ac_cv_func_posix_spawnp=no \
                    ac_cv_func_forkpty=no ac_cv_lib_util_forkpty=no \
                    ac_cv_func_getgroups=no
  endif
endif

ifeq ($(OS), linux)
  EXTRA_CONFIGURE=ac_cv_pthread=yes
endif

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) \
                      --disable-shared \
                      --without-ensurepip \
                      --disable-framework \
                      --without-pymalloc \
                      --enable-ipv6 \
                      $(EXTRA_CONFIGURE)


LIBDYLIB=$(PLATFORM)/libpython$(PYTHON_VERSION).a

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../crosscompile.patch
	cd $(PLATFORM); patch -p1 -i ../android.patch
	cd $(PLATFORM); patch -p1 -i ../apple.patch
	cd $(PLATFORM); patch -p1 -i ../01-GH-27394-move-crypt-h.patch
ifeq ($(OS),darwin_embedded)
	cd $(PLATFORM); patch -p1 -i ../darwin_embedded.patch
endif
	cp modules.setup $(PLATFORM)/Modules/Setup

#Add -liconv as needed, and add the _scproxy module for darwin
#  disable locale altogether for Android
ifeq ($(OS),android)
	cd $(PLATFORM); sed -ie 's|_locale -DPy_BUILD_CORE_BUILTIN _localemodule.c -lintl \$$(LINK_ICONV)|#_locale -DPy_BUILD_CORE_BUILTIN _localemodule.c -lintl \$$(LINK_ICONV)|' Modules/Setup
endif
ifeq ($(OS),linux)
	# _posixshmem module requires librt for linux for SHM_OPEN/SHM_UNLINK
	cd $(PLATFORM); sed -ie 's|# -lrt # _posixshmem|-lrt # _posixshmem|' Modules/Setup
endif
ifeq ($(OS),osx)
	cd $(PLATFORM); sed -ie 's|\$$(OSX_SCPROXY)|_scproxy \$$(srcdir)/Modules/_scproxy.c -framework SystemConfiguration -framework CoreFoundation|' Modules/Setup
endif

	cd $(PLATFORM); $(AUTORECONF)
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM) $(HOSTPLATFORM) PYTHON_FOR_BUILD=$(NATIVEPREFIX)/bin/python3 CROSS_COMPILE_TARGET=yes libpython$(PYTHON_VERSION).a
	touch $@

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) $(HOSTPLATFORM) PYTHON_FOR_BUILD=$(NATIVEPREFIX)/bin/python3 CROSS_COMPILE_TARGET=yes install
	find $(PREFIX)/lib/python$(PYTHON_VERSION) -type f -name "*.pyc" -delete
	touch $(LIBDYLIB)
	touch $@

clean:
	rm -rf .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
