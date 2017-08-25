# This file is sourced from xbmc/Makefile and tools/darwin/Support/makepythoninterface.command

FILEPATH := $(abspath $(dir $(MAKEFILE_LIST)))
VERSION.TXT := $(FILEPATH)/../version.txt
GITVERFILE := ../VERSION
GIT = $(notdir $(shell which git))

.PHONY: GitRevision $(FILEPATH)/.GitRevision
all: $(FILEPATH)/CompileInfo.cpp GitRevision
GitRevision: $(FILEPATH)/.GitRevision

$(FILEPATH)/.GitRevision:
	@if test -f $(GITVERFILE); then \
          GITREV=$$(cat $(GITVERFILE)) ;\
        elif test "$(GIT)" = "git" && test -d $(FILEPATH)/../.git ; then \
          if ! git diff-files --ignore-submodules --quiet -- || ! git diff-index --cached --ignore-submodules --quiet HEAD --; then \
            BUILD_DATE=$$(date -u "+%F"); \
            BUILD_SCMID=$$(git --no-pager log --abbrev=7 -n 1 --pretty=format:"%h-dirty"); \
            GITREV="$${BUILD_DATE}-$${BUILD_SCMID}" ;\
          else \
            BUILD_DATE=$$(git --no-pager log -n 1 --date=short --pretty=format:"%cd"); \
            BUILD_SCMID=$$(git --no-pager log --abbrev=7 -n 1 --pretty=format:"%h"); \
            GITREV="$${BUILD_DATE}-$${BUILD_SCMID}" ;\
          fi ;\
        else \
          GITREV="Unknown" ;\
        fi ;\
        [ -f $@ ] && OLDREV=$$(cat $@) ;\
        if test "$${OLDREV}" != "$${GITREV}"; then \
          echo $$GITREV > $@ ;\
        fi


$(FILEPATH)/CompileInfo.cpp: $(VERSION.TXT) $(FILEPATH)/CompileInfo.cpp.in $(FILEPATH)/.GitRevision
	@GITREV=$$(cat $(FILEPATH)/.GitRevision) ;\
	APP_NAME=$$(awk '/APP_NAME/ {print $$2}' $(VERSION.TXT)) ;\
	MAJOR=$$(awk '/VERSION_MAJOR/ {print $$2}' $(VERSION.TXT)) ;\
	MINOR=$$(awk '/VERSION_MINOR/ {print $$2}' $(VERSION.TXT)) ;\
	TAG=$$(awk '/VERSION_TAG/ {print $$2}' $(VERSION.TXT)) ;\
	APP_PACKAGE=$$(awk '/APP_PACKAGE/ {print $$2}' $(VERSION.TXT)) ;\
	sed -e "s/\@APP_NAME\@/$$APP_NAME/" -e "s/\@APP_VERSION_MAJOR\@/$$MAJOR/" -e "s/\@APP_VERSION_MINOR\@/$$MINOR/" -e "s/\@APP_VERSION_TAG\@/$$TAG/" -e "s/\@APP_SCMID\@/$$GITREV/" -e "s/\@APP_PACKAGE\@/$$APP_PACKAGE/" $@.in > $@

