include ../../Makefile.include
include ../depends.mk

# lib name, version
LIBNAME=libnfs
VERSION=1.3.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=./configure CFLAGS=-D_FILE_OFFSET_BITS=64 --prefix=$(PREFIX) --host=$(HOST)

SO_NAME=$(SOURCE)/lib/libnfs.so

CLEAN_FILES=$(ARCHIVE) $(SOURCE)

all: $(SO_NAME) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	-rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); ./bootstrap
	cd $(SOURCE); $(CONFIGURE)

$(SO_NAME): $(SOURCE)
	make -C $(SOURCE)

.installed:
	make -C $(SOURCE) install
	touch $@
clean:
	make -C $(SOURCE) clean
	rm  -f .installed
	rm -rf $(SOURCE)

distclean::
	rm -rf $(SOURCE) .installed
