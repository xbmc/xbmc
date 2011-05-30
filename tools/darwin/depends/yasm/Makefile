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
APPBIN=$(TOOLCHAIN)/bin/$(APPNAME)
YASM=$(PREFIX)/bin/$(APPNAME)

all: $(APPBIN) $(YASM)

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

$(YASM): $(APPBIN)
	ln -s $(APPBIN) $(YASM)

clean:
	make -C $(SOURCE) clean
	rm -f $(APPBIN) $(YASM)

distclean::
	rm -rf $(SOURCE) $(APPBIN) $(YASM)

