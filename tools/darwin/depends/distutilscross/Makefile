include ../Makefile.include

# lib name, version
APPNAME=distutilscross
VERSION=0.1
SOURCE=$(APPNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
export PATH:=$(TOOLCHAIN)/bin:$(PATH)

APP=$(SOURCE)/bin/$(APPNAME)
APPBIN=$(TOOLCHAIN)/bin/$(APPNAME)

all: $(APPBIN)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore

$(APPBIN): $(SOURCE)
	cd $(SOURCE); $(TOOLCHAIN)/bin/python setup.py install --prefix=$(TOOLCHAIN)

clean:
	rm -rf $(SOURCE)

distclean::
	rm -rf $(SOURCE)
