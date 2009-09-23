## Process this file with automake to produce Makefile.in

SUBDIRS = $(ENABLED_HACKS)
DIST_SUBDIRS = \
	cyclone euphoria fieldlines flocks flux helios hyperspace lattice \
	plasma skyrocket solarwinds

AM_ETAGSFLAGS = --extra=fq

if HACKMODE
if SYMLINKS
install-exec-hook:
	${mkinstalldirs} $(bindir)
	for i in $(ENABLED_BINARIES); do \
		$(LN_S) -f $(hackreldir)/$$i $(DESTDIR)$(bindir)/$$i; \
	done
endif SYMLINKS

uninstall-hook:
	for i in $(ENABLED_BINARIES); do rm -f $(bindir)/$$i; done
	-rmdir $(pkgdatadir)

else !HACKMODE
install-exec-hook:
uninstall-hook:
endif !HACKMODE

# Workaround for Exuberant ctags failure when no filenames are specified.
ETAGS = echo | etags
ETAGS_ARGS = -L -

# See Makefile.common for why this is here.
distclean: distclean-recursive
	-rm -rf ./$(DEPDIR)
	-rm -f Makefile
