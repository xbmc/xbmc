include ../Makefile.include

# lib name, version
APPNAME=autoconf
VERSION=2.63
SOURCE=$(APPNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
export PATH:=$(TOOLCHAIN)/bin:$(PATH)
CONFIGURE=./configure --prefix=$(TOOLCHAIN)

APP=$(SOURCE)/bin/$(APPNAME)
APPBIN=$(TOOLCHAIN)/bin/$(APPNAME)

all: $(APPBIN)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	sed -i bak -e "s/'libtoolize'/'glibtoolize'/" $(SOURCE)/bin/autoreconf.in
	cd $(SOURCE); $(CONFIGURE)

$(APP): $(SOURCE)
	make -C $(SOURCE)

$(APPBIN):
	make $(APP)
	make -C $(SOURCE) install

clean:
	make -C $(SOURCE) clean

distclean::
	rm -rf $(SOURCE)
