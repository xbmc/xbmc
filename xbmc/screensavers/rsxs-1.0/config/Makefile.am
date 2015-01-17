## Process this file with automake to produce Makefile.in

EXTRA_DIST = \
	rs-plasma.xml     rs-plasma.conf     \
	rs-cyclone.xml    rs-cyclone.conf    \
	rs-fieldlines.xml rs-fieldlines.conf \
	rs-helios.xml     rs-helios.conf     \
	rs-euphoria.xml   rs-euphoria.conf   \
	rs-lattice.xml    rs-lattice.conf    \
	rs-flocks.xml     rs-flocks.conf     \
	rs-flux.xml       rs-flux.conf       \
	rs-solarwinds.xml rs-solarwinds.conf \
	rs-hyperspace.xml rs-hyperspace.conf

dist_noinst_DATA =

if HACKMODE

if XSCREENSAVER_MODULAR
dist_module_DATA = $(ENABLED_MODULES)

update-xscreensaver:
	test -x $(XSCREENSAVER_UPDATER) && \
		$(XSCREENSAVER_UPDATER) || \
			echo "*** Could not add hacks to XScreenSaver configuration" >&2

install-data-hook: update-xscreensaver
uninstall-hook: update-xscreensaver

else !XSCREENSAVER_MODULAR
dist_noinst_DATA += $(ENABLED_MODULES)

install-data-hook: $(top_srcdir)/scripts/update-rsxs-hacks
	if test -f $(defaultdir)/XScreenSaver; then \
		TMPFILE=`mktemp /tmp/$$$$.XXXXXX` || exit 1 && \
		trap "rm -f $$TMPFILE" 0 && \
		$(SHELL) $(top_srcdir)/scripts/update-rsxs-hacks $(ENABLED_HACKS) < $(defaultdir)/XScreenSaver > $$TMPFILE && \
		$(mkinstalldirs) $(DESTDIR)$(defaultdir) && \
		$(install_sh_DATA) $$TMPFILE $(DESTDIR)$(defaultdir)/XScreenSaver; \
	fi

uninstall-hook: $(top_srcdir)/scripts/update-rsxs-hacks
	if test -f $(defaultdir)/XScreenSaver; then \
		TMPFILE=`mktemp /tmp/$$$$.XXXXXX` || exit 1 && \
		trap "rm -f $$TMPFILE" 0 && \
		$(SHELL) $(top_srcdir)/scripts/update-rsxs-hacks < $(defaultdir)/XScreenSaver > $$TMPFILE && \
		$(mkinstalldirs) $(DESTDIR)$(defaultdir) && \
		$(install_sh_DATA) $$TMPFILE $(defaultdir)/XScreenSaver; \
	fi

endif !XSCREENSAVER_MODULAR

if XSCREENSAVER_CONFIGURABLE
dist_config_DATA = $(ENABLED_CONFIGS)
else !XSCREENSAVER_CONFIGURABLE
dist_noinst_DATA += $(ENABLED_CONFIGS)
endif !XSCREENSAVER_CONFIGURABLE

else !HACKMODE

dist_noinst_DATA += $(ENABLED_MODULES) $(ENABLED_CONFIGS)

endif !HACKMODE
