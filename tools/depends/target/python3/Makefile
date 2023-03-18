include ../../Makefile.include PYTHON3-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile PYTHON3-VERSION ../../download-files.include \
                                 apple.patch \
                                 crosscompile.patch \
                                 darwin_embedded.patch \
                                 10-android-modules.patch \
                                 10-linux-modules.patch \
                                 10-osx-modules.patch \
                                 modules.setup

ifeq ($(findstring apple-darwin, $(HOST)), apple-darwin)
  HOSTPLATFORM=_PYTHON_HOST_PLATFORM="darwin"
  LINK_ICONV=-liconv
  ifeq ($(OS), darwin_embedded)
    EXTRA_CONFIGURE=ac_cv_func_wait3=no ac_cv_func_wait4=no ac_cv_func_waitpid=no \
                    ac_cv_func_execv=no ac_cv_func_fexecv=no ac_cv_func_getentropy=no \
                    ac_cv_func_setpriority=no ac_cv_func_sendfile=no ac_cv_header_sched_h=no \
                    ac_cv_func_posix_spawn=no ac_cv_func_posix_spawnp=no \
                    ac_cv_func_forkpty=no ac_cv_lib_util_forkpty=no \
                    ac_cv_func_getgroups=no \
                    ac_cv_func_system=no
  endif
  # required for _localemodule
  EXTRA_CONFIGURE+= ac_cv_lib_intl_textdomain=yes
  # uses SDK ffi
  EXTRA_CONFIGURE+= --with-system-ffi
endif

ifeq ($(OS),android)
  LDFLAGS+= -liconv
endif

ifeq ($(OS), linux)
  EXTRA_CONFIGURE=ac_cv_pthread=yes
  ifeq ($(TARGET_PLATFORM),webos)
    LDFLAGS+= -liconv
  endif

endif

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) \
                      --disable-shared \
                      --without-ensurepip \
                      --disable-framework \
                      --without-pymalloc \
                      --enable-ipv6 \
                      --with-build-python=$(NATIVEPREFIX)/bin/python3 \
                      --with-system-expat=yes \
                      --disable-test-modules \
                      MODULE_BUILDTYPE=static \
                      $(EXTRA_CONFIGURE)

export LDFLAGS

LIBDYLIB=$(PLATFORM)/libpython$(PYTHON_VERSION).a

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../crosscompile.patch
	cd $(PLATFORM); patch -p1 -i ../apple.patch
ifeq ($(OS),darwin_embedded)
	cd $(PLATFORM); patch -p1 -i ../darwin_embedded.patch
endif
	cp modules.setup $(PLATFORM)/Modules/Setup

ifeq ($(OS),android)
	cd $(PLATFORM); patch -p1 -i ../10-android-modules.patch
endif
ifeq ($(OS),linux)
	cd $(PLATFORM); patch -p1 -i ../10-linux-modules.patch
endif
ifeq ($(OS),osx)
	cd $(PLATFORM); patch -p1 -i ../10-osx-modules.patch
endif

	cd $(PLATFORM); $(AUTORECONF)
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM) $(HOSTPLATFORM) CROSS_COMPILE_TARGET=yes libpython$(PYTHON_VERSION).a
	touch $@

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) $(HOSTPLATFORM) CROSS_COMPILE_TARGET=yes install
	find $(PREFIX)/lib/python$(PYTHON_VERSION) -type f -name "*.pyc" -delete
	touch $(LIBDYLIB)
	touch $@

clean:
	rm -rf .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
