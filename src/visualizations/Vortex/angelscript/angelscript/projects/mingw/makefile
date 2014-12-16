# Angelscript MingW makefile
# Type 'make' then 'make install' to complete the installation of the library

CXX = g++
CXXFLAGS = -ggdb
SRCDIR = ../../source
OBJDIR = obj
SRCNAMES = \
  as_arrayobject.cpp \
  as_atomic.cpp \
  as_builder.cpp  \
  as_bytecode.cpp \
  as_callfunc.cpp \
  as_callfunc_mips.cpp \
  as_callfunc_ppc_64.cpp \
  as_callfunc_ppc.cpp \
  as_callfunc_sh4.cpp \
  as_callfunc_x86.cpp \
  as_compiler.cpp \
  as_configgroup.cpp \
  as_context.cpp \
  as_datatype.cpp \
  as_generic.cpp \
  as_gc.cpp \
  as_globalproperty.cpp \
  as_memory.cpp \
  as_module.cpp \
  as_objecttype.cpp \
  as_outputbuffer.cpp \
  as_parser.cpp \
  as_restore.cpp \
  as_scriptcode.cpp \
  as_scriptengine.cpp \
  as_scriptfunction.cpp \
  as_scriptnode.cpp \
  as_scriptobject.cpp \
  as_string.cpp \
  as_string_util.cpp \
  as_thread.cpp \
  as_tokenizer.cpp \
  as_typeinfo.cpp \
  as_variablescope.cpp \

OBJ = $(addprefix $(OBJDIR)/, $(notdir $(SRCNAMES:.cpp=.o)))
BIN = ../../lib/libangelscript.a
OBJ_D = $(subst /,\,$(OBJ))
BIN_D = $(subst /,\,$(BIN))
DELETER = del /f
COPIER = copy /y
INCLUDEFILES_D = ..\..\include\angelscript.h
UNINSTALLFILES_D = $(MINGDIR)\lib\libangelscript.a $(MINGDIR)\include\angelscript.h

all: $(BIN)

$(BIN): $(OBJ)
	ar rcs $(BIN) $(OBJ)
	@echo -------------------------------------------------------------------
	@echo Done. Now type 'make install' to install the library on your MinGW.

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<


clean:
	$(DELETER) $(OBJ_D) $(BIN_D)

install: $(BIN)
	$(COPIER) $(BIN_D) $(MINGDIR)\lib
	$(COPIER) $(INCLUDEFILES_D) $(MINGDIR)\include
	@echo -------------------------------------------------------------------
	@echo Angelscript library installed. Enjoy!

uninstall:
	$(DELETER) $(UNINSTALLFILES_D)
	@echo -------------------------------------------------------------------
	@echo Angelscript library uninstalled.

.PHONY: all clean install uninstall
