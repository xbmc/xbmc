include ../../Makefile.include
DEPS= ../../Makefile.include

SOURCE=Backrow
ARCHIVE=$(SOURCE).tar.gz
# Specific download location
BASE_URL=http://xbmc-for-atv2.googlecode.com/svn/trunk/Backrow

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):                                                                                                                                                                                 
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM)/$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	mkdir -p $(PLATFORM)/$(SOURCE)
	cd $(PLATFORM)/$(SOURCE);$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

.installed-$(PLATFORM): $(PLATFORM)/$(SOURCE)
	mkdir -p $(PREFIX)/include
	cp -pR $(PLATFORM)/$(SOURCE) $(PREFIX)/include/
	touch $@

clean:
distclean::
	rm -rf $(SOURCE) .installed-$(PLATFORM)
