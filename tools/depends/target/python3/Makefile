include ../../Makefile.include PYTHON3-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile PYTHON3-VERSION ../../download-files.include \
                                 apple.patch \
                                 10-android-modules.patch \
                                 11-android-ctypes.patch

ifeq ($(findstring apple-darwin, $(HOST)), apple-darwin)
  ifeq ($(OS), darwin_embedded)
    EXTRA_CONFIGURE=ac_cv_func_wait3=no ac_cv_func_wait4=no ac_cv_func_waitpid=no \
                    ac_cv_func_execv=no ac_cv_func_fexecv=no ac_cv_func_getentropy=no \
                    ac_cv_func_setpriority=no ac_cv_func_sendfile=no ac_cv_header_sched_h=no \
                    ac_cv_func_posix_spawn=no ac_cv_func_posix_spawnp=no \
                    ac_cv_func_forkpty=no ac_cv_lib_util_forkpty=no \
                    ac_cv_func_getgroups=no \
                    ac_cv_func_system=no
    export SDKROOT
  endif
  ifeq ($(OS), osx)
    export SDKROOT
  endif
  # required for _localemodule
  EXTRA_CONFIGURE+= ac_cv_lib_intl_textdomain=yes
endif

ifeq ($(OS), linux)
  EXTRA_CONFIGURE=ac_cv_pthread=yes
  ifeq ($(TARGET_PLATFORM),webos)
    # Force intl check to succeed
    EXTRA_CONFIGURE+=ac_cv_lib_intl_textdomain=yes
  endif
endif

# Disabled c extension modules for all platforms
PY_MODULES = py_cv_module_grp=n/a \
             py_cv_module_syslog=n/a \
             py_cv_module__dbm=n/a \
             py_cv_module__gdbm=n/a \
             py_cv_module__uuid=n/a \
             py_cv_module_readline=n/a \
             py_cv_module__curses=n/a \
             py_cv_module__curses_panel=n/a \
             py_cv_module_xx=n/a \
             py_cv_module_xxlimited=n/a \
             py_cv_module_xxlimited_35=n/a \
             py_cv_module_xxsubtype=n/a \
             py_cv_module__xxsubinterpreters=n/a \
             py_cv_module__tkinter=n/a \
             py_cv_module__curses=n/a \
             py_cv_module__codecs_jp=n/a \
             py_cv_module__codecs_kr=n/a \
             py_cv_module__codecs_tw=n/a

# These modules use "internal" libs for building. The required static archives
# are not installed outside of the cpython build tree, and cause failure in kodi linking
# If we wish to support them in the future, we should create "system libs" for them
PY_MODULES+= py_cv_module__decimal=n/a \
             py_cv_module__sha2=n/a


ifeq ($(OS), darwin_embedded)
  PY_MODULES+= py_cv_module__posixsubprocess=n/a \
               py_cv_module__scproxy=n/a
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
                      $(PY_MODULES) \
                      $(EXTRA_CONFIGURE)

# Use LIBS as some platforms (webos) require iconv to be late on the linker line
# appending to LDFLAGS puts the iconv link argument too early, causing link failures
export LIBS=$(LINK_ICONV)

LIBDYLIB=$(PLATFORM)/libpython$(PYTHON_VERSION).a

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../apple.patch

ifeq ($(OS),android)
	cd $(PLATFORM); patch -p1 -i ../10-android-modules.patch
	cd $(PLATFORM); patch -p1 -i ../11-android-ctypes.patch
endif
	cd $(PLATFORM); $(AUTORECONF)
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM) libpython$(PYTHON_VERSION).a
	touch $@

.installed-$(PLATFORM): $(LIBDYLIB)
# We specifically use -j1 as some threading issues can occur with install directory creation
	$(MAKE) -C $(PLATFORM) $(HOSTPLATFORM) install -j1
	find $(PREFIX)/lib/python$(PYTHON_VERSION) -type f -name "*.pyc" -delete
	find $(PREFIX)/lib/python$(PYTHON_VERSION) -type d -name "config-$(PYTHON_VERSION)-*" -exec rm -r {} +
	touch $(LIBDYLIB)
	touch $@

clean:
	rm -rf .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
