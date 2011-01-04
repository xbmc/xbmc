commonSources:= as_callfunc_armasm.S as_arrayobject.cpp as_atomic.cpp as_builder.cpp as_bytecode.cpp as_callfunc.cpp as_callfunc_arm.cpp as_callfunc_mips.cpp as_callfunc_ppc.cpp as_callfunc_ppc_64.cpp as_callfunc_sh4.cpp as_callfunc_x86.cpp as_callfunc_x64_gcc.cpp as_compiler.cpp as_context.cpp as_configgroup.cpp as_datatype.cpp as_generic.cpp as_gc.cpp as_globalproperty.cpp as_memory.cpp as_module.cpp as_objecttype.cpp as_outputbuffer.cpp as_parser.cpp as_restore.cpp as_scriptcode.cpp as_scriptengine.cpp as_scriptfunction.cpp as_scriptnode.cpp as_scriptobject.cpp as_string.cpp as_string_util.cpp as_thread.cpp as_tokenizer.cpp as_typeinfo.cpp as_variablescope.cpp 
LOCAL_PATH:= $(call my-dir)/../../source
include $(CLEAR_VARS)
LOCAL_SRC_FILES:= $(commonSources)
LOCAL_MODULE:= libangelscript
include $(BUILD_STATIC_LIBRARY)

