include ../../Makefile.include PCRE-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile PCRE-VERSION ../../download-files.include \
                                001-all-cmakeconfig.patch \
                                002-all-enable_docs_pc.patch \
                                003-all-postfix.patch \
                                004-win-pdb.patch \
                                jit_aarch64.patch \
                                ios-clear_cache.patch \
                                tvos-bitcode-fix.patch

# configuration settings
CMAKE_OPTIONS=-DPCRE_NEWLINE=ANYCRLF \
              -DPCRE_NO_RECURSE=ON \
              -DPCRE_MATCH_LIMIT_RECURSION=1500 \
              -DPCRE_SUPPORT_JIT=ON \
              -DPCRE_SUPPORT_PCREGREP_JIT=ON \
              -DPCRE_SUPPORT_UTF=ON \
              -DPCRE_SUPPORT_UNICODE_PROPERTIES=ON \
              -DPCRE_SUPPORT_LIBZ=OFF \
              -DPCRE_SUPPORT_LIBBZ2=OFF \
              -DPCRE_BUILD_PCRECPP=OFF \
              -DPCRE_BUILD_PCREGREP=OFF \
              -DPCRE_BUILD_TESTS=OFF \
              -DCMAKE_BUILD_TYPE=Release

ifeq ($(OS),android)
  # CMake CheckFunctionExists incorrectly detects strtoq for android
  CMAKE_OPTIONS+= -DHAVE_STRTOQ=0
endif

LIBDYLIB=$(PLATFORM)/build/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); mkdir -p build
	cd $(PLATFORM); patch -p1 -i ../001-all-cmakeconfig.patch
	cd $(PLATFORM); patch -p1 -i ../002-all-enable_docs_pc.patch
	cd $(PLATFORM); patch -p1 -i ../003-all-postfix.patch
	cd $(PLATFORM); patch -p1 -i ../004-win-pdb.patch
	cd $(PLATFORM); patch -p1 -i ../tvos-bitcode-fix.patch
	cd $(PLATFORM); patch -p1 -i ../jit_aarch64.patch
	cd $(PLATFORM); patch -p1 -i ../ios-clear_cache.patch
	cd $(PLATFORM)/build; $(CMAKE) $(CMAKE_OPTIONS) ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
