include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=Parse-Yapp
VERSION=1.21
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=dbf6182d4813ff7e355ea1713c748bfdf8290040a93f123acec645c7a1733fe457ab6e0ab51c4ec83cf82bc43d7fb35cbf89875df7b5c2ffc9635e85458cfeee
include ../../download-files.include

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX)

# clear env vars in case already active on system
export PERL_MB_OPT=
export PERL_MM_OPT=

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	-rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

.installed-$(PLATFORM): $(PLATFORM)
	cd $(PLATFORM); perl Makefile.PL PREFIX=$(NATIVEPREFIX)
	cd $(PLATFORM); $(MAKE)
	cd $(PLATFORM); $(MAKE) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
