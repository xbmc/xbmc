## Process this file with automake to produce Makefile.in

EXTRA_DIST = update-rsxs-hacks.m4sh
noinst_SCRIPTS = update-rsxs-hacks
MAINTAINERCLEANFILES = update-rsxs-hacks.in

$(srcdir)/update-rsxs-hacks.in: $(srcdir)/update-rsxs-hacks.m4sh
	$(AUTOM4TE) --language=m4sh --output=$@ $<
