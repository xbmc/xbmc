include ../Makefile.include
include ../config.site.mk

# lib name, version
LIBNAME=rtmpdump
VERSION=e0056c51cc1710c9a44d2a2c4e2f344fa9cabcf4
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings

LIBDYLIB=$(SOURCE)/.libs/$(LIBNAME).dylib

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)
	#git clone git://git.ffmpeg.org/rtmpdump $(SOURCE)
	#cd $(SOURCE); git archive --format=tar --prefix=$(SOURCE)/ $(VERSION) | gzip -9 > $(TARBALLS_LOCATION)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); patch -p1 < ../librtmp-60-second-fix.patch
	sed -i -e 's|CC=|#CC=|' $(SOURCE)/Makefile
	sed -i -e 's|LD=|#LD=|' $(SOURCE)/Makefile
	sed -i -e 's|CC=|#CC=|' $(SOURCE)/librtmp/Makefile
	sed -i -e 's|LD=|#LD=|' $(SOURCE)/librtmp/Makefile
	sed -i -e 's|AR=|#AR=|' $(SOURCE)/librtmp/Makefile
	sed -i -e 's|headerpad_max_install_names|headerpad_max_install_names -install_name \$$(libdir)/\$$@|' $(SOURCE)/librtmp/Makefile

$(LIBDYLIB): $(SOURCE)
	make SYS=darwin prefix=$(PREFIX) XCFLAGS="$(CFLAGS)" XLDFLAGS="$(LDFLAGS)" -j $(MAKE_JOBS) -C $(SOURCE)/librtmp

.installed:
	make SYS=darwin prefix=$(PREFIX) -C $(SOURCE)/librtmp install
	touch $@

clean:
	make SYS=darwin -C $(SOURCE) clean
	rm -f .installed

distclean::
	rm -rf $(SOURCE) .installed

