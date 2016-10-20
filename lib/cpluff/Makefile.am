## Process this file with automake to produce Makefile.in.

# Copyright 2007 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = libcpluff loader console po test
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

distcheck-hook: distcheck-potfiles distcheck-examples

distcheck-potfiles:
	files="`cd '$(srcdir)' && find console libcpluff loader -type f -name '*.[ch]' -exec grep -q '_(' '{}' \; -print`"; \
		rc=0; \
		for f in $$files; do \
			if ! grep -q "$$f" '$(srcdir)/po/POTFILES.in'; then \
				echo "$$f missing from POTFILES.in"; \
				rc=1; \
			fi; \
		done; \
		exit $$rc

distcheck-examples:
	{ test ! -d _examples_test || { \
			chmod -R u+w _examples_test; \
			rm -rf _examples_test; } && \
		mkdir _examples_test && \
		absdistdir="`cd '$(distdir)' && pwd`" && \
		cd _examples_test && \
		instdir="`pwd`/_inst" && \
		"$$absdistdir"/configure --prefix="$$instdir" && \
		make install && \
		cd _inst/share/doc/cpluff/examples && \
		./autogen.sh && \
		CPPFLAGS="-I$$instdir/include" LDFLAGS="-L$$instdir/lib" CPLUFF_LOADER="$$instdir/bin/cpluff-loader" ./configure --prefix="$$instdir" && \
		make install && \
		test "`LC_ALL=C "$$instdir"/bin/cpfile . | tail -1`" = ".: directory" && \
		cd ../../../../../.. && \
		chmod -R u+w _examples_test && \
		rm -rf _examples_test; } || \
		{ echo "Could not compile, install or run example source code"; \
			exit 1; }

clean-local:
	test ! -f examples/Makefile || (cd examples && $(MAKE) $(AM_MAKEFLAGS) clean)

.PHONY: doc examples examples-install examples-clean
