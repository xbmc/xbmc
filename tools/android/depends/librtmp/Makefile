include ../Makefile.include
DEPS= ../Makefile.include Makefile prefix.patch

# lib name, version
LIBNAME=rtmpdump
VERSION=e0056c51cc1710c9a44d2a2c4e2f344fa9cabcf4
GIT_DIR=$(TARBALLS_LOCATION)/$(LIBNAME).git
BASE_URL=git://git.ffmpeg.org/$(LIBNAME).git
SOURCE=$(LIBNAME)-$(VERSION)
#tell git to use the addons repo rather than xbmc's repo
export GIT_DIR
export GIT_WORK_TREE=$(PLATFORM)

# configuration settings

LIBDYLIB=$(PLATFORM)/librtmp/librtmp.a

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(GIT_DIR)/HEAD:
	cd $(TARBALLS_LOCATION); git clone --bare $(BASE_URL)

$(GIT_DIR)/current/$(VERSION): $(GIT_DIR)/HEAD $(DEPS)
	git rev-list -1 $(VERSION) >/dev/null || git fetch origin "+refs/heads/*:refs/remotes/origin/*"
	git rev-list -1 $(VERSION) >/dev/null
	rm -rf $(GIT_DIR)/current; mkdir -p $(GIT_DIR)/current
	touch $@

$(PLATFORM)/bootstrap: $(GIT_DIR)/current/$(VERSION)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	git checkout $(VERSION) -- .

$(PLATFORM): $(PLATFORM)/bootstrap
	cd $(PLATFORM); patch -p1 < ../librtmp-60-second-fix.patch
	cd $(PLATFORM); patch -p0 < ../prefix.patch

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/librtmp CROSS_COMPILE=$(CROSSTOOLS) PREFIX=$(PREFIX) XCFLAGS="$(CFLAGS)" XLDFLAGS="$(LDFLAGS) -lm"

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/librtmp install PREFIX=$(PREFIX)
	rm -f $(PREFIX)/lib/librtmp.la $(PREFIX)/lib/librtmp.so
	mv -f $(PREFIX)/lib/librtmp.so.0 $(PREFIX)/lib/librtmp.so
	$(RPL) -e "librtmp.so.0" "librtmp.so\x00\x00" $(PREFIX)/lib/librtmp.so
	-$(READELF) --dynamic $(PREFIX)/lib/librtmp.so | grep ibrary
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

