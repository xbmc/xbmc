include ../Makefile.include

LIBNAME=mysql
VERSION=5.1.55
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX)  \
  --enable-thread-safe-client -with-extra-charsets=complex \
  --without-readline --without-server --without-bench --without-docs --without-man

LIBDYLIB=$(SOURCE)/libmysql/.libs/$(LIBNAME).dylib

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	if test "$(DARWIN)" = "ios"; then \
		cd $(SOURCE); patch -p1 < ../01-mysqlclient-cross-compile.patch; \
		autoreconf -vif; \
	fi
	cd $(SOURCE); $(CONFIGURE)
	if test "$(DARWIN)" = "ios"; then \
		sed -i "" -e "s|#define HAVE_CURSES_H 1|/* & */|" \
			-e "s|#define HAVE_FDATASYNC 1|/* & */|" \
			-e "s|#define HAVE_TERMCAP_H 1|/* & */|" \
			-e "s|#define HAVE_TERM_H 1|/* & */|" \
			$(SOURCE)/include/config.h; \
		cd $(SOURCE); patch -p1 < ../02-mysqlclient-ios.patch; \
	fi

$(LIBDYLIB): $(SOURCE)
	make -j $(MAKE_JOBS) -C $(SOURCE)

.installed:
	make -C $(SOURCE)/scripts install
	make -C $(SOURCE)/include install
	make -C $(SOURCE)/libmysql install
	touch $@

clean:
	make -C $(SOURCE) clean
	rm -f .installed

distclean::
	rm -rf $(SOURCE) .installed
