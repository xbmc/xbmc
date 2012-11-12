TOPDIR ?= .
INTERFACES_DIR ?= xbmc/interfaces

JAVA ?= $(shell which java)
ifeq ($(JAVA),)
JAVA = java-not-found
endif

SWIG ?= $(shell which swig)
ifeq ($(SWIG),)
SWIG = swig-not-found
endif

DOXYGEN ?= $(shell which doxygen)
ifeq ($(DOXYGEN),)
DOXYGEN = doxygen-not-found
else
DOXY_XML_PATH=$(GENDIR)/doxygenxml
endif

GENDIR = $(INTERFACES_DIR)/python/generated
GROOVY_DIR = $(TOPDIR)/lib/groovy

GENERATED =  $(GENDIR)/AddonModuleXbmc.cpp
GENERATED += $(GENDIR)/AddonModuleXbmcgui.cpp
GENERATED += $(GENDIR)/AddonModuleXbmcplugin.cpp
GENERATED += $(GENDIR)/AddonModuleXbmcaddon.cpp
GENERATED += $(GENDIR)/AddonModuleXbmcvfs.cpp

GENERATE_DEPS += $(TOPDIR)/xbmc/interfaces/legacy/*.h

vpath %.i $(INTERFACES_DIR)/swig

$(GENDIR)/%.cpp: $(GENDIR)/%.xml $(JAVA) $(SWIG) $(DOXY_XML_PATH)
	$(JAVA) -cp "$(GROOVY_DIR)/groovy-all-1.8.4.jar:$(GROOVY_DIR)/commons-lang-2.6.jar:$(TOPDIR)/tools/codegenerator:$(INTERFACES_DIR)/python" \
          groovy.ui.GroovyMain $(TOPDIR)/tools/codegenerator/Generator.groovy $< $(INTERFACES_DIR)/python/PythonSwig.cpp.template $@ $(DOXY_XML_PATH)
	rm $<

$(GENDIR)/%.xml: %.i $(SWIG) $(JAVA) $(GENERATE_DEPS)
	mkdir -p $(GENDIR)
	$(SWIG) -w401 -c++ -o $@ -xml -I$(TOPDIR)/xbmc -xmllang python $<

codegenerated: $(DOXYGEN) $(SWIG) $(JAVA) $(GENERATED)

$(DOXY_XML_PATH): $(SWIG) $(JAVA)
	cd $(INTERFACES_DIR)/python; ($(DOXYGEN) Doxyfile > /dev/null) 2>&1 | grep -v " warning: "
	touch $@

$(DOXYGEN):
	@echo "Warning: No doxygen installed. The Api will not have any docstrings."
	mkdir -p $(GENDIR)/doxygenxml

$(JAVA):
	@echo Java not found, it will be used if found after configure.
	@echo This is not necessarily an error.
	@false

$(SWIG):
	@echo Swig not found, it will be used if found after configure.
	@echo This is not necessarily an error.
	@false

