include ../Makefile.include
include ../config.site.mk

LIBNAME=boost
VERSION=1_44_0
LIBVERSION=1.44.0
SOURCE=$(LIBNAME)_$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2

bjam_args = toolset=darwin-$(platform_gcc_version)
boost_flags = $(platform_sdk_path) -arch $(ARCH)
ifeq ("$(DARWIN)", "ios")
  bjam_args += --architecture=arm target-os=iphone macosx-version=iphone-$(platform_sdk_version) define=_LITTLE_ENDIAN
  # arm uses non-thread-safe compare-and-swap instruction so use posix thread primitives
  boost_flags += -DBOOST_AC_USE_PTHREADS -DBOOST_SP_USE_PTHREADS
else
  ifeq ("$(ARCH)", "ppc")
    bjam_args +=  --architecture=$(boost_architecture)=power address-model=32
  else ifeq ("$(ARCH)", "i386")
    bjam_args +=  --architecture=$(boost_architecture)=x86 address-model=32
  else
    bjam_args +=  --architecture=$(boost_architecture)=x86 address-model=64
  endif
  bjam_args += target-os=darwin --link=static
endif

all: .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

.installed: $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	echo 'using darwin : $(platform_gcc_version) : $(platform_cxx) :' >> $(SOURCE)/tools/build/v2/user-config.jam
	echo '  <cflags>"-isysroot $(boost_flags) -fvisibility=default -fvisibility-inlines-hidden"' >> $(SOURCE)/tools/build/v2/user-config.jam
	echo '  <cxxflags>"-isysroot $(boost_flags) -fvisibility=default -fvisibility-inlines-hidden"' >> $(SOURCE)/tools/build/v2/user-config.jam
	echo '  <linkflags>"-Wl,-syslibroot,$(boost_flags)"' >> $(SOURCE)/tools/build/v2/user-config.jam
	echo ';' >> $(SOURCE)/tools/build/v2/user-config.jam
	cd $(SOURCE); patch -p1 < ../add-arm-mem-barrier.patch
	cd $(SOURCE); patch -p1 < ../fix-deprecated-swp.patch
	cd $(SOURCE); ./bootstrap.sh --prefix=$(PREFIX) --with-libraries=thread,date_time
	cd $(SOURCE); ./bjam --prefix=$(PREFIX) $(bjam_args) install
	# boost builds .so instead of .dylib for iphone, fix it.
	if test "$(DARWIN)" = "ios"; then \
		mv $(PREFIX)/lib/libboost_thread.so.$(LIBVERSION) $(PREFIX)/lib/libboost_thread.dylib; \
		rm $(PREFIX)/lib/libboost_thread.so; \
		mv $(PREFIX)/lib/libboost_date_time.so.$(LIBVERSION) $(PREFIX)/lib/libboost_date_time.dylib; \
		rm $(PREFIX)/lib/libboost_date_time.so; \
	fi
	install_name_tool -id $(PREFIX)/lib/libboost_thread.dylib $(PREFIX)/lib/libboost_thread.dylib
	install_name_tool -id $(PREFIX)/lib/libboost_date_time.dylib $(PREFIX)/lib/libboost_date_time.dylib
	touch .installed

clean:
	rm -rf $(SOURCE) .installed

distclean::
	rm -rf $(SOURCE) .installed

