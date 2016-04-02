BOOTSTRAP_SUBDIRS += configure.ac
BOOTSTRAP_SUBDIRS += lib/cpluff/configure.ac
BOOTSTRAP_SUBDIRS += lib/gtest/configure.ac

BOOTSTRAP_TARGETS=$(basename $(BOOTSTRAP_SUBDIRS))
all: $(BOOTSTRAP_TARGETS)

%: %.ac
	autoreconf -vif $(@D)
	-@rm -rf $(@D)/autom4te.cache

%: %.in
	autoreconf -vif $(@D)
	-@rm -rf $(@D)/autom4te.cache

configure: configure.ac
	autoreconf -vif $(@D)
	-@rm -rf $(@D)/autom4te.cache
	@test -n "$$BOOTSTRAP_STANDALONE" || ( echo "Configuration is stale. You should almost certainly reconfigure" && false )

