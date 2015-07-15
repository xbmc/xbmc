include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

#Headers Only!
LIBNAME=boost
VERSION=1_57_0
SOURCE=$(LIBNAME)_$(VERSION)
ARCHIVE=$(SOURCE)_stripped.tar.bz2

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
ifeq (arm, $(findstring arm, $(CPU)))
	#ugly hack to avoid issue with line endings. boost headers use \r\n and patch
	#will be converted by git to \n and fail to apply
	cd $(PLATFORM); echo "#define BOOST_SP_USE_SPINLOCK" >> boost/config/user.hpp
endif
	cd $(PLATFORM); cp -r boost $(PREFIX)/include/

.installed-$(PLATFORM): $(PLATFORM)
	cd $(PLATFORM); touch .installed-$(PLATFORM)

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
