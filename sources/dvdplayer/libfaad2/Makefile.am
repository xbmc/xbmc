SUBDIRS = libfaad common frontend plugins

EXTRA_DIST = faad2.spec

rpm:	Makefile
	make dist
	$(RPMBUILD) -ta $(PACKAGE)-$(VERSION).tar.gz
	rm $(PACKAGE)-$(VERSION).tar.gz
