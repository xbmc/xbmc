include ../../Makefile.include PCRE2-VERSION ../../download-files.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile PCRE2-VERSION ../../download-files.include

CMAKE_OPTIONS := -DPCRE2_BUILD_PCRE2GREP=OFF \
                 -DPCRE2_BUILD_TESTS=OFF \
                 -DPCRE2_SUPPORT_LIBBZ2=OFF \
                 -DPCRE2_SUPPORT_LIBZ=OFF \
                 -DPCRE2_SUPPORT_LIBEDIT=OFF \
                 -DPCRE2_SUPPORT_LIBREADLINE=OFF \
                 -DPCRE2GREP_SUPPORT_JIT=OFF \
                 -DPCRE2GREP_SUPPORT_CALLOUT=OFF \
                 -DPCRE2GREP_SUPPORT_CALLOUT_FORK=OFF

LIBDYLIB=$(PLATFORM)/build/lib$(LIBNAME)-8.a

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM)/build; $(CMAKE_FOR_BUILD) $(CMAKE_OPTIONS) ..

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
