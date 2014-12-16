/*
   AngelCode Scripting Library
   Copyright (c) 2003-2009 Andreas Jonsson

   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any
   purpose, including commercial applications, and to alter it and
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
      distribution.

   The original version of this library can be located at:
   http://www.angelcode.com/angelscript/

   Andreas Jonsson
   andreas@angelcode.com
*/


//
// angelscript_c.h
//
// The script engine interface for the C language.
//
// The idea is that the library should be compiled with a C++ compiler with the AS_C_INTERFACE 
// preprocessor word defined. The C application will then be able to link with the library and
// use this header file to interact with it.
//
// Note: This header file is not yet complete. I'd appreciate any help with completing it.
//

#ifndef ANGELSCRIPT_C_H
#define ANGELSCRIPT_C_H

#define ANGELSCRIPT_VERSION        21800
#define ANGELSCRIPT_VERSION_STRING "2.18.0"

#ifdef AS_USE_NAMESPACE
 #define BEGIN_AS_NAMESPACE namespace AngelScript {
 #define END_AS_NAMESPACE }
#else
 #define BEGIN_AS_NAMESPACE
 #define END_AS_NAMESPACE
#endif

BEGIN_AS_NAMESPACE

typedef enum { asTRUE = 1, asFALSE = 0 } asBOOL;
typedef unsigned char  asBYTE;
typedef unsigned short asWORD;
typedef unsigned int   asUINT;
typedef size_t         asPWORD;
#ifdef __LP64__
    typedef unsigned int  asDWORD;
    typedef unsigned long asQWORD;
    typedef long asINT64;
#else
    typedef unsigned long asDWORD;
  #if defined(__GNUC__) || defined(__MWERKS__)
    typedef unsigned long long asQWORD;
    typedef long long asINT64;
  #else
    typedef unsigned __int64 asQWORD;
    typedef __int64 asINT64;
  #endif
#endif

// Enumerations and constants

// Engine properties
typedef enum 
{
	asEP_ALLOW_UNSAFE_REFERENCES      = 1,
	asEP_OPTIMIZE_BYTECODE            = 2,
	asEP_COPY_SCRIPT_SECTIONS         = 3,
	asEP_MAX_STACK_SIZE               = 4,
	asEP_USE_CHARACTER_LITERALS       = 5,
	asEP_ALLOW_MULTILINE_STRINGS      = 6,
	asEP_ALLOW_IMPLICIT_HANDLE_TYPES  = 7,
	asEP_BUILD_WITHOUT_LINE_CUES      = 8,
	asEP_INIT_GLOBAL_VARS_AFTER_BUILD = 9,
	asEP_REQUIRE_ENUM_SCOPE           = 10,
	asEP_SCRIPT_SCANNER               = 11,
	asEP_INCLUDE_JIT_INSTRUCTIONS     = 12,
	asEP_STRING_ENCODING              = 13
} asEEngineProp;

// Calling conventions
typedef enum 
{
	asCALL_CDECL            = 0,
	asCALL_STDCALL          = 1,
	asCALL_THISCALL         = 2,
	asCALL_CDECL_OBJLAST    = 3,
	asCALL_CDECL_OBJFIRST   = 4,
	asCALL_GENERIC          = 5
} asECallConvTypes;

// Object type flags
typedef enum 
{
	asOBJ_REF                   = 0x01,
	asOBJ_VALUE                 = 0x02,
	asOBJ_GC                    = 0x04,
	asOBJ_POD                   = 0x08,
	asOBJ_NOHANDLE              = 0x10,
	asOBJ_SCOPED                = 0x20,
	asOBJ_APP_CLASS             = 0x100,
	asOBJ_APP_CLASS_CONSTRUCTOR = 0x200,
	asOBJ_APP_CLASS_DESTRUCTOR  = 0x400,
	asOBJ_APP_CLASS_ASSIGNMENT  = 0x800,
	asOBJ_APP_CLASS_C           = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_CONSTRUCTOR),
	asOBJ_APP_CLASS_CD          = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_CONSTRUCTOR + asOBJ_APP_CLASS_DESTRUCTOR),
	asOBJ_APP_CLASS_CA          = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_CONSTRUCTOR + asOBJ_APP_CLASS_ASSIGNMENT),
	asOBJ_APP_CLASS_CDA         = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_CONSTRUCTOR + asOBJ_APP_CLASS_DESTRUCTOR + asOBJ_APP_CLASS_ASSIGNMENT),
	asOBJ_APP_CLASS_D           = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_DESTRUCTOR),
	asOBJ_APP_CLASS_A           = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_ASSIGNMENT),
	asOBJ_APP_CLASS_DA          = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_DESTRUCTOR + asOBJ_APP_CLASS_ASSIGNMENT),
	asOBJ_APP_PRIMITIVE         = 0x1000,
	asOBJ_APP_FLOAT             = 0x2000,
	asOBJ_MASK_VALID_FLAGS      = 0x3F3F,
	asOBJ_SCRIPT_OBJECT         = 0x10000
} asEObjTypeFlags;

// Behaviours
typedef enum 
{
	// Value object memory management
	asBEHAVE_CONSTRUCT,
	asBEHAVE_DESTRUCT,

	// Reference object memory management
	asBEHAVE_FACTORY,
	asBEHAVE_ADDREF,
	asBEHAVE_RELEASE,

	// Object operators
	asBEHAVE_VALUE_CAST,
	asBEHAVE_IMPLICIT_VALUE_CAST,
	asBEHAVE_REF_CAST,
	asBEHAVE_IMPLICIT_REF_CAST,
	asBEHAVE_INDEX,
	asBEHAVE_TEMPLATE_CALLBACK,

	// Garbage collection behaviours
	asBEHAVE_FIRST_GC,
	 asBEHAVE_GETREFCOUNT = asBEHAVE_FIRST_GC,
	 asBEHAVE_SETGCFLAG,
	 asBEHAVE_GETGCFLAG,
	 asBEHAVE_ENUMREFS,
	 asBEHAVE_RELEASEREFS,
	asBEHAVE_LAST_GC = asBEHAVE_RELEASEREFS
} asEBehaviours;

// Return codes
typedef enum 
{
	asSUCCESS                              =  0,
	asERROR                                = -1,
	asCONTEXT_ACTIVE                       = -2,
	asCONTEXT_NOT_FINISHED                 = -3,
	asCONTEXT_NOT_PREPARED                 = -4,
	asINVALID_ARG                          = -5,
	asNO_FUNCTION                          = -6,
	asNOT_SUPPORTED                        = -7,
	asINVALID_NAME                         = -8,
	asNAME_TAKEN                           = -9,
	asINVALID_DECLARATION                  = -10,
	asINVALID_OBJECT                       = -11,
	asINVALID_TYPE                         = -12,
	asALREADY_REGISTERED                   = -13,
	asMULTIPLE_FUNCTIONS                   = -14,
	asNO_MODULE                            = -15,
	asNO_GLOBAL_VAR                        = -16,
	asINVALID_CONFIGURATION                = -17,
	asINVALID_INTERFACE                    = -18,
	asCANT_BIND_ALL_FUNCTIONS              = -19,
	asLOWER_ARRAY_DIMENSION_NOT_REGISTERED = -20,
	asWRONG_CONFIG_GROUP                   = -21,
	asCONFIG_GROUP_IS_IN_USE               = -22,
	asILLEGAL_BEHAVIOUR_FOR_TYPE           = -23,
	asWRONG_CALLING_CONV                   = -24,
	asMODULE_IS_IN_USE                     = -25,
	asBUILD_IN_PROGRESS                    = -26,
	asINIT_GLOBAL_VARS_FAILED              = -27
} asERetCodes;

// Context states
typedef enum 
{
    asEXECUTION_FINISHED      = 0,
    asEXECUTION_SUSPENDED     = 1,
    asEXECUTION_ABORTED       = 2,
    asEXECUTION_EXCEPTION     = 3,
    asEXECUTION_PREPARED      = 4,
    asEXECUTION_UNINITIALIZED = 5,
    asEXECUTION_ACTIVE        = 6,
    asEXECUTION_ERROR         = 7
} asEContextState;

// ExecuteString flags
typedef enum 
{
	asEXECSTRING_ONLY_PREPARE	= 1,
	asEXECSTRING_USE_MY_CONTEXT = 2
} asEExecStrFlags;

// Message types
typedef enum 
{
    asMSGTYPE_ERROR       = 0,
    asMSGTYPE_WARNING     = 1,
    asMSGTYPE_INFORMATION = 2
} asEMsgType;

// Garbage collector flags
typedef enum
{
	asGC_FULL_CYCLE      = 1,
	asGC_ONE_STEP        = 2,
	asGC_DESTROY_GARBAGE = 4,
	asGC_DETECT_GARBAGE  = 8
} asEGCFlags;

// Prepare flags
const int asPREPARE_PREVIOUS = -1;

// Config groups
const char * const asALL_MODULES = (const char * const)-1;

// Type id flags
typedef enum 
{
	asTYPEID_VOID           = 0,
	asTYPEID_BOOL           = 1,
	asTYPEID_INT8           = 2,
	asTYPEID_INT16          = 3,
	asTYPEID_INT32          = 4,
	asTYPEID_INT64          = 5,
	asTYPEID_UINT8          = 6,
	asTYPEID_UINT16         = 7,
	asTYPEID_UINT32         = 8,
	asTYPEID_UINT64         = 9,
	asTYPEID_FLOAT          = 10,
	asTYPEID_DOUBLE         = 11,
	asTYPEID_OBJHANDLE      = 0x40000000,
	asTYPEID_HANDLETOCONST  = 0x20000000,
	asTYPEID_MASK_OBJECT    = 0x1C000000,
	asTYPEID_APPOBJECT      = 0x04000000,
	asTYPEID_SCRIPTOBJECT   = 0x0C000000,
	asTYPEID_SCRIPTARRAY    = 0x10000000,
	asTYPEID_MASK_SEQNBR    = 0x03FFFFFF
} asETypeIdFlags;

// GetModule flags
typedef enum
{
	asGM_ONLY_IF_EXISTS       = 0,
	asGM_CREATE_IF_NOT_EXISTS = 1,
	asGM_ALWAYS_CREATE        = 2
} asEGMFlags;

typedef struct 
{
	const char *section;
	int         row;
	int         col;
	asEMsgType  type;
	const char *message;
} asSMessageInfo;

typedef struct asIScriptEngine asIScriptEngine;
typedef struct asIScriptModule asIScriptModule;
typedef struct asIScriptContext asIScriptContext;
typedef struct asIScriptGeneric asIScriptGeneric;
typedef struct asIScriptObject asIScriptObject;
typedef struct asIScriptArray asIScriptArray;
typedef struct asIObjectType asIObjectType;
typedef struct asIScriptFunction asIScriptFunction;
typedef struct asIBinaryStream asIBinaryStream;

typedef void (*asBINARYREADFUNC_t)(void *ptr, asUINT size, void *param);
typedef void (*asBINARYWRITEFUNC_t)(const void *ptr, asUINT size, void *param);
typedef void *(*asALLOCFUNC_t)(size_t);
typedef void (*asFREEFUNC_t)(void *);
typedef void (*asFUNCTION_t)();

// API functions

// ANGELSCRIPT_EXPORT is defined when compiling the dll or lib
// ANGELSCRIPT_DLL_LIBRARY_IMPORT is defined when dynamically linking to the
// dll through the link lib automatically generated by MSVC++
// ANGELSCRIPT_DLL_MANUAL_IMPORT is defined when manually loading the dll
// Don't define anything when linking statically to the lib

#ifdef WIN32
  #ifdef ANGELSCRIPT_EXPORT
    #define AS_API __declspec(dllexport)
  #elif defined ANGELSCRIPT_DLL_LIBRARY_IMPORT
    #define AS_API __declspec(dllimport)
  #else // statically linked library
    #define AS_API
  #endif
#else
  #define AS_API
#endif

#ifndef ANGELSCRIPT_DLL_MANUAL_IMPORT
#ifdef __cplusplus
extern "C"
{
#endif
	// Engine
	AS_API asIScriptEngine * asCreateScriptEngine(asDWORD version);
	AS_API const char * asGetLibraryVersion();
	AS_API const char * asGetLibraryOptions();

	// Context
	AS_API asIScriptContext * asGetActiveContext();

	// Thread support
	AS_API int asThreadCleanup();

	// Memory management
	AS_API int asSetGlobalMemoryFunctions(asALLOCFUNC_t allocFunc, asFREEFUNC_t freeFunc);
	AS_API int asResetGlobalMemoryFunctions();

	AS_API int                asEngine_AddRef(asIScriptEngine *e);
	AS_API int                asEngine_Release(asIScriptEngine *e);
	AS_API int                asEngine_SetEngineProperty(asIScriptEngine *e, asEEngineProp property, asPWORD value);
	AS_API asPWORD            asEngine_GetEngineProperty(asIScriptEngine *e, asEEngineProp property);
	AS_API int                asEngine_SetMessageCallback(asIScriptEngine *e, asFUNCTION_t callback, void *obj, asDWORD callConv);
	AS_API int                asEngine_ClearMessageCallback(asIScriptEngine *e);
	AS_API int                asEngine_WriteMessage(asIScriptEngine *e, const char *section, int row, int col, asEMsgType type, const char *message);
	AS_API int                asEngine_RegisterObjectType(asIScriptEngine *e, const char *name, int byteSize, asDWORD flags);
	AS_API int                asEngine_RegisterObjectProperty(asIScriptEngine *e, const char *obj, const char *declaration, int byteOffset);
	AS_API int                asEngine_RegisterObjectMethod(asIScriptEngine *e, const char *obj, const char *declaration, asFUNCTION_t funcPointer, asDWORD callConv);
	AS_API int                asEngine_RegisterObjectBehaviour(asIScriptEngine *e, const char *datatype, asEBehaviours behaviour, const char *declaration, asFUNCTION_t funcPointer, asDWORD callConv);
	AS_API int                asEngine_RegisterGlobalProperty(asIScriptEngine *e, const char *declaration, void *pointer);
	AS_API int                asEngine_GetGlobalPropertyCount(asIScriptEngine *e);
	AS_API int                asEngine_GetGlobalPropertyByIndex(asIScriptEngine *e, asUINT index, const char **name, int *typeId, asBOOL *isConst, const char **configGroup, void **pointer);
	AS_API int                asEngine_RegisterGlobalFunction(asIScriptEngine *e, const char *declaration, asFUNCTION_t funcPointer, asDWORD callConv);
	AS_API int                asEngine_GetGlobalFunctionCount(asIScriptEngine *e);
	AS_API int                asEngine_GetGlobalFunctionIdByIndex(asIScriptEngine *e, asUINT index);
	AS_API int                asEngine_RegisterInterface(asIScriptEngine *e, const char *name);
	AS_API int                asEngine_RegisterInterfaceMethod(asIScriptEngine *e, const char *intf, const char *declaration);
	AS_API int                asEngine_RegisterStringFactory(asIScriptEngine *e, const char *datatype, asFUNCTION_t factoryFunc, asDWORD callConv);
	AS_API int                asEngine_GetStringFactoryReturnTypeId(asIScriptEngine *e);
	AS_API int                asEngine_RegisterEnum(asIScriptEngine *e, const char *type);
	AS_API int                asEngine_RegisterEnumValue(asIScriptEngine *e, const char *type, const char *name, int value);
	AS_API int                asEngine_GetEnumCount(asIScriptEngine *e);
	AS_API const char *       asEngine_GetEnumByIndex(asIScriptEngine *e, asUINT index, int *enumTypeId, const char **configGroup);
	AS_API int                asEngine_GetEnumValueCount(asIScriptEngine *e, int enumTypeId);
	AS_API const char *       asEngine_GetEnumValueByIndex(asIScriptEngine *e, int enumTypeId, asUINT index, int *outValue);
	AS_API int                asEngine_RegisterTypedef(asIScriptEngine *e, const char *type, const char *decl);
	AS_API int                asEngine_GetTypedefCount(asIScriptEngine *e);
	AS_API const char *       asEngine_GetTypedefByIndex(asIScriptEngine *e, asUINT index, int *typeId, const char **configGroup);
	AS_API int                asEngine_BeginConfigGroup(asIScriptEngine *e, const char *groupName);
	AS_API int                asEngine_EndConfigGroup(asIScriptEngine *e);
	AS_API int                asEngine_RemoveConfigGroup(asIScriptEngine *e, const char *groupName);
	AS_API int                asEngine_SetConfigGroupModuleAccess(asIScriptEngine *e, const char *groupName, const char *module, asBOOL hasAccess);
	AS_API asIScriptModule *  asEngine_GetModule(asIScriptEngine *e, const char *module, asEGMFlags flag);
	AS_API int                asEngine_DiscardModule(asIScriptEngine *e, const char *module);
	AS_API asIScriptFunction *asEngine_GetFunctionDescriptorById(asIScriptEngine *e, int funcId);
	AS_API int                asEngine_GetTypeIdByDecl(asIScriptEngine *e, const char *decl);
	AS_API const char *       asEngine_GetTypeDeclaration(asIScriptEngine *e, int typeId);
	AS_API int                asEngine_GetSizeOfPrimitiveType(asIScriptEngine *e, int typeId);
	AS_API asIObjectType *    asEngine_GetObjectTypeById(asIScriptEngine *e, int typeId);
	AS_API asIObjectType *    asEngine_GetObjectTypeByIndex(asIScriptEngine *e, asUINT index);
	AS_API int                asEngine_GetObjectTypeCount(asIScriptEngine *e);
	AS_API asIScriptContext * asEngine_CreateContext(asIScriptEngine *e);
	AS_API void *             asEngine_CreateScriptObject(asIScriptEngine *e, int typeId);
	AS_API void *             asEngine_CreateScriptObjectCopy(asIScriptEngine *e, void *obj, int typeId);
	AS_API void               asEngine_CopyScriptObject(asIScriptEngine *e, void *dstObj, void *srcObj, int typeId);
	AS_API void               asEngine_ReleaseScriptObject(asIScriptEngine *e, void *obj, int typeId);
	AS_API void               asEngine_AddRefScriptObject(asIScriptEngine *e, void *obj, int typeId);
	AS_API asBOOL             asEngine_IsHandleCompatibleWithObject(asIScriptEngine *e, void *obj, int objTypeId, int handleTypeId);
	AS_API int                asEngine_ExecuteString(asIScriptEngine *e, const char *module, const char *script, asIScriptContext **ctx, asDWORD flags);
	AS_API int                asEngine_GarbageCollect(asIScriptEngine *e, asDWORD flags);
	AS_API void               asEngine_GetGCStatistics(asIScriptEngine *e, asUINT *currentSize, asUINT *totalDestroyed, asUINT *totalDetected);
	AS_API void               asEngine_NotifyGarbageCollectorOfNewObject(asIScriptEngine *e, void *obj, int typeId);
	AS_API void               asEngine_GCEnumCallback(asIScriptEngine *e, void *obj);
	AS_API void *             asEngine_SetUserData(asIScriptEngine *e, void *data);
	AS_API void *             asEngine_GetUserData(asIScriptEngine *e);

	AS_API asIScriptEngine   *asModule_GetEngine(asIScriptModule *m);
	AS_API void               asModule_SetName(asIScriptModule *m, const char *name);
	AS_API const char        *asModule_GetName(asIScriptModule *m); 
	AS_API int                asModule_AddScriptSection(asIScriptModule *m, const char *name, const char *code, size_t codeLength, int lineOffset);
	AS_API int                asModule_Build(asIScriptModule *m);
	AS_API int                asModule_GetFunctionCount(asIScriptModule *m);
	AS_API int                asModule_GetFunctionIdByIndex(asIScriptModule *m, int index);
	AS_API int                asModule_GetFunctionIdByName(asIScriptModule *m, const char *name);
	AS_API int                asModule_GetFunctionIdByDecl(asIScriptModule *m, const char *decl);
	AS_API asIScriptFunction *asModule_GetFunctionDescriptorByIndex(asIScriptModule *m, int index);
	AS_API asIScriptFunction *asModule_GetFunctionDescriptorById(asIScriptModule *m, int funcId);
	AS_API int                asModule_ResetGlobalVars(asIScriptModule *m);
	AS_API int                asModule_GetGlobalVarCount(asIScriptModule *m);
	AS_API int                asModule_GetGlobalVarIndexByName(asIScriptModule *m, const char *name);
	AS_API int                asModule_GetGlobalVarIndexByDecl(asIScriptModule *m, const char *decl);
	AS_API const char        *asModule_GetGlobalVarDeclaration(asIScriptModule *m, int index);
	AS_API const char        *asModule_GetGlobalVarName(asIScriptModule *m, int index);
	AS_API int                asModule_GetGlobalVarTypeId(asIScriptModule *m, int index, asBOOL *isConst);
	AS_API void              *asModule_GetAddressOfGlobalVar(asIScriptModule *m, int index);
	AS_API int                asModule_GetObjectTypeCount(asIScriptModule *m);
	AS_API asIObjectType     *asModule_GetObjectTypeByIndex(asIScriptModule *m, asUINT index);
	AS_API int                asModule_GetTypeIdByDecl(asIScriptModule *m, const char *decl);
	AS_API int                asModule_GetEnumCount(asIScriptModule *m);
	AS_API const char *       asModule_GetEnumByIndex(asIScriptModule *m, asUINT index, int *enumTypeId);
	AS_API int                asModule_GetEnumValueCount(asIScriptModule *m, int enumTypeId);
	AS_API const char *       asModule_GetEnumValueByIndex(asIScriptModule *m, int enumTypeId, asUINT index, int *outValue);
	AS_API int                asModule_GetTypedefCount(asIScriptModule *m);
	AS_API const char *       asModule_GetTypedefByIndex(asIScriptModule *m, asUINT index, int *typeId);
	AS_API int                asModule_GetImportedFunctionCount(asIScriptModule *m);
	AS_API int                asModule_GetImportedFunctionIndexByDecl(asIScriptModule *m, const char *decl);
	AS_API const char        *asModule_GetImportedFunctionDeclaration(asIScriptModule *m, int importIndex);
	AS_API const char        *asModule_GetImportedFunctionSourceModule(asIScriptModule *m, int importIndex);
	AS_API int                asModule_BindImportedFunction(asIScriptModule *m, int importIndex, int funcId);
	AS_API int                asModule_UnbindImportedFunction(asIScriptModule *m, int importIndex);
	AS_API int                asModule_BindAllImportedFunctions(asIScriptModule *m);
	AS_API int                asModule_UnbindAllImportedFunctions(asIScriptModule *m);
	AS_API int                asModule_SaveByteCode(asIScriptModule *m, asIBinaryStream *out);
	AS_API int                asModule_LoadByteCode(asIScriptModule *m, asIBinaryStream *in);

	AS_API int              asContext_AddRef(asIScriptContext *c);
	AS_API int              asContext_Release(asIScriptContext *c);
	AS_API asIScriptEngine *asContext_GetEngine(asIScriptContext *c);
	AS_API int              asContext_GetState(asIScriptContext *c);
	AS_API int              asContext_Prepare(asIScriptContext *c, int funcID);
	AS_API int              asContext_SetArgByte(asIScriptContext *c, asUINT arg, asBYTE value);
	AS_API int              asContext_SetArgWord(asIScriptContext *c, asUINT arg, asWORD value);
	AS_API int              asContext_SetArgDWord(asIScriptContext *c, asUINT arg, asDWORD value);
	AS_API int              asContext_SetArgQWord(asIScriptContext *c, asUINT arg, asQWORD value);
	AS_API int              asContext_SetArgFloat(asIScriptContext *c, asUINT arg, float value);
	AS_API int              asContext_SetArgDouble(asIScriptContext *c, asUINT arg, double value);
	AS_API int              asContext_SetArgAddress(asIScriptContext *c, asUINT arg, void *addr);
	AS_API int              asContext_SetArgObject(asIScriptContext *c, asUINT arg, void *obj);
	AS_API void *           asContext_GetAddressOfArg(asIScriptContext *c, asUINT arg);
	AS_API int              asContext_SetObject(asIScriptContext *c, void *obj);
	AS_API asBYTE           asContext_GetReturnByte(asIScriptContext *c);
	AS_API asWORD           asContext_GetReturnWord(asIScriptContext *c);
	AS_API asDWORD          asContext_GetReturnDWord(asIScriptContext *c);
	AS_API asQWORD          asContext_GetReturnQWord(asIScriptContext *c);
	AS_API float            asContext_GetReturnFloat(asIScriptContext *c);
	AS_API double           asContext_GetReturnDouble(asIScriptContext *c);
	AS_API void *           asContext_GetReturnAddress(asIScriptContext *c);
	AS_API void *           asContext_GetReturnObject(asIScriptContext *c);
	AS_API void *           asContext_GetAddressOfReturnValue(asIScriptContext *c);
	AS_API int              asContext_Execute(asIScriptContext *c);
	AS_API int              asContext_Abort(asIScriptContext *c);
	AS_API int              asContext_Suspend(asIScriptContext *c);
	AS_API int              asContext_GetCurrentLineNumber(asIScriptContext *c, int *column);
	AS_API int              asContext_GetCurrentFunction(asIScriptContext *c);
	AS_API int              asContext_SetException(asIScriptContext *c, const char *string);
	AS_API int              asContext_GetExceptionLineNumber(asIScriptContext *c, int *column);
	AS_API int              asContext_GetExceptionFunction(asIScriptContext *c);
	AS_API const char *     asContext_GetExceptionString(asIScriptContext *c);
	AS_API int              asContext_SetLineCallback(asIScriptContext *c, asFUNCTION_t callback, void *obj, int callConv);
	AS_API void             asContext_ClearLineCallback(asIScriptContext *c);
	AS_API int              asContext_SetExceptionCallback(asIScriptContext *c, asFUNCTION_t callback, void *obj, int callConv);
	AS_API void             asContext_ClearExceptionCallback(asIScriptContext *c);
	AS_API int              asContext_GetCallstackSize(asIScriptContext *c);
	AS_API int              asContext_GetCallstackFunction(asIScriptContext *c, int index);
	AS_API int              asContext_GetCallstackLineNumber(asIScriptContext *c, int index, int *column);
	AS_API int              asContext_GetVarCount(asIScriptContext *c, int stackLevel);
	AS_API const char *     asContext_GetVarName(asIScriptContext *c, int varIndex, int stackLevel);
	AS_API const char *     asContext_GetVarDeclaration(asIScriptContext *c, int varIndex, int stackLevel);
	AS_API int              asContext_GetVarTypeId(asIScriptContext *c, int varIndex, int stackLevel);
	AS_API void *           asContext_GetAddressOfVar(asIScriptContext *c, int varIndex, int stackLevel);
	AS_API int              asContext_GetThisTypeId(asIScriptContext *c, int stackLevel);
	AS_API void *           asContext_GetThisPointer(asIScriptContext *c, int stackLevel);
	AS_API void *           asContext_SetUserData(asIScriptContext *c, void *data);
	AS_API void *           asContext_GetUserData(asIScriptContext *c);

	AS_API asIScriptEngine *asGeneric_GetEngine(asIScriptGeneric *g);
	AS_API int              asGeneric_GetFunctionId(asIScriptGeneric *g);
	AS_API void *           asGeneric_GetObject(asIScriptGeneric *g);
	AS_API int              asGeneric_GetObjectTypeId(asIScriptGeneric *g);
	AS_API int              asGeneric_GetArgCount(asIScriptGeneric *g);
	AS_API asBYTE           asGeneric_GetArgByte(asIScriptGeneric *g, asUINT arg);
	AS_API asWORD           asGeneric_GetArgWord(asIScriptGeneric *g, asUINT arg);
	AS_API asDWORD          asGeneric_GetArgDWord(asIScriptGeneric *g, asUINT arg);
	AS_API asQWORD          asGeneric_GetArgQWord(asIScriptGeneric *g, asUINT arg);
	AS_API float            asGeneric_GetArgFloat(asIScriptGeneric *g, asUINT arg);
	AS_API double           asGeneric_GetArgDouble(asIScriptGeneric *g, asUINT arg);
	AS_API void *           asGeneric_GetArgAddress(asIScriptGeneric *g, asUINT arg);
	AS_API void *           asGeneric_GetArgObject(asIScriptGeneric *g, asUINT arg);
	AS_API void *           asGeneric_GetAddressOfArg(asIScriptGeneric *g, asUINT arg);
	AS_API int              asGeneric_GetArgTypeId(asIScriptGeneric *g, asUINT arg);
	AS_API int              asGeneric_SetReturnByte(asIScriptGeneric *g, asBYTE val);
	AS_API int              asGeneric_SetReturnWord(asIScriptGeneric *g, asWORD val);
	AS_API int              asGeneric_SetReturnDWord(asIScriptGeneric *g, asDWORD val);
	AS_API int              asGeneric_SetReturnQWord(asIScriptGeneric *g, asQWORD val);
	AS_API int              asGeneric_SetReturnFloat(asIScriptGeneric *g, float val);
	AS_API int              asGeneric_SetReturnDouble(asIScriptGeneric *g, double val);
	AS_API int              asGeneric_SetReturnAddress(asIScriptGeneric *g, void *addr);
	AS_API int              asGeneric_SetReturnObject(asIScriptGeneric *g, void *obj);
	AS_API void *           asGeneric_GetAddressOfReturnLocation(asIScriptGeneric *g);
	AS_API int              asGeneric_GetReturnTypeId(asIScriptGeneric *g);

	AS_API asIScriptEngine *asObject_GetEngine(asIScriptObject *s);
	AS_API int              asObject_AddRef(asIScriptObject *s);
	AS_API int              asObject_Release(asIScriptObject *s);
	AS_API int              asObject_GetTypeId(asIScriptObject *s);
	AS_API asIObjectType *  asObject_GetObjectType(asIScriptObject *s);
	AS_API int              asObject_GetPropertyCount(asIScriptObject *s);
	AS_API int              asObject_GetPropertyTypeId(asIScriptObject *s, asUINT prop);
	AS_API const char *     asObject_GetPropertyName(asIScriptObject *s, asUINT prop);
	AS_API void *           asObject_GetPropertyPointer(asIScriptObject *s, asUINT prop);
	AS_API int              asObject_CopyFrom(asIScriptObject *s, asIScriptObject *other);

	AS_API asIScriptEngine *asArray_GetEngine(asIScriptArray *a);
	AS_API int              asArray_AddRef(asIScriptArray *a);
	AS_API int              asArray_Release(asIScriptArray *a);
	AS_API int              asArray_GetArrayTypeId(asIScriptArray *a);
	AS_API int              asArray_GetElementTypeId(asIScriptArray *a);
	AS_API asUINT           asArray_GetElementCount(asIScriptArray *a);
	AS_API void *           asArray_GetElementPointer(asIScriptArray *a, asUINT index);
	AS_API void             asArray_Resize(asIScriptArray *a, asUINT size);
	AS_API int              asArray_CopyFrom(asIScriptArray *a, asIScriptArray *other);

	AS_API asIScriptEngine         *asObjectType_GetEngine(const asIObjectType *o);
	AS_API int                      asObjectType_AddRef(asIObjectType *o);
	AS_API int                      asObjectType_Release(asIObjectType *o);
	AS_API const char              *asObjectType_GetName(const asIObjectType *o);
	AS_API asIObjectType           *asObjectType_GetBaseType(const asIObjectType *o);
	AS_API asDWORD                  asObjectType_GetFlags(const asIObjectType *o);
	AS_API asUINT                   asObjectType_GetSize(const asIObjectType *o);
	AS_API int                      asObjectType_GetTypeId(const asIObjectType *o);
	AS_API int                      asObjectType_GetSubTypeId(const asIObjectType *o);
	AS_API int                      asObjectType_GetBehaviourCount(const asIObjectType *o);
	AS_API int                      asObjectType_GetBehaviourByIndex(const asIObjectType *o, asUINT index, asEBehaviours *outBehaviour);
	AS_API int                      asObjectType_GetInterfaceCount(const asIObjectType *o);
	AS_API asIObjectType           *asObjectType_GetInterface(const asIObjectType *o, asUINT index);
	AS_API int                      asObjectType_GetFactoryCount(const asIObjectType *o);
	AS_API int                      asObjectType_GetFactoryIdByIndex(const asIObjectType *o, int index);
	AS_API int                      asObjectType_GetFactoryIdByDecl(const asIObjectType *o, const char *decl);
	AS_API int                      asObjectType_GetMethodCount(const asIObjectType *o);
	AS_API int                      asObjectType_GetMethodIdByIndex(const asIObjectType *o, int index);
	AS_API int                      asObjectType_GetMethodIdByName(const asIObjectType *o, const char *name);
	AS_API int                      asObjectType_GetMethodIdByDecl(const asIObjectType *o, const char *decl);
	AS_API asIScriptFunction       *asObjectType_GetMethodDescriptorByIndex(const asIObjectType *o, int index);
	AS_API int                      asObjectType_GetPropertyCount(const asIObjectType *o);
	AS_API int                      asObjectType_GetPropertyTypeId(const asIObjectType *o, asUINT prop);
	AS_API const char              *asObjectType_GetPropertyName(const asIObjectType *o, asUINT prop);
	AS_API int                      asObjectType_GetPropertyOffset(const asIObjectType *o, asUINT prop);

	AS_API asIScriptEngine     *asScriptFunction_GetEngine(const asIScriptFunction *f);
	AS_API const char          *asScriptFunction_GetModuleName(const asIScriptFunction *f);
	AS_API asIObjectType       *asScriptFunction_GetObjectType(const asIScriptFunction *f);
	AS_API const char          *asScriptFunction_GetObjectName(const asIScriptFunction *f);
	AS_API const char          *asScriptFunction_GetName(const asIScriptFunction *f);
	AS_API const char          *asScriptFunction_GetDeclaration(const asIScriptFunction *f, bool includeObjectName);
	AS_API asBOOL               asScriptFunction_IsClassMethod(const asIScriptFunction *f);
	AS_API asBOOL               asScriptFunction_IsInterfaceMethod(const asIScriptFunction *f);
	AS_API int                  asScriptFunction_GetParamCount(const asIScriptFunction *f);
	AS_API int                  asScriptFunction_GetParamTypeId(const asIScriptFunction *f, int index);
	AS_API int                  asScriptFunction_GetReturnTypeId(const asIScriptFunction *f);
#ifdef __cplusplus
}
#endif
#endif // ANGELSCRIPT_DLL_MANUAL_IMPORT

END_AS_NAMESPACE

#endif
