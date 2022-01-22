include ../../Makefile.include
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile ../../download-files.include

APPNAME=cmake
VERSION=3.21.3
SOURCE=$(APPNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=0571b78443906c5ad51fb0fafbd32d565caf628cc150b1190802cb819e8497c108ea6b7ecaa03133df2dbbceb730696d24b4df38468c92088c769ce4076d9e9f
include ../../download-files.include

# configuration settings

SETENV=CC="$(CC_FOR_BUILD)" CXX="$(CXX_FOR_BUILD)" LD=$(LD_FOR_BUILD) CFLAGS="$(NATIVE_CFLAGS)" \
	CXXFLAGS="$(NATIVE_CXXFLAGS)" LDFLAGS="$(NATIVE_LDFLAGS)"

ifeq ($(NATIVE_OS), osx)
  SETENV+=SDKROOT=$(shell xcrun --show-sdk-path)
endif

CONFIGURE=./bootstrap --prefix=$(NATIVEPREFIX) --system-curl
ifeq ($(USE_CCACHE), yes)
	CONFIGURE+=--enable-ccache
endif

APP=$(PLATFORM)/bin/$(APPNAME)

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	-rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(SETENV) $(CONFIGURE)

$(APP): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)
	touch $@

.installed-$(PLATFORM): $(APP)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

