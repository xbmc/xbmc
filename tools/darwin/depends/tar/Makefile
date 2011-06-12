include ../Makefile.include

# app name, version
APPNAME=tar
VERSION=1.23
SOURCE=$(APPNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

export LIBTOOL=builds/unix/libtool
export PATH:=$(TOOLCHAIN)/bin:$(PATH)
CONFIGURE=./configure --prefix=$(TOOLCHAIN) \
--program-transform-name=s/tar/gtar/ --disable-dependency-tracking

APP=$(SOURCE)/src/tar
APPBIN=$(TOOLCHAIN)/bin/tar

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
	rm -f $(TOOLCHAIN)/bin/tar $(TOOLCHAIN)/bin/gnutar
	cd $(TOOLCHAIN)/bin; ln -s gtar tar; ln -s gtar gnutar

clean:
	make -C $(SOURCE) clean

distclean::
	rm -rf $(SOURCE)

