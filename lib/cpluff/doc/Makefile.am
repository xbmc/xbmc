## Process this file with automake to produce Makefile.in.

# Copyright 2007 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

SUBDIRS = img

EXTRA_DIST = index.en.html copyright.en.html cpluff_style.css common_style.css

htmldocdir = $(docdir)/html
htmldoc_DATA = index.en.html copyright.en.html cpluff_style.css common_style.css

examplesdir = $(docdir)/examples

install-data-local: install-reference-doc install-examples-source

install-reference-doc:
	{ test ! -d '$(DESTDIR)$(htmldocdir)/reference' || \
		{ chmod -R u+w '$(DESTDIR)$(htmldocdir)/reference' && \
			rm -rf '$(DESTDIR)$(htmldocdir)/reference'; }; } && \
	$(MKDIR_P) '$(DESTDIR)$(htmldocdir)' && \
	if test -d '$(srcdir)/reference'; then \
		cp -r '$(srcdir)/reference' '$(DESTDIR)$(htmldocdir)'; \
	else \
		cd .. && \
		$(MAKE) $(AM_MAKEFLAGS) doc && \
		cp -r libcpluff/doc/reference '$(DESTDIR)$(htmldocdir)' && \
		cd doc; \
	fi && \
	find '$(DESTDIR)$(htmldocdir)/reference' -type f -exec chmod 644 '{}' \; && \
	cd '$(DESTDIR)$(htmldocdir)' && \
	rm -f index.html && \
	$(LN_S) index.en.html index.html

install-examples-source:
	{ test ! -d '$(DESTDIR)$(examplesdir)' || \
		{ chmod -R u+w '$(DESTDIR)$(examplesdir)' && \
			rm -rf '$(DESTDIR)$(examplesdir)'; }; } && \
	for d in \
		cpfile/plugins/cext \
		cpfile/plugins/core \
		cpfile/plugins/extension \
		cpfile/plugins/special; do \
		$(MKDIR_P) "$(DESTDIR)$(examplesdir)/$$d"; \
	done && \
	cd '$(srcdir)/..' && \
	find examples -type f \( -name 'autogen.sh' \
		-or -name 'configure.ac' \
		-or -name 'Makefile.am' \
		-or -name 'cpfile.in' \
		-or -name '*.nmake' \
		-or -name '*.bat' \
		-or -name '*.txt' \
		-or -name '*.xml' \
		-or -name '*.xsd' \
		-or -name '*.c' \
		-or -name '*.h' \) \
		-exec $(INSTALL_DATA) '{}' '$(DESTDIR)$(docdir)/{}' \; && \
	chmod 755 '$(DESTDIR)$(examplesdir)/autogen.sh'

uninstall-local:
	chmod -R u+w '$(DESTDIR)$(docdir)'
	rm -rf '$(DESTDIR)$(docdir)'

.PHONY: install-reference-doc install-examples-source
