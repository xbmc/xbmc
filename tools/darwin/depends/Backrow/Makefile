include ../Makefile.include

SOURCE=Backrow
ARCHIVE=$(SOURCE).tar.gz
# Specific download location
BASE_URL=http://xbmc-for-atv2.googlecode.com/svn/trunk/Backrow

all: $(SOURCE) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore

.installed:
	mkdir -p $(PREFIX)/include
	cp -rf $(SOURCE) $(PREFIX)/include/
	touch $@

clean:
distclean::
	rm -rf $(SOURCE) .installed
