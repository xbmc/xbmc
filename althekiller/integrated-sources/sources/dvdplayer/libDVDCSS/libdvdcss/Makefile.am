SUBDIRS = src test doc
DIST_SUBDIRS = $(SUBDIRS) debian

EXTRA_DIST = libdvdcss.spec bootstrap

AUTOMAKE_OPTIONS = foreign dist-bzip2 subdir-objects

doc-dummy:

doc: doc-dummy
	$(MAKE) -C doc doc

