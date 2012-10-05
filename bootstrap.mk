BOOTSTRAP_SUBDIRS += configure.in
BOOTSTRAP_SUBDIRS += xbmc/screensavers/rsxs-0.9/configure.ac
BOOTSTRAP_SUBDIRS += xbmc/visualizations/Goom/goom2k4-0/configure.in
BOOTSTRAP_SUBDIRS += lib/cpluff/configure.ac
BOOTSTRAP_SUBDIRS += lib/gtest/configure.ac

ifneq ($(wildcard lib/libdvd/libdvdcss/configure.ac),)
BOOTSTRAP_SUBDIRS += lib/libdvd/libdvdcss/configure.ac
DVD_CSS=lib/libdvd/libdvdcss/configure
endif
BOOTSTRAP_SUBDIRS += lib/libdvd/libdvdread/configure.ac
BOOTSTRAP_SUBDIRS += lib/libdvd/libdvdnav/configure.ac

ifneq ($(wildcard pvr-addons/Makefile.am),)
BOOTSTRAP_SUBDIRS += pvr-addons/configure.ac
endif

BOOTSTRAP_TARGETS=$(basename $(BOOTSTRAP_SUBDIRS))
all: $(BOOTSTRAP_TARGETS)

#preserve order for libdvd. dvdcss (if present) -> dvdread -> dvdnav.
lib/libdvd/libdvdread/configure: $(DVD_CSS)
lib/libdvd/libdvdnav/configure: lib/libdvd/libdvdread/configure

%: %.ac
	autoreconf -vif $(@D)
	-@rm -rf $(@D)/autom4te.cache

%: %.in
	autoreconf -vif $(@D)
	-@rm -rf $(@D)/autom4te.cache

configure: configure.in
	autoreconf -vif $(@D)
	-@rm -rf $(@D)/autom4te.cache
	@test -n "$$BOOTSTRAP_STANDALONE" || ( echo "Configuration is stale. You should almost certainly reconfigure" && false )

