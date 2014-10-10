TEMPLATE = lib

DEPENDPATH += ../../source ../../include
INCLUDEPATH += ../../include

CONFIG -= debug debug_and_release release app_bundle qt dll

CONFIG += staticlib release

DEFINES += _CRT_SECURE_NO_WARNINGS

DESTDIR = ../../lib

win32: LIBS += -lwinmm

HEADERS += ../../include/angelscript.h \
           ../../source/as_array.h \
           ../../source/as_arrayobject.h \
           ../../source/as_builder.h \
           ../../source/as_bytecode.h \
           ../../source/as_bytecodedef.h \
           ../../source/as_callfunc.h \
           ../../source/as_compiler.h \
           ../../source/as_config.h \
           ../../source/as_configgroup.h \
           ../../source/as_context.h \
           ../../source/as_datatype.h \
           ../../source/as_debug.h \
           ../../source/as_generic.h \
           ../../source/as_map.h \
           ../../source/as_memory.h \
           ../../source/as_module.h \
           ../../source/as_objecttype.h \
           ../../source/as_outputbuffer.h \
           ../../source/as_parser.h \
           ../../source/as_property.h \
           ../../source/as_restore.h \
           ../../source/as_scriptcode.h \
           ../../source/as_scriptengine.h \
           ../../source/as_scriptfunction.h \
           ../../source/as_scriptnode.h \
           ../../source/as_scriptobject.h \
           ../../source/as_string.h \
           ../../source/as_string_util.h \
           ../../source/as_texts.h \
           ../../source/as_thread.h \
           ../../source/as_tokendef.h \
           ../../source/as_tokenizer.h \
           ../../source/as_typeinfo.h \
           ../../source/as_variablescope.h

SOURCES += ../../source/as_arrayobject.cpp \
           ../../source/as_atomic.cpp \
           ../../source/as_builder.cpp \
           ../../source/as_bytecode.cpp \
           ../../source/as_callfunc.cpp \
           ../../source/as_callfunc_mips.cpp \
           ../../source/as_callfunc_sh4.cpp \
           ../../source/as_callfunc_x86.cpp \
           ../../source/as_compiler.cpp \
           ../../source/as_configgroup.cpp \
           ../../source/as_context.cpp \
           ../../source/as_datatype.cpp \
           ../../source/as_gc.cpp \
           ../../source/as_generic.cpp \
           ../../source/as_globalproperty.cpp \
           ../../source/as_memory.cpp \
           ../../source/as_module.cpp \
           ../../source/as_objecttype.cpp \
           ../../source/as_outputbuffer.cpp \
           ../../source/as_parser.cpp \
           ../../source/as_restore.cpp \
           ../../source/as_scriptcode.cpp \
           ../../source/as_scriptengine.cpp \
           ../../source/as_scriptfunction.cpp \
           ../../source/as_scriptnode.cpp \
           ../../source/as_scriptobject.cpp \
           ../../source/as_string.cpp \
           ../../source/as_string_util.cpp \
           ../../source/as_thread.cpp \
           ../../source/as_tokenizer.cpp \
           ../../source/as_typeinfo.cpp \
           ../../source/as_variablescope.cpp

HEADERS += ../../../add_on/scriptany/scriptany.h \
           ../../../add_on/scriptdictionary/scriptdictionary.h \
           ../../../add_on/scriptmath/scriptmath.h \
           ../../../add_on/scriptmath3d/scriptmath3d.h \
           ../../../add_on/scriptstring/scriptstring.h \
           ../../../add_on/scriptbuilder/scriptbuilder.h

SOURCES += ../../../add_on/scriptany/scriptany.cpp \
           ../../../add_on/scriptdictionary/scriptdictionary.cpp \
           ../../../add_on/scriptmath/scriptmath.cpp \
           ../../../add_on/scriptmath3d/scriptmath3d.cpp \
           ../../../add_on/scriptstring/scriptstring.cpp \
           ../../../add_on/scriptstring/scriptstring_utils.cpp \
           ../../../add_on/scriptbuilder/scriptbuilder.cpp

OBJECTS_DIR = tmp
MOC_DIR = tmp
UI_DIR = tmp
RCC_DIR = tmp

# QMAKE_CXXFLAGS_RELEASE += /MP

