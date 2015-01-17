## Process this file with automake to produce Makefile.in

SUBDIRS = $(ENABLED_HACKS)
DIST_SUBDIRS = \
	cyclone euphoria fieldlines flocks flux helios hyperspace lattice \
	plasma skyrocket solarwinds

AM_ETAGSFLAGS = --extra=fq

uninstall-hook:
	-rmdir $(pkgdatadir)

# Workaround for Exuberant ctags failure when no filenames are specified.
ETAGS = echo | etags
ETAGS_ARGS = -L -

# See Makefile.common for why this is here.
distclean: distclean-recursive
	-rm -rf ./$(DEPDIR)
	-rm -f Makefile
