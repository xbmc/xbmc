VPATH=../../docs
top_builddir=..
-include $(top_builddir)/Make.conf

DOXYFILE = ftgl_dox
DOCDIRS = html

all: doc

doc: doc-stamp
doc-stamp: $(DOXYFILE)
	sed 's,^INPUT[ 	].*,INPUT=../../src,' $^ | $(DOXYGEN) -
	touch $@

install-local: doc
	$(INSTALL) -d -m 0755 $(datadir)/doc/$(PACKAGE_NAME)/html
	$(INSTALL) -m 0644 html/* $(datadir)/doc/$(PACKAGE_NAME)/html/

distclean-local:
	$(RM) -r $(DOCDIRS)
	$(RM) doc-stamp

include $(top_builddir)/Make.rules

.PHONY: all doc
