
OSTYPE=$(shell uname)
VERSION=2.5.6
BASE_URL=http://pysqlite.googlecode.com/files
SOURCE=pysqlite-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

RETRIEVE_TOOL=curl
RETRIEVE_TOOL_FLAGS=-Ls --output $(ARCHIVE)
ARCHIVE_TOOL=tar
ARCHIVE_TOOL_FLAGS=xf

PYTHONDIR=$(abs_top_srcdir)/lib/python
ifeq ($(OSTYPE),Darwin)
PYTHON=$(PYTHONDIR)/python.exe
else
PYTHON=$(PYTHONDIR)/python
endif

DESTDIR=$(abs_top_srcdir)/addons/script.module.pysqlite/lib/

CLEAN_FILES=$(ARCHIVE) $(SOURCE) $(DESTDIR)

all: $(DESTDIR)

$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(ARCHIVE)
	-rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(ARCHIVE)

$(DESTDIR): $(SOURCE) $(PYTHON)
	-rm -rf $(DESTDIR)
	pushd $(SOURCE) && LD_LIBRARY_PATH=$(PYTHONDIR) $(PYTHON) setup.py build --build-lib $(DESTDIR); popd

$(PYTHON):
	$(MAKE) -C $(abs_top_srcdir) libpython

include ../../../Makefile.include

