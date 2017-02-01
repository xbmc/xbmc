## Process this file with automake to produce Makefile.in.

# Copyright 2007 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = libcpluff loader console po test docsrc doc
DIST_SUBDIRS = $(SUBDIRS) examples

EXTRA_DIST = COPYRIGHT.txt INSTALL.txt ChangeLog.txt Roadmap.txt autogen.sh plugin.xsd

doc_DATA = COPYRIGHT.txt ChangeLog.txt Roadmap.txt

doc:
	cd libcpluff && $(MAKE) $(AM_MAKEFLAGS) $@

examples: all
	cd examples && $(MAKE) $(AM_MAKEFLAGS) all LIBS='$(CURDIR)/libcpluff/libcpluff.la'

examples-install: install
	cd examples && $(MAKE) $(AM_MAKEFLAGS) install LIBS='$(CURDIR)/libcpluff/libcpluff.la'

dist-hook:
	rm -f '$(distdir)/ABOUT-NLS'

distcheck-hook:
	files="`cd '$(srcdir)' && find console libcpluff loader -type f -name '*.[ch]' -exec grep -q '_(' '{}' \; -print`"; \
		rc=0; \
		for f in $$files; do \
			if ! grep -q "$$f" '$(srcdir)/po/POTFILES.in'; then \
				echo "$$f missing from POTFILES.in"; \
				rc=1; \
			fi; \
		done; \
		exit $$rc

clean-local:
	test ! -f examples/Makefile || (cd examples && $(MAKE) $(AM_MAKEFLAGS) clean)

.PHONY: doc examples examples-install examples-clean
