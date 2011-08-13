include ../Makefile.include

# lib name, version
APPNAME=libtool
VERSION=2.2.6
SOURCE=$(APPNAME)-$(VERSION)
ARCHIVE=$(SOURCE)a.tar.gz

# configuration settings
export PATH:=$(TOOLCHAIN)/bin:$(PATH)
CONFIGURE=./configure --prefix=$(TOOLCHAIN) \
  --program-prefix=g

APP=$(SOURCE)/bin/$(APPNAME)
APPBIN=$(TOOLCHAIN)/bin/glibtool

all: $(APPBIN)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); $(CONFIGURE)

$(APP): $(SOURCE)
	make -j $(MAKE_JOBS) -C $(SOURCE)

$(APPBIN):
	make $(APP)
	make -C $(SOURCE) install

clean:
	make -C $(SOURCE) clean

distclean::
	rm -rf $(SOURCE)
