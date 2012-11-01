include ../../Makefile.include
include ../depends.mk

# lib name, version
LIBNAME=tinyxml
VERSION=2.6.2
SOURCE=$(LIBNAME)
ARCHIVE=$(LIBNAME)_2_6_2.tar.gz
BASE_URL=http://sourceforge.net/projects/tinyxml/files/tinyxml/$(VERSION)

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --host=$(HOST)

LIBDYLIB=$(SOURCE)/.libs/lib$(LIBNAME).so

CLEAN_FILES=$(ARCHIVE) $(SOURCE)

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(SOURCE); patch -p1 -i ../enforce-use-stl.patch
	cd $(SOURCE); patch -p1 -i ../entity.patch
	cd $(SOURCE); patch -p1 -i ../makefix.patch
	cd $(SOURCE); sed -i -e '/^TINYXML_USE_STL/ s|=.*|=YES|' -e "s|^RELEASE_CFLAGS.*|& $(CXXFLAGS) -fPIC|" Makefile
	echo $(SOURCE) > .gitignore

$(LIBDYLIB): $(SOURCE)
	make -j $(JOBS) -C $(SOURCE) CFLAGS="$(CFLAGS) -DTIXML_USE_STL" CXXFLAGS="$(CXXFLAGS) -DTIXML_USE_STL" LDFLAGS="$(LDFLAGS)" AR="$(AR)" RANLIB="$(RANLIB)" CC="$(CC)" CXX="$(CXX)" LD="$(LD)" LIBS="-ldl -lc -lstdc++"
	cd $(SOURCE); $(CXX) -fPIC $(CXXFLAGS) $(LDFLAGS) -shared -o lib$(LIBNAME).so.$(VERSION) -Wl,-soname,lib$(LIBNAME).so.0 *.o 

.installed:
	install -m 0644 $(SOURCE)/tinyxml.h $(PREFIX)/include/
	install -m 0644 $(SOURCE)/tinystr.h $(PREFIX)/include/
	install -m 0755 $(SOURCE)/libtinyxml.so.$(VERSION) $(PREFIX)/lib
	cd $(PREFIX)/lib; rm -f libtinyxml.so; ln -s libtinyxml.so.$(VERSION) libtinyxml.so
	cd $(PREFIX)/lib; rm -f libtinyxml.so.0; ln -s libtinyxml.so.$(VERSION) libtinyxml.so.0
	touch $@

clean:
	make -C $(SOURCE) clean
	rm -f .installed

distclean::
	rm -rf $(SOURCE) .installed
