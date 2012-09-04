include ../../Makefile.include

APPNAME=cmake
VERSION=2.8.4
SOURCE=$(APPNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
PREFIX:=$(XBMCPREFIX)
export PATH:=$(PREFIX)/bin:$(PATH)
CONFIGURE=./bootstrap --prefix=$(PREFIX)

APP=$(SOURCE)/$(APPNAME)

CLEAN_FILES=$(ARCHIVE) $(SOURCE)

all: $(APP) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	-rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); $(CONFIGURE)

$(APP): $(SOURCE)
	make -j $(JOBS) -C $(SOURCE)

.installed:
	make -C $(SOURCE) install
	touch $@

clean:
	make -C $(SOURCE) clean
	rm -f .installed

distclean::
	rm -rf $(SOURCE) .installed

