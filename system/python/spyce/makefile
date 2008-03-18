PYTHON := $(shell which python)
SPYCE_VERSION = $(shell $(PYTHON) -c "import spyce; print spyce.__version__")
SPYCE_RELEASE = $(shell $(PYTHON) -c "import spyce; print spyce.__release__")

SRC := $(wildcard *.py) $(wildcard modules/*.py) $(wildcard tags/*.py)
OTHER := CHANGES LICENCE README THANKS spyceApache.conf spyce.conf.eg spyce.mime
DOC_SRC := $(wildcard docs/*.spy) docs/*.gif docs/examples/*.gif
DOC = $(wildcard docs/*.html) docs/*.gif
EXAMPLES := $(wildcard docs/examples/*.spy) $(wildcard docs/examples/*.spi) $(wildcard docs/examples/*.tmpl) $(wildcard docs/examples/*.py) $(wildcard docs/examples/*.gif)

SFUSER := batripler
SFCVSanon := :pserver:anonymous@cvs.spyce.sourceforge.net:/cvsroot/spyce
SFCVSdev := :ext:$(SFUSER)@cvs.sourceforge.net:/cvsroot/spyce
SFCVS := $(SFCVSdev)

ssh := ssh -1 -x
scp := scp -o Protocol=1

COMPILED = $(SRC:.py=.pyc)

# install paths
DESTDIR := /
INSTALL_ROOT := $(DESTDIR)
INSTALL_CODE := $(INSTALL_ROOT)/usr/share

RPMROOT := /home/barr/misc/rpm

.PHONY: all compile clean remake tgz rpm www

all: compile docs

# make this a dependency every time you use $PYTHON or $SPYCE_VERSION
versionchk: $(PYTHON)
	$(PYTHON) verchk.py
	touch versionchk

clean: versionchk
	-rm versionchk
	-rm -f spyce.spec spyce.nsi
	-rm -f *.pyc
	-rm -f modules/*.pyc
	-rm -f tags/*.pyc
	-rm -f spyce-$(SPYCE_VERSION)-$(SPYCE_RELEASE).tgz
	-rm -f spyce-$(SPYCE_VERSION)-$(SPYCE_RELEASE).noarch.rpm
	-rm -f $(RPMROOT)/SOURCES/spyce-$(SPYCE_VERSION)-$(SPYCE_RELEASE).tgz
	-rm -f $(RPMROOT)/SRPMS/spyce-$(SPYCE_VERSION)-$(SPYCE_RELEASE).src.rpm
	-rm -f $(RPMROOT)/RPMS/noarch/spyce-$(SPYCE_VERSION)-$(SPYCE_RELEASE).noarch.rpm
	-rm -rf $(RPMROOT)/BUILD/spyce-$(SPYCE_VERSION)
	-$(MAKE) -C docs clean

perm:
	chmod a+r -R .
	chmod a+x . `find . -type d`
	chmod a+x run_*.py verchk.py

remake: clean all

compile: $(COMPILED)

# make documentation
docs: compile
	@$(MAKE) -C docs

# make source tarball
tgz: versionchk
	@echo "Making clean source tarball: spyce-$(SPYCE_VERSION)-$(SPYCE_RELEASE).tgz"
	-rm -rf spyce-$(SPYCE_VERSION)
	cvs -d $(SFCVS) export -d spyce-$(SPYCE_VERSION) -D now spyce
	chmod a+r -R spyce-$(SPYCE_VERSION)
	chmod a+x `find spyce-$(SPYCE_VERSION) -type d`
	chmod a+x spyce-$(SPYCE_VERSION)/run_*.py spyce-$(SPYCE_VERSION)/verchk.py
	# process .nsi.in so that we can just use .tgz to make .exe installer
	cd spyce-$(SPYCE_VERSION); make spyce.nsi; rm -f *.pyc *.pyo; cd ..
	tar --totals -czf spyce-$(SPYCE_VERSION)-$(SPYCE_RELEASE).tgz spyce-$(SPYCE_VERSION)
	rm -rf spyce-$(SPYCE_VERSION)

# make rpm
rpm: versionchk tgz spyce.spec
	cp spyce-$(SPYCE_VERSION)-$(SPYCE_RELEASE).tgz $(RPMROOT)/SOURCES
	rpmbuild -ba spyce.spec
	cp $(RPMROOT)/RPMS/noarch/spyce-$(SPYCE_VERSION)-$(SPYCE_RELEASE).noarch.rpm .

# install Spyce (used for manual install and by rpm scripts)
install: compile docs
	@echo "Installing Spyce python module in: $(INSTALL_CODE)/spyce"
	@for f in `find . -type d`; do \
		mkdir -p $(INSTALL_CODE)/spyce/$$f; \
		chmod a+rx $(INSTALL_CODE)/spyce/$$f; \
	done
	@for f in $(SRC) $(COMPILED) $(OTHER) $(DOC) $(EXAMPLES); do \
		install -m 644 $$f $(INSTALL_CODE)/spyce/$$f; \
	done
	@chmod a+rx $(INSTALL_CODE)/spyce/run_*.py $(INSTALL_CODE)/spyce/verchk.py

# uninstall Spyce (used for manual install and by rpm scripts)
uninstall:
	@echo "Removing main Spyce directory: $(INSTALL_CODE)/spyce"
	@-rm -rf $(INSTALL_CODE)/spyce

# generate python compiled (bytecode) files
$(COMPILED): $(SRC)
	$(PYTHON) -c "exec '''import py_compile,string,os\nfor i in string.split('$+', ' '):\n  print 'Compiling: '+i; py_compile.compile(i)\n'''"

%.pyc: %.py versionchk
	$(PYTHON) -c "import py_compile; py_compile.compile('$<')"

spyce.spec: spyce.spec.in spyce.py
	cat spyce.spec.in | sed "s/__VERSION__/${SPYCE_VERSION}/" | sed "s/__RELEASE__/${SPYCE_RELEASE}/" > spyce.spec

spyce.nsi: spyce.nsi.in spyce.py
	cat spyce.nsi.in | sed "s/__VERSION__/${SPYCE_VERSION}/" | sed "s/__RELEASE__/${SPYCE_RELEASE}/" > spyce.nsi


# update sourceforge
sf: clean tgz compile
	-rm -r www
	mkdir www; mkdir www/htdocs; mkdir www/htdocs/examples; mkdir www/contrib; mkdir www/cgi-bin; mkdir www/cgi-bin/eg
	# docs
	@$(MAKE) -C docs sf
	cp docs/*.html docs/*.gif docs/*.ico www/htdocs
	# examples
	cp docs/examples/*.spy docs/examples/*.spi docs/examples/*.tmpl www/htdocs/examples
	# contribs
	python run_spyceCmd.py -O contrib/*.spy
	cp -r -L contrib/* www/contrib
	pushd www/htdocs; ln -s ../contrib; popd
	# exec
	cp spyce-$(SPYCE_VERSION)-$(SPYCE_RELEASE).tgz www
	cd www; tar -xzf spyce-$(SPYCE_VERSION)-$(SPYCE_RELEASE).tgz; mv spyce-$(SPYCE_VERSION) spyce; rm spyce-$(SPYCE_VERSION)-$(SPYCE_RELEASE).tgz; cd spyce; make compile; cd ../..
	# cgi
	cp docs/examples/*.spy docs/examples/*.spi docs/examples/*.py docs/examples/*.tmpl docs/examples/*.gif www/cgi-bin/eg
	cd www; for f in cgi-bin/eg/*.spy; do spyce/misc/addfirstline.sh $$f '#!/usr/bin/python ../../spyce/run_spyceCGI.py'; done; cd ..
	# package it all up
	tar -czvf www.tgz www/
	-rm -r www
	# send it over
	$(scp) www.tgz $(SFUSER)@shell.sourceforge.net:.
	-rm www.tgz
	make clean
	# unpackage it there
	$(ssh) $(SFUSER)@shell.sourceforge.net 'tar -xzf www.tgz; cd www; chmod -R a+r .; chmod a+x `find . -type d`; chmod a+x cgi-bin/eg/*.spy; cd spyce; make perm; cd ../..; rm www.tgz'
	# out with the old and in with the new
	$(ssh) $(SFUSER)@shell.sourceforge.net 'rm -rf spyce-www/*; mv www/* spyce-www; rmdir www'

# upload files to sourceforge file release system
upload: versionchk rpm
	ncftpput upload.sourceforge.net incoming spyce-$(SPYCE_VERSION)-$(SPYCE_RELEASE).tgz spyce-$(SPYCE_VERSION)-$(SPYCE_RELEASE).noarch.rpm
	make clean

