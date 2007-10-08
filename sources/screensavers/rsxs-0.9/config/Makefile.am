## Process this file with automake to produce Makefile.in

XML = @ENABLED_CONFIGS@
EXTRA_DIST = rs-plasma.xml rs-cyclone.xml rs-fieldlines.xml rs-helios.xml \
		rs-euphoria.xml rs-lattice.xml rs-flocks.xml rs-flux.xml \
		rs-solarwinds.xml rs-hyperspace.xml

if HACKMODE
if XSCREENSAVER4
dist_config_DATA = $(XML)
else !XSCREENSAVER4
dist_noinst_DATA = $(XML)
endif !XSCREENSAVER4
else !HACKMODE
dist_noinst_DATA = $(XML)
endif !HACKMODE
