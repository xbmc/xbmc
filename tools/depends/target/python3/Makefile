include ../../Makefile.include
DEPS= ../../Makefile.include Makefile apple.patch crosscompile.patch android.patch \
       modules.setup darwin_embedded.patch

# lib name, version
LIBNAME=Python
VERSION=3.8.8
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz

ifeq ($(findstring apple-darwin, $(HOST)), apple-darwin)
  LDFLAGS+= -framework Foundation
  ifeq ($(OS), darwin_embedded)
    EXTRA_CONFIGURE=ac_cv_func_wait3=no ac_cv_func_wait4=no ac_cv_func_waitpid=no \
                    ac_cv_func_execv=no ac_cv_func_fexecv=no ac_cv_func_getentropy=no \
                    ac_cv_func_setpriority=no ac_cv_func_sendfile=no ac_cv_header_sched_h=no \
                    ac_cv_func_posix_spawn=no ac_cv_func_posix_spawnp=no \
                    ac_cv_func_forkpty=no ac_cv_lib_util_forkpty=no \
                    ac_cv_func_getgroups=no
  endif
endif

export PATH:=$(NATIVEPREFIX)/bin:$(PATH)
export LINK_ICONV:=$(LINK_ICONV)

CONFIGURE=./configure \
            --prefix=$(PREFIX) \
            --disable-shared \
            --host=$(HOST) \
            --build=$(BUILD) \
            --enable-ipv6 \
            --without-ensurepip \
            --with-system-ffi \
            --with-system-expat \
            --without-pymalloc \
            CC="$(CC)" \
            CXX="$(CXX)" \
            PKG_CONFIG_LIBDIR=$(PREFIX)/lib/pkgconfig \
            LDFLAGS="$(LDFLAGS)" \
            CFLAGS="$(CFLAGS)" \
            CPPFLAGS="$(CPPFLAGS)" \
            $(EXTRA_CONFIGURE)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../crosscompile.patch
	cd $(PLATFORM); patch -p1 -i ../apple.patch
ifeq ($(OS),darwin_embedded)
	cd $(PLATFORM); patch -p1 -i ../darwin_embedded.patch
endif
	cp modules.setup $(PLATFORM)/Modules/Setup
#  disable locale altogether for Android
ifeq ($(OS),android)
	cd $(PLATFORM); sed -ie 's|_locale -DPy_BUILD_CORE_BUILTIN _localemodule.c -lintl $(LINK_ICONV)|#_locale -DPy_BUILD_CORE_BUILTIN _localemodule.c -lintl $(LINK_ICONV)|' Modules/Setup
endif
# add scproxy module for osx only
ifeq ($(OS),osx)
	cd $(PLATFORM); sed -ie 's|\$$(OSX_SCPROXY)|_scproxy \$$(srcdir)/Modules/_scproxy.c -framework SystemConfiguration -framework CoreFoundation|' Modules/Setup
endif
	cd $(PLATFORM); $(AUTORECONF)
	cd $(PLATFORM); $(CONFIGURE)

.installed-$(PLATFORM): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)
	$(MAKE) -C $(PLATFORM) install prefix=$(PREFIX)
	find $(PREFIX)/lib/python$(PYTHON_VERSION) -type f -name "*.pyc" -delete
	touch $@

clean:
	rm -rf .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
