include ../Makefile.include

# lib name, version
LIBNAME=Python
VERSION=2.6.5
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2

# configuration settings
export OPT=$(CFLAGS)
CONFIGURE=./configure --prefix=$(PREFIX) \
  --enable-shared --disable-toolbox-glue --enable-unicode=ucs4

LIBDYLIB=$(SOURCE)/libpython2.6.dylib

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(LIBDYLIB): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); $(CONFIGURE)
	# python2.6 has an issue detecting and using the same version of openssl in configure and setup.py
	# this forces python2.6 hashlib to be compatible with osx 10.4 boxes.
	sed -ie "s|openssl_ver >= 0x00907000)|openssl_ver >= 0x00907000 and False)|" "$(SOURCE)/setup.py"
	sed -ie "s|(openssl_ver < 0x00908000)|(True or openssl_ver < 0x00908000)|" "$(SOURCE)/setup.py"
	cd $(SOURCE); make

.installed:
	cd $(SOURCE); make install
	find $(PREFIX)/lib/python2.6 -type f -name *.pyc -exec rm -f {} \;	
	find $(PREFIX)/lib/python2.6 -type f -name *.pyo -exec rm -f {} \;	
	touch $@

clean:
	rm -rf $(SOURCE) .installed

distclean::
	rm -rf $(SOURCE) .installed

