# Test Framework MingW makefile

CXX = g++
CXXFLAGS = -ggdb -I../../../../angelscript/include
SRCDIR = ../../source
OBJDIR = obj

SRCNAMES = \
  bstr.cpp \
  main.cpp \
  test_2func.cpp \
  test_any.cpp \
  test_argref.cpp \
  test_array.cpp \
  test_arrayhandle.cpp \
  test_arrayintf.cpp \
  test_arrayobject.cpp \
  test_autohandle.cpp \
  test_circularimport.cpp \
  test_config.cpp \
  test_configaccess.cpp \
  test_condition.cpp \
  test_constobject.cpp \
  test_constproperty.cpp \
  test_constructor.cpp \
  test_constructor2.cpp \
  test_conversion.cpp \
  test_custommem.cpp \
  test_debug.cpp \
  test_dict.cpp \
  test_discard.cpp \
  test_dynamicconfig.cpp \
  test_exception.cpp \
  test_exceptionmemory.cpp \
  test_float.cpp \
  test_funcoverload.cpp \
  test_generic.cpp \
  test_import.cpp \
  test_import2.cpp \
  test_multiassign.cpp \
  test_nested.cpp \
  test_nevervisited.cpp \
  test_notinitialized.cpp \
  test_object.cpp \
  test_object2.cpp \
  test_object3.cpp \
  test_objhandle.cpp \
  test_objhandle2.cpp \
  test_objzerosize.cpp \
  test_optimize.cpp \
  test_pointer.cpp \
  test_refargument.cpp \
  test_return_with_cdecl_objfirst.cpp \
  test_saveload.cpp \
  test_scriptstring.cpp \
  test_scriptstruct.cpp \
  test_suspend.cpp \
  test_stack2.cpp \
  test_stdvector.cpp \
  test_stream.cpp \
  test_structintf.cpp \
  test_vector3.cpp \
  test_vector3_2.cpp \
  test2modules.cpp \
  testbstr.cpp \
  testbstr2.cpp  \
  testcdecl_class.cpp \
  testcdecl_class_a.cpp \
  testcdecl_class_c.cpp \
  testcdecl_class_d.cpp \
  testcreateengine.cpp \
  testenumglobvar.cpp \
  testexecute.cpp \
  testexecute1arg.cpp \
  testexecute2args.cpp \
  testexecute32args.cpp \
  testexecute32mixedargs.cpp \
  testexecute4args.cpp \
  testexecute4argsf.cpp \
  testexecutemixedargs.cpp \
  testexecutescript.cpp \
  testexecutestring.cpp \
  testexecutethis32mixedargs.cpp \
  testglobalvar.cpp \
  testint64.cpp \
  testlongtoken.cpp \
  testmoduleref.cpp \
  testmultipleinheritance.cpp \
  testnegateoperator.cpp \
  testnotcomplexstdcall.cpp \
  testnotcomplexthiscall.cpp \
  testoutput.cpp \
  testreturn.cpp \
  testreturnd.cpp \
  testreturnf.cpp \
  teststdcall4args.cpp \
  teststdstring.cpp \
  teststack.cpp \
  testswitch.cpp \
  testtempvar.cpp \
  testvirtualinheritance.cpp \
  testvirtualmethod.cpp \
  utils.cpp \
  stdstring.cpp \
 

OBJ = $(addprefix $(OBJDIR)/, $(notdir $(SRCNAMES:.cpp=.o))) obj/scriptstring.o
BIN = ../../bin/mingw.exe
OBJ_D = $(subst /,\,$(OBJ))
BIN_D = $(subst /,\,$(BIN))
DELETER = del /f

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) -o $(BIN) $(OBJ) -langelscript -L ../../../../angelscript/lib
	@echo -------------------------------------------------------------------
	@echo Done. 

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/scriptstring.o: ../../../../add_on/scriptstring/scriptstring.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<
	
clean:
	$(DELETER) $(OBJ_D) $(BIN_D)

.PHONY: all clean