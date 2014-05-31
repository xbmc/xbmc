# This file is sourced from xbmc/Makefile and tools/darwin/Support/makepythoninterface.command

FILEPATH := $(abspath $(dir $(MAKEFILE_LIST)))
VERSION.TXT := $(FILEPATH)/../version.txt

$(FILEPATH)/CompileInfo.cpp: $(VERSION.TXT) $(FILEPATH)/CompileInfo.cpp.in
	@MAJOR=$$(awk '/VERSION_MAJOR/ {print $$2}' $(VERSION.TXT)) ;\
	MINOR=$$(awk '/VERSION_MINOR/ {print $$2}' $(VERSION.TXT)) ;\
	TAG=$$(awk '/VERSION_TAG/ {print $$2}' $(VERSION.TXT)) ;\
	sed -e "s/\@APP_VERSION_MAJOR\@/$$MAJOR/" -e "s/\@APP_VERSION_MINOR\@/$$MINOR/" -e "s/\@APP_VERSION_TAG\@/$$TAG/" $@.in > $@
