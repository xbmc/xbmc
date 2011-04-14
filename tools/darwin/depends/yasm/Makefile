include ../Makefile.include

# lib name, version
APPNAME=yasm
VERSION=1.1.0
SOURCE=$(APPNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
export PATH:=$(TOOLCHAIN)/bin:$(PATH)
CONFIGURE=./configure --prefix=$(TOOLCHAIN)

APP=$(SOURCE)/$(APPNAME)

all: $(APP) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); $(CONFIGURE)

$(APP): $(SOURCE)
	make -C $(SOURCE)

.installed:
	make -C $(SOURCE) install
	rm -f $(PREFIX)/bin/yasm
	ln -s $(TOOLCHAIN)/bin/yasm $(PREFIX)/bin/yasm
	touch $@

clean:
	make -C $(SOURCE) clean
	rm -f .installed $(TOOLCHAIN)/bin/yasm $(PREFIX)/bin/yasm 

distclean::
	rm -rf $(SOURCE) .installed $(TOOLCHAIN)/bin/yasm $(PREFIX)/bin/yasm

