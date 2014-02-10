include ../../Makefile.include
DEPS= ../../Makefile.include add-arm-mem-barrier.patch fix-deprecated-swp.patch fix_shared_ptr_multithread.patch Makefile

#Headers Only!
LIBNAME=boost
VERSION=1_44_0
SOURCE=$(LIBNAME)_$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo 'using gcc : : $(CC) : ' > $(PLATFORM)/tools/build/v2/user-config.jam
	echo '	<cflags>"$(CFLAGS)"' >> $(PLATFORM)/tools/build/v2/user-config.jam
	echo '  <cxxflags>"$(CXXFLAGS)"' >> $(PLATFORM)/tools/build/v2/user-config.jam
	echo '	;' >> $(PLATFORM)/tools/build/v2/user-config.jam
	cd $(PLATFORM); patch -p1 < ../add-arm-mem-barrier.patch
	cd $(PLATFORM); patch -p1 < ../fix-deprecated-swp.patch
ifeq (arm, $(findstring arm, $(CPU)))
	cd $(PLATFORM); patch -p0 < ../fix_shared_ptr_multithread.patch
endif
	cd $(PLATFORM); patch -p1 < ../fix-self-assignment-warnings.patch
	cd $(PLATFORM); ./bootstrap.sh --prefix=$(PREFIX)

.installed-$(PLATFORM): $(PLATFORM)
	cd $(PLATFORM); ./bjam --prefix=$(PREFIX) toolset=gcc --toolset-root=$(TOOLCHAIN)/$(HOST)/bin --disable-icu \
	--without-date_time --without-filesystem --without-graph --without-graph_parallel --without-iostreams --without-math --without-mpi --without-program_options \
--without-python --without-random --without-regex --without-serialization --without-signals --without-system --without-test --without-thread --without-wave install 
	touch .installed-$(PLATFORM)

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
