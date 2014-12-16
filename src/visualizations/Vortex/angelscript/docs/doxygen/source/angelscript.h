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
// angelscript.h
//
// The script engine interface
//


//! \file angelscript.h
//! \brief The API definition for AngelScript.
//!
//! This header file describes the complete application programming interface for AngelScript.

#ifndef ANGELSCRIPT_H
#define ANGELSCRIPT_H

#include <stddef.h>

#ifdef AS_USE_NAMESPACE
 #define BEGIN_AS_NAMESPACE namespace AngelScript {
 #define END_AS_NAMESPACE }
#else
 #define BEGIN_AS_NAMESPACE
 #define END_AS_NAMESPACE
#endif

BEGIN_AS_NAMESPACE

// AngelScript version

//! \details Version 2.18.0
#define ANGELSCRIPT_VERSION        21800
#define ANGELSCRIPT_VERSION_STRING "2.18.0"

// Data types

class asIScriptEngine;
class asIScriptModule;
class asIScriptContext;
class asIScriptGeneric;
class asIScriptObject;
class asIScriptArray;
class asIObjectType;
class asIScriptFunction;
class asIBinaryStream;
class asIJITCompiler;

// Enumerations and constants

// Engine properties
//! Engine properties
enum asEEngineProp
{
	//! Allow unsafe references. Default: false.
	asEP_ALLOW_UNSAFE_REFERENCES      = 1,
	//! Optimize byte code. Default: true.
	asEP_OPTIMIZE_BYTECODE            = 2,
	//! Copy script section memory. Default: true.
	asEP_COPY_SCRIPT_SECTIONS         = 3,
	//! Maximum stack size for script contexts. Default: 0 (no limit).
	asEP_MAX_STACK_SIZE               = 4,
	//! Interpret single quoted strings as character literals. Default: false.
	asEP_USE_CHARACTER_LITERALS       = 5,
	//! Allow linebreaks in string constants. Default: false.
	asEP_ALLOW_MULTILINE_STRINGS      = 6,
	//! Allow script to declare implicit handle types. Default: false.
	asEP_ALLOW_IMPLICIT_HANDLE_TYPES  = 7,
	//! Remove SUSPEND instructions between each statement. Default: false.
	asEP_BUILD_WITHOUT_LINE_CUES      = 8,
	//! Initialize global variables after a build. Default: true.
	asEP_INIT_GLOBAL_VARS_AFTER_BUILD = 9,
	//! When set the enum values must be prefixed with the enum type. Default: false.
	asEP_REQUIRE_ENUM_SCOPE           = 10,
	//! Select scanning method: 0 - ASCII, 1 - UTF8. Default: 1 (UTF8).
	asEP_SCRIPT_SCANNER               = 11,
	//! When set extra bytecode instructions needed for JIT compiled funcions will be included. Default: false.
	asEP_INCLUDE_JIT_INSTRUCTIONS     = 12,
	//! Select string encoding for literals: 0 - UTF8/ASCII, 1 - UTF16. Default: 0 (UTF8)
	asEP_STRING_ENCODING              = 13
};

// Calling conventions
//! Calling conventions
enum asECallConvTypes
{
	//! A cdecl function.
	asCALL_CDECL            = 0,
	//! A stdcall function.
	asCALL_STDCALL          = 1,
	//! A thiscall class method.
	asCALL_THISCALL         = 2,
	//! A cdecl function that takes the object pointer as the last parameter.
	asCALL_CDECL_OBJLAST    = 3,
	//! A cdecl function that takes the object pointer as the first parameter.
	asCALL_CDECL_OBJFIRST   = 4,
	//! A function using the generic calling convention.
	asCALL_GENERIC          = 5
};

// Object type flags
//! Object type flags
enum asEObjTypeFlags
{
	//! A reference type.
	asOBJ_REF                   = 0x01,
	//! A value type.
	asOBJ_VALUE                 = 0x02,
	//! A garbage collected type. Only valid for reference types.
	asOBJ_GC                    = 0x04,
	//! A plain-old-data type. Only valid for value types.
	asOBJ_POD                   = 0x08,
	//! This reference type doesn't allow handles to be held. Only valid for reference types.
	asOBJ_NOHANDLE              = 0x10,
	//! The life time of objects of this type are controlled by the scope of the variable. Only valid for reference types.
	asOBJ_SCOPED                = 0x20,
	//! A template type.
	asOBJ_TEMPLATE              = 0x40,
	//! The C++ type is a class type. Only valid for value types.
	asOBJ_APP_CLASS             = 0x100,
	//! The C++ class has an explicit constructor. Only valid for value types.
	asOBJ_APP_CLASS_CONSTRUCTOR = 0x200,
	//! The C++ class has an explicit destructor. Only valid for value types.
	asOBJ_APP_CLASS_DESTRUCTOR  = 0x400,
	//! The C++ class has an explicit assignment operator. Only valid for value types.
	asOBJ_APP_CLASS_ASSIGNMENT  = 0x800,
	//! The C++ type is a class with a constructor, but no destructor or assignment operator.
	asOBJ_APP_CLASS_C           = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_CONSTRUCTOR),
	//! The C++ type is a class with a constructor and destructor, but no assignment operator.
	asOBJ_APP_CLASS_CD          = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_CONSTRUCTOR + asOBJ_APP_CLASS_DESTRUCTOR),
	//! The C++ type is a class with a constructor and assignment operator, but no destructor.
	asOBJ_APP_CLASS_CA          = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_CONSTRUCTOR + asOBJ_APP_CLASS_ASSIGNMENT),
	//! The C++ type is a class with a constructor, destructor, and assignment operator.
	asOBJ_APP_CLASS_CDA         = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_CONSTRUCTOR + asOBJ_APP_CLASS_DESTRUCTOR + asOBJ_APP_CLASS_ASSIGNMENT),
	//! The C++ type is a class with a destructor, but no constructor or assignment operator.
	asOBJ_APP_CLASS_D           = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_DESTRUCTOR),
	//! The C++ type is a class with an assignment operator, but no constructor or destructor.
	asOBJ_APP_CLASS_A           = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_ASSIGNMENT),
	//! The C++ type is a class with a destructor and assignment operator, but no constructor.
	asOBJ_APP_CLASS_DA          = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_DESTRUCTOR + asOBJ_APP_CLASS_ASSIGNMENT),
	//! The C++ type is a primitive type. Only valid for value types.
	asOBJ_APP_PRIMITIVE         = 0x1000,
	//! The C++ type is a float or double. Only valid for value types.
	asOBJ_APP_FLOAT             = 0x2000,
	asOBJ_MASK_VALID_FLAGS      = 0x3F7F,
	//! The object is a script class or an interface.
	asOBJ_SCRIPT_OBJECT         = 0x10000
};

// Behaviours
//! Behaviours
enum asEBehaviours
{
	// Value object memory management
	//! \brief Constructor
	asBEHAVE_CONSTRUCT,
	//! \brief Destructor
	asBEHAVE_DESTRUCT,

	// Reference object memory management
	//! \brief Factory
	asBEHAVE_FACTORY,
	//! \brief AddRef
	asBEHAVE_ADDREF,
	//! \brief Release
	asBEHAVE_RELEASE,

	// Object operators
	//! \brief Explicit value cast operator
	asBEHAVE_VALUE_CAST,
	//! \brief Implicit value cast operator
	asBEHAVE_IMPLICIT_VALUE_CAST,
	//! \brief Explicit reference cast operator
	asBEHAVE_REF_CAST,
	//! \brief Implicit reference cast operator
	asBEHAVE_IMPLICIT_REF_CAST,
	//! \brief operator []
	asBEHAVE_INDEX,
	//! \brief Callback for validating template instances
	asBEHAVE_TEMPLATE_CALLBACK,

	// Garbage collection behaviours
	asBEHAVE_FIRST_GC,
	//! \brief (GC) Get reference count
	 asBEHAVE_GETREFCOUNT = asBEHAVE_FIRST_GC,
	 //! \brief (GC) Set GC flag
	 asBEHAVE_SETGCFLAG,
	 //! \brief (GC) Get GC flag
	 asBEHAVE_GETGCFLAG,
	 //! \brief (GC) Enumerate held references
	 asBEHAVE_ENUMREFS,
	 //! \brief (GC) Release all references
	 asBEHAVE_RELEASEREFS,
	asBEHAVE_LAST_GC = asBEHAVE_RELEASEREFS,

	asBEHAVE_MAX
};

// Return codes
//! Return codes
enum asERetCodes
{
	//! Success
	asSUCCESS                              =  0,
	//! Failure
	asERROR                                = -1,
	//! The context is active
	asCONTEXT_ACTIVE                       = -2,
	//! The context is not finished
	asCONTEXT_NOT_FINISHED                 = -3,
	//! The context is not prepared
	asCONTEXT_NOT_PREPARED                 = -4,
	//! Invalid argument
	asINVALID_ARG                          = -5,
	//! The function was not found
	asNO_FUNCTION                          = -6,
	//! Not supported
	asNOT_SUPPORTED                        = -7,
	//! Invalid name
	asINVALID_NAME                         = -8,
	//! The name is already taken
	asNAME_TAKEN                           = -9,
	//! Invalid declaration
	asINVALID_DECLARATION                  = -10,
	//! Invalid object
	asINVALID_OBJECT                       = -11,
	//! Invalid type
	asINVALID_TYPE                         = -12,
	//! Already registered
	asALREADY_REGISTERED                   = -13,
	//! Multiple matching functions
	asMULTIPLE_FUNCTIONS                   = -14,
	//! The module was not found
	asNO_MODULE                            = -15,
	//! The global variable was not found
	asNO_GLOBAL_VAR                        = -16,
	//! Invalid configuration
	asINVALID_CONFIGURATION                = -17,
	//! Invalid interface
	asINVALID_INTERFACE                    = -18,
	//! All imported functions couldn't be bound
	asCANT_BIND_ALL_FUNCTIONS              = -19,
	//! The array sub type has not been registered yet
	asLOWER_ARRAY_DIMENSION_NOT_REGISTERED = -20,
	//! Wrong configuration group
	asWRONG_CONFIG_GROUP                   = -21,
	//! The configuration group is in use
	asCONFIG_GROUP_IS_IN_USE               = -22,
	//! Illegal behaviour for the type
	asILLEGAL_BEHAVIOUR_FOR_TYPE           = -23,
	//! The specified calling convention doesn't match the function/method pointer
	asWRONG_CALLING_CONV                   = -24,
	//! A build is currently in progress
	asBUILD_IN_PROGRESS                    = -25,
	//! The initialization of global variables failed
	asINIT_GLOBAL_VARS_FAILED              = -26
};

// Context states

//! \brief Context states.
enum asEContextState
{
	//! The context has successfully completed the execution.
    asEXECUTION_FINISHED      = 0,
    //! The execution is suspended and can be resumed.
    asEXECUTION_SUSPENDED     = 1,
    //! The execution was aborted by the application.
    asEXECUTION_ABORTED       = 2,
    //! The execution was terminated by an unhandled script exception.
    asEXECUTION_EXCEPTION     = 3,
    //! The context has been prepared for a new execution.
    asEXECUTION_PREPARED      = 4,
    //! The context is not initialized.
    asEXECUTION_UNINITIALIZED = 5,
    //! The context is currently executing a function call.
    asEXECUTION_ACTIVE        = 6,
    //! The context has encountered an error and must be reinitialized.
    asEXECUTION_ERROR         = 7
};

#ifdef AS_DEPRECATED
// Deprecated since 2.18.0, 2009-12-08
// ExecuteString flags
//! \brief ExecuteString flags.
//! \deprecated since 2.18.0
enum asEExecStrFlags
{
	//! Only prepare the context
	asEXECSTRING_ONLY_PREPARE   = 1,
	//! Use the pre-allocated context
	asEXECSTRING_USE_MY_CONTEXT = 2
};
#endif

// Message types

//! \brief Compiler message types.
enum asEMsgType
{
	//! The message is an error.
    asMSGTYPE_ERROR       = 0,
    //! The message is a warning.
    asMSGTYPE_WARNING     = 1,
    //! The message is informational only.
    asMSGTYPE_INFORMATION = 2
};

// Garbage collector flags

//! \brief Garbage collector flags.
enum asEGCFlags
{
	//! Execute a full cycle.
	asGC_FULL_CYCLE      = 1,
	//! Execute only one step
	asGC_ONE_STEP        = 2,
	//! Destroy known garbage
	asGC_DESTROY_GARBAGE = 4,
	//! Detect garbage with circular references
	asGC_DETECT_GARBAGE  = 8
};

// Token classes
//! \brief Token classes.
enum asETokenClass
{
	//! Unknown token.
	asTC_UNKNOWN    = 0,
	//! Keyword token.
	asTC_KEYWORD    = 1,
	//! Literal value token.
	asTC_VALUE      = 2,
	//! Identifier token.
	asTC_IDENTIFIER = 3,
	//! Comment token.
	asTC_COMMENT    = 4,
	//! White space token.
	asTC_WHITESPACE = 5
};

// Prepare flags
const int asPREPARE_PREVIOUS = -1;

// Config groups
const char * const asALL_MODULES = (const char * const)-1;

// Type id flags
//! \brief Type id flags
enum asETypeIdFlags
{
	//! The type id for void
	asTYPEID_VOID           = 0,
	//! The type id for bool
	asTYPEID_BOOL           = 1,
	//! The type id for int8
	asTYPEID_INT8           = 2,
	//! The type id for int16
	asTYPEID_INT16          = 3,
	//! The type id for int
	asTYPEID_INT32          = 4,
	//! The type id for int64
	asTYPEID_INT64          = 5,
	//! The type id for uint8
	asTYPEID_UINT8          = 6,
	//! The type id for uint16
	asTYPEID_UINT16         = 7,
	//! The type id for uint
	asTYPEID_UINT32         = 8,
	//! The type id for uint64
	asTYPEID_UINT64         = 9,
	//! The type id for float
	asTYPEID_FLOAT          = 10,
	//! The type id for double
	asTYPEID_DOUBLE         = 11,
	//! The bit that shows if the type is a handle
	asTYPEID_OBJHANDLE      = 0x40000000,
	//! The bit that shows if the type is a handle to a const
	asTYPEID_HANDLETOCONST  = 0x20000000,
	//! If any of these bits are set, then the type is an object
	asTYPEID_MASK_OBJECT    = 0x1C000000,
	//! The bit that shows if the type is an application registered type
	asTYPEID_APPOBJECT      = 0x04000000,
	//! The bit that shows if the type is a script class
	asTYPEID_SCRIPTOBJECT   = 0x08000000,
	//! The bit that shows if the type is a script array
	asTYPEID_SCRIPTARRAY    = 0x10000000,
	//! The mask for the type id sequence number
	asTYPEID_MASK_SEQNBR    = 0x03FFFFFF
};

// Type modifiers
//! \brief Type modifiers
enum asETypeModifiers
{
	//! No modification
	asTM_NONE     = 0,
	//! Input reference
	asTM_INREF    = 1,
	//! Output reference
	asTM_OUTREF   = 2,
	//! In/out reference
	asTM_INOUTREF = 3
};

// GetModule flags
//! \brief Flags for GetModule.
enum asEGMFlags
{
	//! \brief Don't return any module if it is not found.
	asGM_ONLY_IF_EXISTS       = 0,
	//! \brief Create the module if it doesn't exist.
	asGM_CREATE_IF_NOT_EXISTS = 1,
	//! \brief Always create a new module, discarding the existing one.
	asGM_ALWAYS_CREATE        = 2
};

// Compile flags
//! \brief Flags for compilation
enum asECompileFlags
{
	//! \brief The compiled function should be added to the scope of the module.
	asCOMP_ADD_TO_MODULE = 1
};



//! \typedef asBYTE
//! \brief 8 bit unsigned integer

//! \typedef asWORD
//! \brief 16 bit unsigned integer

//! \typedef asDWORD
//! \brief 32 bit unsigned integer

//! \typedef asQWORD
//! \brief 64 bit unsigned integer

//! \typedef asUINT
//! \brief 32 bit unsigned integer

//! \typedef asINT64
//! \brief 64 bit integer

//! \typedef asPWORD
//! \brief Unsigned integer with the size of a pointer.

//
// asBYTE  =  8 bits
// asWORD  = 16 bits
// asDWORD = 32 bits
// asQWORD = 64 bits
// asPWORD = size of pointer
//
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

typedef void (*asFUNCTION_t)();
typedef void (*asGENFUNC_t)(asIScriptGeneric *);

//! The function signature for the custom memory allocation function
typedef void *(*asALLOCFUNC_t)(size_t);
//! The function signature for the custom memory deallocation function
typedef void (*asFREEFUNC_t)(void *);

//! \ingroup funcs
//! \brief Returns an asSFuncPtr representing the function specified by the name
#define asFUNCTION(f) asFunctionPtr(f)
//! \ingroup funcs
//! \brief Returns an asSFuncPtr representing the function specified by the name, parameter list, and return type
#if defined(_MSC_VER) && _MSC_VER <= 1200
// MSVC 6 has a bug that prevents it from properly compiling using the correct asFUNCTIONPR with operator >
// so we need to use ordinary C style cast instead of static_cast. The drawback is that the compiler can't 
// check that the cast is really valid.
#define asFUNCTIONPR(f,p,r) asFunctionPtr((void (*)())((r (*)p)(f)))
#else
#define asFUNCTIONPR(f,p,r) asFunctionPtr((void (*)())(static_cast<r (*)p>(f)))
#endif

#ifndef AS_NO_CLASS_METHODS

class asCUnknownClass;
typedef void (asCUnknownClass::*asMETHOD_t)();

//! \brief Represents a function or method pointer.
struct asSFuncPtr
{
	union
	{
		// The largest known method point is 20 bytes (MSVC 64bit),
		// but with 8byte alignment this becomes 24 bytes. So we need
		// to be able to store at least that much.
		char dummy[25]; 
		struct {asMETHOD_t   mthd; char dummy[25-sizeof(asMETHOD_t)];} m;
		struct {asFUNCTION_t func; char dummy[25-sizeof(asFUNCTION_t)];} f;
	} ptr;
	asBYTE flag; // 1 = generic, 2 = global func, 3 = method
};

//! \ingroup funcs
//! \brief Returns an asSFuncPtr representing the class method specified by class and method name.
#define asMETHOD(c,m) asSMethodPtr<sizeof(void (c::*)())>::Convert((void (c::*)())(&c::m))
//! \ingroup funcs
//! \brief Returns an asSFuncPtr representing the class method specified by class, method name, parameter list, return type.
#define asMETHODPR(c,m,p,r) asSMethodPtr<sizeof(void (c::*)())>::Convert(static_cast<r (c::*)p>(&c::m))

#else // Class methods are disabled

struct asSFuncPtr
{
	union
	{
		char dummy[25]; // largest known class method pointer
		struct {asFUNCTION_t func; char dummy[25-sizeof(asFUNCTION_t)];} f;
	} ptr;
	asBYTE flag; // 1 = generic, 2 = global func
};

#endif

//! \brief Represents a compiler message
struct asSMessageInfo
{
	//! The script section where the message is raised
	const char *section;
	//! The row number
	int         row;
	//! The column
	int         col;
	//! The type of message
	asEMsgType  type;
	//! The message text
	const char *message;
};


// API functions

// ANGELSCRIPT_EXPORT is defined when compiling the dll or lib
// ANGELSCRIPT_DLL_LIBRARY_IMPORT is defined when dynamically linking to the
// dll through the link lib automatically generated by MSVC++
// ANGELSCRIPT_DLL_MANUAL_IMPORT is defined when manually loading the dll
// Don't define anything when linking statically to the lib

//! \def AS_API
//! \brief A define that specifies how the function should be imported

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
extern "C"
{
	// Engine
	//! \brief Creates the script engine.
	//!
	//! \param[in] version The library version. Should always be \ref ANGELSCRIPT_VERSION.
	//! \return A pointer to the script engine interface.
	//!
	//! Call this function to create a new script engine. When you're done with the
	//! script engine, i.e. after you've executed all your scripts, you should call
	//! \ref asIScriptEngine::Release "Release" on the pointer to free the engine object.
	AS_API asIScriptEngine * asCreateScriptEngine(asDWORD version);
	//! \brief Returns the version of the compiled library.
	//!
	//! \return A null terminated string with the library version.
	//!
	//! The returned string can be used for presenting the library version in a log file, or in the GUI.
	AS_API const char * asGetLibraryVersion();
	//! \brief Returns the options used to compile the library.
	//!
	//! \return A null terminated string with indicators that identify the options
	//!         used to compile the script library.
	//!
	//! This can be used to identify at run-time different ways to configure the engine.
	//! For example, if the returned string contain the identifier AS_MAX_PORTABILITY then
	//! functions and methods must be registered with the \ref asCALL_GENERIC calling convention.
	AS_API const char * asGetLibraryOptions();

	// Context
	//! \brief Returns the currently active context.
	//!
	//! \return A pointer to the currently executing context, or null if no context is executing.
	//!
	//! This function is most useful for registered functions, as it will allow them to obtain
	//! a pointer to the context that is calling the function, and through that get the engine,
	//! or custom user data.
	//!
	//! If the script library is compiled with multithread support, this function will return
	//! the context that is currently active in the thread that is being executed. It will thus
	//! work even if there are multiple threads executing scripts at the same time.
	AS_API asIScriptContext * asGetActiveContext();

	// Thread support
	//! \brief Cleans up memory allocated for the current thread.
	//!
	//! \return A negative value on error.
	//! \retval asCONTEXT_ACTIVE A context is still active.
	//!
	//! Call this method before terminating a thread that has
	//! accessed the engine to clean up memory allocated for that thread.
	//!
	//! It's not necessary to call this if only a single thread accesses the engine.
	AS_API int asThreadCleanup();

	// Memory management
	//! \brief Set the memory management functions that AngelScript should use.
	//!
	//! \param[in] allocFunc The function that will be used to allocate memory.
	//! \param[in] freeFunc The function that will be used to free the memory.
	//! \return A negative value on error.
	//!
	//! Call this method to register the global memory allocation and deallocation
	//! functions that AngelScript should use for memory management. This function
	//! Should be called before \ref asCreateScriptEngine.
	//!
	//! If not called, AngelScript will use the malloc and free functions from the
	//! standard C library.
	AS_API int asSetGlobalMemoryFunctions(asALLOCFUNC_t allocFunc, asFREEFUNC_t freeFunc);

	//! \brief Remove previously registered memory management functions.
	//!
	//! \return A negative value on error.
	//!
	//! Call this method to restore the default memory management functions.
	AS_API int asResetGlobalMemoryFunctions();
}
#endif // ANGELSCRIPT_DLL_MANUAL_IMPORT

// Interface declarations

//! \brief The engine interface
class asIScriptEngine
{
public:
	// Memory management
    //! \name Memory management
    //! \{

	//! \brief Increase reference counter.
	//!
	//! \return The number of references to this object.
	//!
	//! Call this method when storing an additional reference to the object.
	//! Remember that the first reference that is received from \ref asCreateScriptEngine
	//! is already accounted for.
	virtual int AddRef() = 0;
	//! \brief Decrease reference counter.
	//!
	//! \return The number of references to this object.
	//!
	//! Call this method when you will no longer use the references that you own.
	virtual int Release() = 0;
	//! \}

	// Engine properties
    //! \name Engine properties
    //! \{

	//! \brief Dynamically change some engine properties.
	//!
	//! \param[in] property One of the \ref asEEngineProp values.
	//! \param[in] value The new value of the property.
	//! \return Negative value on error.
	//! \retval asINVALID_ARG Invalid property.
	//!
	//! With this method you can change the way the script engine works in some regards.
	virtual int     SetEngineProperty(asEEngineProp property, asPWORD value) = 0;
	//! \brief Retrieve current engine property settings.
	//!
	//! \param[in] property One of the \ref asEEngineProp values.
	//! \return The value of the property, or 0 if it is an invalid property.
	//!
	//! Calling this method lets you determine the current value of the engine properties.
	virtual asPWORD GetEngineProperty(asEEngineProp property) = 0;
	//! \}

	// Compiler messages
    //! \name Compiler messages
    //! \{

	//! \brief Sets a message callback that will receive compiler messages.
	//!
	//! \param[in] callback A function or class method pointer.
	//! \param[in] obj      The object for methods, or an optional parameter for functions.
	//! \param[in] callConv The calling convention.
	//! \return A negative value for an error.
	//! \retval asINVALID_ARG   One of the arguments is incorrect, e.g. obj is null for a class method.
	//! \retval asNOT_SUPPORTED The arguments are not supported, e.g. asCALL_GENERIC.
	//!
	//! This method sets the callback routine that will receive compiler messages.
	//! The callback routine can be either a class method, e.g:
	//! \code
	//! void MyClass::MessageCallback(const asSMessageInfo *msg);
	//! r = engine->SetMessageCallback(asMETHOD(MyClass,MessageCallback), &obj, asCALL_THISCALL);
	//! \endcode
	//! or a global function, e.g:
	//! \code
	//! void MessageCallback(const asSMessageInfo *msg, void *param);
	//! r = engine->SetMessageCallback(asFUNCTION(MessageCallback), param, asCALL_CDECL);
	//! \endcode
	//! It is recommended to register the message callback routine right after creating the engine,
	//! as some of the registration functions can provide useful information to better explain errors.
	virtual int SetMessageCallback(const asSFuncPtr &callback, void *obj, asDWORD callConv) = 0;
	//! \brief Clears the registered message callback routine.
	//!
	//! \return A negative value on error.
	//!
	//! Call this method to remove the message callback.
	virtual int ClearMessageCallback() = 0;
	//! \brief Writes a message to the message callback.
	//!
	//! \param[in] section The name of the script section.
	//! \param[in] row The row number.
	//! \param[in] col The column number.
	//! \param[in] type The message type.
	//! \param[in] message The message text.
	//! \return A negative value on error.
	//! \retval asINVALID_ARG The section or message is null.
	//! 
	//! This method can be used by the application to write messages
	//! to the same message callback that the script compiler uses. This
	//! is useful for example if a preprocessor is used.
	virtual int WriteMessage(const char *section, int row, int col, asEMsgType type, const char *message) = 0;
	//! \}

	// JIT Compiler
	//! \name JIT compiler
	//! \{

	//! \brief Sets the JIT compiler
	virtual int SetJITCompiler(asIJITCompiler *compiler) = 0;
	//! \brief Returns the JIT compiler
	virtual asIJITCompiler *GetJITCompiler() = 0;
	//! \}

	// Global functions
    //! \name Global functions
    //! \{

	//! \brief Registers a global function.
    //!
    //! \param[in] declaration The declaration of the global function in script syntax.
    //! \param[in] funcPointer The function pointer.
    //! \param[in] callConv The calling convention for the function.
    //! \return A negative value on error, or the function id if successful.
    //! \retval asNOT_SUPPORTED The calling convention is not supported.
    //! \retval asWRONG_CALLING_CONV The function's calling convention doesn't match \a callConv.
    //! \retval asINVALID_DECLARATION The function declaration is invalid.
    //! \retval asNAME_TAKEN The function name is already used elsewhere.
    //!
    //! This method registers system functions that the scripts may use to communicate with the host application.
    //! 
    //! \see \ref doc_register_func
	virtual int RegisterGlobalFunction(const char *declaration, const asSFuncPtr &funcPointer, asDWORD callConv) = 0;
	//! \brief Returns the number of registered functions.
	//! \return The number of registered functions.
	virtual int GetGlobalFunctionCount() = 0;
	//! \brief Returns the function id of the registered function.
	//! \param[in] index The index of the registered global function.
	//! \return The id of the function, or a negative value on error.
	//! \retval asINVALID_ARG \a index is too large.
	virtual int GetGlobalFunctionIdByIndex(asUINT index) = 0;
	//! \}

	// Global properties
    //! \name Global properties
    //! \{

	//! \brief Registers a global property.
    //!
    //! \param[in] declaration The declaration of the global property in script syntax.
    //! \param[in] pointer The address of the property that will be used to access the property value.
    //! \return A negative value on error.
    //! \retval asINVALID_DECLARATION The declaration has invalid syntax.
    //! \retval asINVALID_TYPE The declaration is a reference.
    //! \retval asNAME_TAKEN The name is already taken.
    //!
    //! Use this method to register a global property that the scripts will be
    //! able to access as global variables. The property may optionally be registered
    //! as const, if the scripts shouldn't be allowed to modify it.
    //!
    //! When registering the property, the application must pass the address to
    //! the actual value. The application must also make sure that this address
    //! remains valid throughout the life time of this registration, i.e. until
    //! the engine is released or the dynamic configuration group is removed.
	virtual int RegisterGlobalProperty(const char *declaration, void *pointer) = 0;
	//! \brief Returns the number of registered global properties.
	//! \return The number of registered global properties.
	virtual int GetGlobalPropertyCount() = 0;
	//! \brief Returns the detail on the registered global property.
	//! \param[in] index The index of the global variable.
	//! \param[out] name Receives the name of the property.
	//! \param[out] typeId Receives the typeId of the property.
	//! \param[out] isConst Receives the constness indicator of the property.
	//! \param[out] configGroup Receives the config group in which the property was registered.
	//! \param[out] pointer Receives the pointer of the property.
	//! \return A negative value on error.
	//! \retval asINVALID_ARG \a index is too large.
	virtual int GetGlobalPropertyByIndex(asUINT index, const char **name, int *typeId = 0, bool *isConst = 0, const char **configGroup = 0, void **pointer = 0) = 0;
	//! \}

	// Object types
    //! \name Object types
    //! \{

	//! \brief Registers a new object type.
    //!
    //! \param[in] obj The name of the type.
    //! \param[in] byteSize The size of the type in bytes. Only necessary for value types.
    //! \param[in] flags One or more of the asEObjTypeFlags.
    //! \return A negative value on error.
    //! \retval asINVALID_ARG The flags are invalid.
    //! \retval asINVALID_NAME The name is invalid.
    //! \retval asALREADY_REGISTERED Another type of the same name already exists.
    //! \retval asNAME_TAKEN The name conflicts with other symbol names.
    //! \retval asLOWER_ARRAY_DIMENSION_NOT_REGISTERED When registering an array type the array element must be a primitive or a registered type.
    //! \retval asINVALID_TYPE The array type was not properly formed.
    //! \retval asNOT_SUPPORTED The array type is not supported, or already in use preventing it from being overloaded.
    //!
    //! Use this method to register new types that should be available to the scripts.
    //! Reference types, which have their memory managed by the application, should be registered with \ref asOBJ_REF.
    //! Value types, which have their memory managed by the engine, should be registered with \ref asOBJ_VALUE.
    //!
    //! \see \ref doc_register_type
	virtual int            RegisterObjectType(const char *obj, int byteSize, asDWORD flags) = 0;
	//! \brief Registers a property for the object type.
    //!
    //! \param[in] obj The name of the type.
    //! \param[in] declaration The property declaration in script syntax.
    //! \param[in] byteOffset The offset into the memory block where this property is found.
    //! \return A negative value on error.
    //! \retval asWRONG_CONFIG_GROUP The object type was registered in a different configuration group.
    //! \retval asINVALID_OBJECT The \a obj does not specify an object type.
    //! \retval asINVALID_TYPE The \a obj parameter has invalid syntax.
    //! \retval asINVALID_DECLARATION The \a declaration is invalid.
    //! \retval asNAME_TAKEN The name conflicts with other members.
    //!
    //! Use this method to register a member property of a class. The property must
    //! be local to the object, i.e. not a global variable or a static member. The
    //! easiest way to get the offset of the property is to use the offsetof macro
    //! from stddef.h.
    //!
    //! \code
    //! struct MyType {float prop;};
    //! r = engine->RegisterObjectProperty("MyType", "float prop", offsetof(MyType, prop)));
    //! \endcode
	virtual int            RegisterObjectProperty(const char *obj, const char *declaration, int byteOffset) = 0;
	//! \brief Registers a method for the object type.
    //!
    //! \param[in] obj The name of the type.
    //! \param[in] declaration The declaration of the method in script syntax.
    //! \param[in] funcPointer The method or function pointer.
    //! \param[in] callConv The calling convention for the method or function.
    //! \return A negative value on error, or the function id if successful.
    //! \retval asWRONG_CONFIG_GROUP The object type was registered in a different configuration group.
    //! \retval asNOT_SUPPORTED The calling convention is not supported.
    //! \retval asINVALID_TYPE The \a obj parameter is not a valid object name.
    //! \retval asINVALID_DECLARATION The \a declaration is invalid.
    //! \retval asNAME_TAKEN The name conflicts with other members.
    //! \retval asWRONG_CALLING_CONV The function's calling convention isn't compatible with \a callConv.
    //!
    //! Use this method to register a member method for the type. The method
    //! that is registered may be an actual class method, or a global function
    //! that takes the object pointer as either the first or last parameter. Or
    //! it may be a global function implemented with the generic calling convention.
    //!
    //! \see \ref doc_register_func
	virtual int            RegisterObjectMethod(const char *obj, const char *declaration, const asSFuncPtr &funcPointer, asDWORD callConv) = 0;
	//! \brief Registers a behaviour for the object type.
    //!
    //! \param[in] obj The name of the type.
    //! \param[in] behaviour One of the object behaviours from \ref asEBehaviours.
    //! \param[in] declaration The declaration of the method in script syntax.
    //! \param[in] funcPointer The method or function pointer.
    //! \param[in] callConv The calling convention for the method or function.
    //! \return A negative value on error, or the function id is successful.
    //! \retval asWRONG_CONFIG_GROUP The object type was registered in a different configuration group.
    //! \retval asINVALID_ARG \a obj is not set, or a global behaviour is given in \a behaviour.
    //! \retval asWRONG_CALLING_CONV The function's calling convention isn't compatible with \a callConv.
    //! \retval asNOT_SUPPORTED The calling convention or the behaviour signature is not supported.
    //! \retval asINVALID_TYPE The \a obj parameter is not a valid object name.
    //! \retval asINVALID_DECLARATION The \a declaration is invalid.
    //! \retval asILLEGAL_BEHAVIOUR_FOR_TYPE The \a behaviour is not allowed for this type.
    //! \retval asALREADY_REGISTERED The behaviour is already registered with the same signature.
    //!
    //! Use this method to register behaviour functions that will be called by
    //! the virtual machine to perform certain operations, such as memory management,
    //! math operations, comparisons, etc.
    //!
    //! \see \ref doc_register_func, \ref doc_reg_opbeh
	virtual int            RegisterObjectBehaviour(const char *obj, asEBehaviours behaviour, const char *declaration, const asSFuncPtr &funcPointer, asDWORD callConv) = 0;
	//! \brief Registers an interface.
    //!
    //! \param[in] name The name of the interface.
    //! \return A negative value on error.
    //! \retval asINVALID_NAME The \a name is null, or a reserved keyword.
    //! \retval asALREADY_REGISTERED An object type with this name already exists.
    //! \retval asERROR The \a name is not a proper identifier.
    //! \retval asNAME_TAKEN The \a name is already used elsewhere.
    //!
    //! This registers an interface that script classes can implement. By doing this the application 
    //! can register functions and methods that receives an \ref asIScriptObject and still be sure that the 
    //! class implements certain methods needed by the application. 
	virtual int            RegisterInterface(const char *name) = 0;
	//! \brief Registers an interface method.
    //!
    //! \param[in] intf The name of the interface.
    //! \param[in] declaration The method declaration.
    //! \return A negative value on error.
    //! \retval asWRONG_CONFIG_GROUP The interface was registered in another configuration group.
    //! \retval asINVALID_TYPE \a intf is not an interface type.
    //! \retval asINVALID_DECLARATION The \a declaration is invalid.
    //! \retval asNAME_TAKEN The method name is already taken.
    //!
    //! This registers a method that the class that implements the interface must have.
	virtual int            RegisterInterfaceMethod(const char *intf, const char *declaration) = 0;
	//! \brief Returns the number of registered object types.
    //! \return The number of object types registered by the application.
	virtual int            GetObjectTypeCount() = 0;
	//! \brief Returns the object type interface by index.
    //! \param[in] index The index of the type.
    //! \return The registered object type interface for the type, or null if not found.
	virtual asIObjectType *GetObjectTypeByIndex(asUINT index) = 0;
	//! \}

	// String factory
    //! \name String factory
    //! \{

    //! \brief Registers the string factory.
    //!
    //! \param[in] datatype The datatype that the string factory returns
    //! \param[in] factoryFunc The pointer to the factory function
    //! \param[in] callConv The calling convention of the factory function
    //! \return A negative value on error, or the function id if successful.
    //! \retval asNOT_SUPPORTED The calling convention is not supported.
    //! \retval asWRONG_CALLING_CONV The function's calling convention doesn't match \a callConv.
    //! \retval asINVALID_TYPE The \a datatype is not a valid type.
    //!
    //! Use this function to register a string factory that will be called when the 
    //! virtual machine finds a string constant in an expression. The string factory 
    //! function will receive two parameters, the length of the string constant in bytes and a 
    //! pointer to the character data. The factory should return a value to a previously 
    //! registered type that will represent the string. Example:
    //!
    //! \code
    //! // Our string factory implementation
    //! std::string StringFactory(unsigned int byteLength, const char *s)
    //! {
    //!     return std::string(s, byteLength);
    //! }
    //!
    //! // Registering the string factory
    //! int r = engine->RegisterStringFactory("string", asFUNCTION(StringFactory), asCALL_CDECL); assert( r >= 0 );
    //! \endcode
    //!
    //! The example assumes that the std::string type has been registered as the string type, with \ref RegisterObjectType.
	virtual int RegisterStringFactory(const char *datatype, const asSFuncPtr &factoryFunc, asDWORD callConv) = 0;
    //! \brief Returns the type id of the type that the string factory returns.
    //! \return The type id of the type that the string type returns, or a negative value on error.
    //! \retval asNO_FUNCTION The string factory has not been registered.
	virtual int GetStringFactoryReturnTypeId() = 0;
	//! \}

	// Enums
    //! \name Enums
    //! \{

	//! \brief Registers an enum type.
    //!
    //! \param[in] type The name of the enum type.
    //! \return A negative value on error.
    //! \retval asINVALID_NAME \a type is null.
    //! \retval asALREADY_REGISTERED Another type with this name already exists.
    //! \retval asERROR The \a type couldn't be parsed.
    //! \retval asINVALID_NAME The \a type is not an identifier, or it is a reserved keyword.
    //! \retval asNAME_TAKEN The type name is already taken.
    //!
    //! This method registers an enum type in the engine. The enum values should then be registered 
    //! with \ref RegisterEnumValue.
	virtual int         RegisterEnum(const char *type) = 0;
	//! \brief Registers an enum value.
    //!
    //! \param[in] type The name of the enum type.
    //! \param[in] name The name of the enum value.
    //! \param[in] value The integer value of the enum value.
    //! \return A negative value on error.
    //! \retval asWRONG_CONFIG_GROUP The enum \a type was registered in a different configuration group.
    //! \retval asINVALID_TYPE The \a type is invalid.
    //! \retval asALREADY_REGISTERED The \a name is already registered for this enum.
    //!
    //! This method registers an enum value for a previously registered enum type.
	virtual int         RegisterEnumValue(const char *type, const char *name, int value) = 0;
	//! \brief Returns the number of registered enum types.
	//! \return The number of registered enum types.
	virtual int         GetEnumCount() = 0;
	//! \brief Returns the registered enum type.
	//! \param[in] index The index of the enum type.
	//! \param[out] enumTypeId Receives the type if of the enum type.
	//! \param[out] configGroup Receives the config group in which the enum was registered.
	//! \return The name of the registered enum type, or null on error.
	virtual const char *GetEnumByIndex(asUINT index, int *enumTypeId, const char **configGroup = 0) = 0;
	//! \brief Returns the number of enum values for the enum type.
	//! \param[in] enumTypeId The type id of the enum type.
	//! \return The number of enum values for the enum type.
	virtual int         GetEnumValueCount(int enumTypeId) = 0;
	//! \brief Returns the name and value of the enum value for the enum type.
	//! \param[in] enumTypeId The type id of the enum type.
	//! \param[in] index The index of the enum value.
	//! \param[out] outValue Receives the value of the enum value.
	//! \return The name of the enum value.
	virtual const char *GetEnumValueByIndex(int enumTypeId, asUINT index, int *outValue) = 0;
	//! \}

	// Typedefs
    //! \name Typedefs
    //! \{

	//! \brief Registers a typedef.
    //!
    //! \param[in] type The name of the new typedef
    //! \param[in] decl The datatype that the typedef represents
    //! \return A negative value on error.
    //! \retval asINVALID_NAME The \a type is null.
    //! \retval asALREADY_REGISTERED A type with the same name already exists.
    //! \retval asINVALID_TYPE The \a decl is not a primitive type.
    //! \retval asINVALID_NAME The \a type is not an identifier, or it is a reserved keyword.
    //! \retval asNAME_TAKEN The name is already used elsewhere.
    //!
    //! This method registers an alias for a data type.
    //!
    //! Currently typedefs can only be registered for built-in primitive types.
	virtual int         RegisterTypedef(const char *type, const char *decl) = 0;
	//! \brief Returns the number of registered typedefs.
	//! \return The number of registered typedefs.
	virtual int         GetTypedefCount() = 0;
	//! \brief Returns a registered typedef.
	//! \param[in] index The index of the typedef.
	//! \param[out] typeId The type that the typedef aliases.
	//! \param[out] configGroup Receives the config group in which the type def was registered.
	//! \return The name of the typedef.
	virtual const char *GetTypedefByIndex(asUINT index, int *typeId, const char **configGroup = 0) = 0;
	//! \}

	// Configuration groups
    //! \name Configuration groups
    //! \{

	//! \brief Starts a new dynamic configuration group.
    //!
    //! \param[in] groupName The name of the configuration group
    //! \return A negative value on error
    //! \retval asNAME_TAKEN Another group with the same name already exists.
    //! \retval asNOT_SUPPORTED Nesting configuration groups is not supported.
    //!
    //! Starts a new dynamic configuration group. This group can be setup so that it is only 
    //! visible to specific modules, and it can also be removed when it is no longer used.
	virtual int BeginConfigGroup(const char *groupName) = 0;
	//! \brief Ends the configuration group.
    //!
    //! \return A negative value on error
    //! \retval asNOT_SUPPORTED Can't end a group that hasn't been begun.
    //!
    //! Ends the current configuration group. Once finished a config group cannot be changed, 
    //! but it can be removed when it is no longer used.
	virtual int EndConfigGroup() = 0;
	//! \brief Removes a previously registered configuration group.
    //!
    //! \param[in] groupName The name of the configuration group
    //! \return A negative value on error
    //! \retval asCONFIG_GROUP_IS_IN_USE The group is in use and cannot be removed.
    //!
    //! Remove the configuration group. If something in the configuration group is currently in 
    //! use, the function will return with an error code. Examples of uses are compiled modules 
    //! that have function calls to functions in the group and global variables of types registered 
    //! in the group.
	virtual int RemoveConfigGroup(const char *groupName) = 0;
	//! \brief Tell AngelScript which modules have access to which configuration groups.
    //!
    //! \param[in] groupName The name of the configuration group
    //! \param[in] module The module name
    //! \param[in] hasAccess Whether the module has access or not to the group members
    //! \return A negative value on error
    //! \retval asWRONG_CONFIG_GROUP No group with the \a groupName was found.
    //!
    //! With this method the application can give modules access to individual configuration groups. 
    //! This is useful when exposing more than one script interface for various parts of the application, 
    //! e.g. one interface for GUI handling, another for in-game events, etc. 
    //!
    //! The default module access is granted. The default for a group can be changed by specifying 
    //! the modulename asALL_MODULES. 
	virtual int SetConfigGroupModuleAccess(const char *groupName, const char *module, bool hasAccess) = 0;
	//! \}

	// Script modules
    //! \name Script modules
    //! \{

	//! \brief Return an interface pointer to the module.
	//!
	//! \param[in] module The name of the module
	//! \param[in] flag One of the \ref asEGMFlags flags
	//! \return A pointer to the module interface
	//!
	//! Use this method to get access to the module interface, which will
	//! let you build new scripts, and enumerate functions and types in
	//! existing modules.
	//!
	//! If \ref asGM_ALWAYS_CREATE is informed as the flag the previous
	//! module with the same name will be discarded, thus any pointers that
	//! the engine holds to it will be invalid after the call.
	virtual asIScriptModule *GetModule(const char *module, asEGMFlags flag = asGM_ONLY_IF_EXISTS) = 0;
	//! \brief Discard a module.
    //!
    //! \param[in] module The name of the module
    //! \return A negative value on error
    //! \retval asNO_MODULE The module was not found.
    //!
    //! Discards a module and frees its memory. Any pointers that the application holds 
	//! to this module will be invalid after this call.
	virtual int              DiscardModule(const char *module) = 0;
	//! \}

	// Script functions
    //! \name Script functions
    //! \{

	//! \brief Returns the function descriptor for the script function
    //! \param[in] funcId The id of the function or method.
    //! \return A pointer to the function description interface, or null if not found.
	virtual asIScriptFunction *GetFunctionDescriptorById(int funcId) = 0;
	//! \}

	// Type identification
    //! \name Type identification
    //! \{

	//! \brief Returns the object type interface for type.
    //! \param[in] typeId The type id of the type.
    //! \return The object type interface for the type, or null if not found.
	virtual asIObjectType *GetObjectTypeById(int typeId) = 0;
	//! \brief Returns a type id by declaration.
	//! \param[in] decl The declaration of the type.
	//! \return A negative value on error, or the type id of the type.
	//! \retval asINVALID_TYPE \a decl is not a valid type.
	//!
	//! Translates a type declaration into a type id. The returned type id is valid for as long as
	//! the type is valid, so you can safely store it for later use to avoid potential overhead by 
	//! calling this function each time. Just remember to update the type id, any time the type is 
	//! changed within the engine, e.g. when recompiling script declared classes, or changing the 
	//! engine configuration.
	//! 
	//! The type id is based on a sequence number and depends on the order in which the type ids are
	//! queried, thus is not guaranteed to always be the same for each execution of the application.
	//! The \ref asETypeIdFlags can be used to obtain some information about the type directly from the id.
	//! 
	//! A base type yields the same type id whether the declaration is const or not, however if the
	//! const is for the subtype then the type id is different, e.g. string@ isn't the same as const
	//! string@ but string is the same as const string.
	//! 
	//! This method is only able to return the type id that are not specific for a script module, i.e.
	//! built-in types and application registered types. Type ids for script declared types should
	//! be obtained through the script module's \ref asIScriptModule::GetTypeIdByDecl "GetTypeIdByDecl".
	virtual int            GetTypeIdByDecl(const char *decl) = 0;
	//! \brief Returns a type declaration.
    //! \param[in] typeId The type id of the type.
    //! \return A null terminated string with the type declaration, or null if not found.
	virtual const char    *GetTypeDeclaration(int typeId) = 0;
	//! \brief Returns the size of a primitive type.
    //! \param[in] typeId The type id of the type.
    //! \return The size of the type in bytes.
	virtual int            GetSizeOfPrimitiveType(int typeId) = 0;
	//! \}

	// Script execution
    //! \name Script execution
    //! \{

	//! \brief Creates a new script context.
    //! \return A pointer to the new script context.
    //!
    //! This method creates a context that will be used to execute the script functions. 
    //! The context interface created will have its reference counter already increased.
	virtual asIScriptContext *CreateContext() = 0;
	//! \brief Creates a script object defined by its type id.
    //! \param[in] typeId The type id of the object to create.
    //! \return A pointer to the new object if successful, or null if not.
    //! 
    //! This method is used to create a script object based on it's type id. The method will 
    //! allocate the memory and call the object's default constructor. Reference counted
    //! objects will have their reference counter set to 1 so the application needs to 
    //! release the pointer when it will no longer use it.
    //!
    //! This only works for objects, for primitive types and object handles the method 
    //! doesn't do anything and returns a null pointer.
	virtual void             *CreateScriptObject(int typeId) = 0;
	//! \brief Creates a copy of a script object.
    //! \param[in] obj A pointer to the source object.
    //! \param[in] typeId The type id of the object.
    //! \return A pointer to the new object if successful, or null if not.
    //!
    //! This method is used to create a copy of an existing object.
    //!
    //! This only works for objects, for primitive types and object handles the method 
    //! doesn't do anything and returns a null pointer.
	virtual void             *CreateScriptObjectCopy(void *obj, int typeId) = 0;
	//! \brief Copy one script object to another.
    //! \param[in] dstObj A pointer to the destination object.
    //! \param[in] srcObj A pointer to the source object.
    //! \param[in] typeId The type id of the objects.
    //!
    //! This calls the assignment operator to copy the object from one to the other.
    //! 
    //! This only works for objects.
	virtual void              CopyScriptObject(void *dstObj, void *srcObj, int typeId) = 0;
	//! \brief Release the script object pointer.
    //! \param[in] obj A pointer to the object.
    //! \param[in] typeId The type id of the object.
    //!
    //! This calls the release method of the object to release the reference.
    //! 
    //! This only works for objects.
	virtual void              ReleaseScriptObject(void *obj, int typeId) = 0;
	//! \brief Increase the reference counter for the script object.
    //! \param[in] obj A pointer to the object.
    //! \param[in] typeId The type id of the object.
    //!
    //! This calls the add ref method of the object to increase the reference count.
    //! 
    //! This only works for objects.
	virtual void              AddRefScriptObject(void *obj, int typeId) = 0;
	//! \brief Returns true if the object referenced by a handle compatible with the specified type.
    //! \param[in] obj A pointer to the object.
    //! \param[in] objTypeId The type id of the object.
    //! \param[in] handleTypeId The type id of the handle.
    //! \return Returns true if the handle type is compatible with the object type.
    //!
    //! This method can be used to determine if a handle of a certain type is 
    //! compatible with an object of another type. This is useful if you have a pointer 
    //! to a object, but only knows that it implements a certain interface and now you 
    //! want to determine if it implements another interface.
	virtual bool              IsHandleCompatibleWithObject(void *obj, int objTypeId, int handleTypeId) = 0;
	//! \}

	// String interpretation
    //! \name String interpretation
    //! \{

	//! \brief Returns the class and length of the first token in the string.
	//! \param[in] string The string to parse.
	//! \param[in] stringLength The length of the string. Can be 0 if the string is null terminated.
	//! \param[out] tokenLength Gives the length of the identified token.
	//! \return One of the \ref asETokenClass values.
	//!
	//! This function is useful for those applications that want to tokenize strings into 
	//! tokens that the script language uses, e.g. IDEs providing syntax highlighting, or intellisense.
	//! It can also be used to parse the meta data strings that may be declared for script entities.
	virtual asETokenClass ParseToken(const char *string, size_t stringLength = 0, int *tokenLength = 0) = 0;
	//! \}

	// Garbage collection
    //! \name Garbage collection
    //! \{

	//! \brief Perform garbage collection.
    //! \param[in] flags Set to a combination of the \ref asEGCFlags.
    //! \return 1 if the cycle wasn't completed, 0 if it was.
    //!
    //! This method will free script objects that can no longer be reached. When the engine 
    //! is released the garbage collector will automatically do a full cycle to release all 
    //! objects still alive. If the engine is long living it is important to call this method 
    //! every once in a while to free up memory allocated by the scripts. If a script does a 
    //! lot of allocations before returning it may be necessary to implement a line callback 
    //! function that calls the garbage collector during execution of the script.
    //! 
    //! It is not necessary to do a full cycle with every call. This makes it possible to spread 
    //! out the garbage collection time over a large period, thus not impacting the responsiveness 
    //! of the application.
	//!
	//! \see \ref doc_gc
	virtual int  GarbageCollect(asDWORD flags = asGC_FULL_CYCLE) = 0;
	//! \brief Obtain statistics from the garbage collector.
	//! \param[out] currentSize The current number of objects known to the garbage collector.
	//! \param[out] totalDestroyed The total number of objects destroyed by the garbage collector.
	//! \param[out] totalDetected The total number of objects detected as garbage with circular references.
	//!
	//! This method can be used to query the number of objects that the garbage collector is 
	//! keeping track of. If the number is very large then it is probably time to call the 
	//! \ref GarbageCollect method so that some of the objects ca be freed.
	//!
	//! \see \ref doc_gc
	virtual void GetGCStatistics(asUINT *currentSize, asUINT *totalDestroyed = 0, asUINT *totalDetected = 0) = 0;
	//! \brief Notify the garbage collector of a new object that needs to be managed.
    //! \param[in] obj A pointer to the newly created object.
    //! \param[in] typeId The type id of the object.
    //!
    //! This method should be called when a new garbage collected object is created. 
    //! The GC will then store a reference to the object so that it can automatically 
    //! detect whether the object is involved in any circular references that should be released.
    //!
    //! \see \ref doc_gc_object
	virtual void NotifyGarbageCollectorOfNewObject(void *obj, int typeId) = 0;
	//! \brief Used by the garbage collector to enumerate all references held by an object.
    //! \param[in] reference A pointer to the referenced object.
    //!
    //! When processing the EnumReferences call the called object should call GCEnumCallback 
    //! for each of the references it holds to other objects.
    //!
    //! \see \ref doc_gc_object
	virtual void GCEnumCallback(void *reference) = 0;
	//! \}

	// User data
    //! \name User data
    //! \{

	//! \brief Register the memory address of some user data.
	//! \param[in] data A pointer to the user data.
	//! \return The previous pointer stored in the engine.
	//!
	//! This method allows the application to associate a value, e.g. a pointer, with the engine instance.
	virtual void *SetUserData(void *data) = 0;
	//! \brief Returns the address of the previously registered user data.
	//! \return The pointer to the user data.
	virtual void *GetUserData() = 0;
	//! \}

#ifdef AS_DEPRECATED
	//! \name Deprecated
	//! \{

	// deprecated since 2009-12-08, 2.18.0
	//! \deprecated Since 2.18.0. Use the \ref doc_addon_helpers "ExecuteString" helper function instead.
	virtual int           ExecuteString(const char *module, const char *script, asIScriptContext **ctx = 0, asDWORD flags = 0) = 0;
	//! \}
#endif

protected:
	virtual ~asIScriptEngine() {}
};

//! \brief The interface to the script modules
class asIScriptModule
{
public:
    //! \name Miscellaneous
    //! \{

	//! \brief Returns a pointer to the engine.
    //! \return A pointer to the engine.
	virtual asIScriptEngine *GetEngine() = 0;
	//! \brief Sets the name of the module.
	//! \param[in] name The new name.
	//!
	//! Sets the name of the script module.
	virtual void             SetName(const char *name) = 0;
	//! \brief Gets the name of the module.
	//! \return The name of the module.
	virtual const char      *GetName() = 0;
	//! \}

	// Compilation
    //! \name Compilation
    //! \{

    //! \brief Add a script section for the next build.
    //!
    //! \param[in] name The name of the script section
    //! \param[in] code The script code buffer
    //! \param[in] codeLength The length of the script code
    //! \param[in] lineOffset An offset that will be added to compiler message line numbers
    //! \return A negative value on error.
    //! \retval asMODULE_IS_IN_USE The module is currently in use.
    //!
    //! This adds a script section to the module. All sections added will be treated as if one 
    //! large script. Errors reported will give the name of the corresponding section.
    //! 
    //! The code added is copied by the engine, so there is no need to keep the original buffer after the call.
    //! Note that this can be changed by setting the engine property \ref asEP_COPY_SCRIPT_SECTIONS
    //! with \ref asIScriptEngine::SetEngineProperty.
    virtual int  AddScriptSection(const char *name, const char *code, size_t codeLength = 0, int lineOffset = 0) = 0;
    //! \brief Build the previously added script sections.
    //!
    //! \return A negative value on error
    //! \retval asINVALID_CONFIGURATION The engine configuration is invalid.
    //! \retval asERROR The script failed to build.
    //! \retval asBUILD_IN_PROGRESS Another thread is currently building. 
    //!
    //! Builds the script based on the added sections, and registered types and functions. After the
    //! build is complete the script sections are removed to free memory. If the script module needs 
    //! to be rebuilt all of the script sections needs to be added again.
    //! 
    //! Compiler messages are sent to the message callback function set with \ref asIScriptEngine::SetMessageCallback. 
    //! If there are no errors or warnings, no messages will be sent to the callback function.
    //!
    //! Any global variables found in the script will be initialized by the
    //! compiler if the engine property \ref asEP_INIT_GLOBAL_VARS_AFTER_BUILD is set.
	virtual int  Build() = 0;
	//! \brief Compile a single function.
	//!
	//! \param[in] sectionName The name of the script section
	//! \param[in] code The script code buffer
	//! \param[in] lineOffset An offset that will be added to compiler message line numbers
	//! \param[in] compileFlags One of \ref asECompileFlags values
	//! \param[out] outFunc Optional parameter to receive the compiled function descriptor 
	//! \return A negative value on error
	//! \retval asINVALID_ARG One or more arguments have invalid values.
	//! \retval asINVALID_CONFIGURATION The engine configuration is invalid.
	//! \retval asBUILD_IN_PROGRESS Another build is in progress.
	//! \retval asERROR The compilation failed.
	//! 
	//! Use this to compile a single function. The function can be optionally added to the scope of the module, in which case
	//! it will be available for subsequent compilations. If not added to the module, the function can still be returned in the
	//! output parameter, which will allow the application to execute it, and then discard it when it is no longer needed.
	//!
	//! If the output function parameter is set, remember to release the function object when you're done with it.
	virtual int  CompileFunction(const char *sectionName, const char *code, int lineOffset, asDWORD compileFlags, asIScriptFunction **outFunc) = 0;
	//! \brief Compile a single global variable and add it to the scope of the module
	//!
	//! \param[in] sectionName The name of the script section
	//! \param[in] code The script code buffer
	//! \param[in] lineOffset An offset that will be added to compiler message line numbers
	//! \return A negative value on error
	//! \retval asINVALID_ARG One or more arguments have invalid values.
	//! \retval asINVALID_CONFIGURATION The engine configuration is invalid.
	//! \retval asBUILD_IN_PROGRESS Another build is in progress.
	//! \retval asERROR The compilation failed.
	//!
	//! Use this to add a single global variable to the scope of a module. The variable can then
	//! be referred to by the application and subsequent compilations.
	//!
	//! The script code may contain an initialization expression, which will be executed by the
	//! compiler if the engine property \ref asEP_INIT_GLOBAL_VARS_AFTER_BUILD is set.
	virtual int  CompileGlobalVar(const char *sectionName, const char *code, int lineOffset) = 0;
	//! \}

	// Functions
    //! \name Functions
    //! \{
	
	//! \brief Returns the number of global functions in the module.
    //! \return A negative value on error, or the number of global functions in this module.
    //! \retval asERROR The module was not compiled successfully.
    //!
    //! This method retrieves the number of compiled script functions.
	virtual int                GetFunctionCount() = 0;
	//! \brief Returns the function id by index.
    //! \param[in] index The index of the function.
    //! \return A negative value on error, or the function id.
    //! \retval asNO_FUNCTION There is no function with that index. 
    //!
    //! This method should be used to retrieve the id of the script function that you wish to 
    //! execute. The id is then sent to the context's \ref asIScriptContext::Prepare "Prepare"  method.
	virtual int                GetFunctionIdByIndex(int index) = 0;
	//! \brief Returns the function id by name.
    //! \param[in] name The name of the function.
    //! \return A negative value on error, or the function id.
    //! \retval asERROR The module was not compiled successfully.
    //! \retval asMULTIPLE_FUNCTIONS Found multiple matching functions.
    //! \retval asNO_FUNCTION Didn't find any matching functions.
    //!
    //! This method should be used to retrieve the id of the script function that you 
    //! wish to execute. The id is then sent to the context's \ref asIScriptContext::Prepare "Prepare" method.
	virtual int                GetFunctionIdByName(const char *name) = 0;
	//! \brief Returns the function id by declaration.
    //! \param[in] decl The function signature.
    //! \return A negative value on error, or the function id.
    //! \retval asERROR The module was not compiled successfully.
    //! \retval asINVALID_DECLARATION The \a decl is invalid.
    //! \retval asMULTIPLE_FUNCTIONS Found multiple matching functions.
    //! \retval asNO_FUNCTION Didn't find any matching functions.
    //!
    //! This method should be used to retrieve the id of the script function that you wish 
    //! to execute. The id is then sent to the context's \ref asIScriptContext::Prepare "Prepare" method.
    //!
    //! The method will find the script function with the exact same declaration.
	virtual int                GetFunctionIdByDecl(const char *decl) = 0;
	//! \brief Returns the function descriptor for the script function
    //! \param[in] index The index of the function.
    //! \return A pointer to the function description interface, or null if not found.
	virtual asIScriptFunction *GetFunctionDescriptorByIndex(int index) = 0;
	//! \brief Returns the function descriptor for the script function
    //! \param[in] funcId The id of the function or method.
    //! \return A pointer to the function description interface, or null if not found.
	virtual asIScriptFunction *GetFunctionDescriptorById(int funcId) = 0;
	//! \brief Remove a single function from the scope of the module
	//! \param[in] funcId The id of the function to remove.
	//! \return A negative value on error.
	//! \retval asNO_FUNCTION The function is not part of the scope.
	//!
	//! This method allows the application to remove a single function from the
	//! scope of the module. The function is not destroyed immediately though,
	//! only when no more references point to it.
	virtual int                RemoveFunction(int funcId) = 0;
	//! \}

	// Global variables
    //! \name Global variables
    //! \{
	
	//! \brief Reset the global variables of the module.
    //!
    //! \return A negative value on error.
    //! \retval asERROR The module was not compiled successfully.
    //!
    //! Resets the global variables declared in this module to their initial value.
	virtual int         ResetGlobalVars() = 0;
	//! \brief Returns the number of global variables in the module.
    //! \return A negative value on error, or the number of global variables in the module.
	//! \retval asERROR The module was not compiled successfully.
	//! \retval asINIT_GLOBAL_VARS_FAILED The initialization of the global variables failed.
	virtual int         GetGlobalVarCount() = 0;
	//! \brief Returns the global variable index by name.
    //! \param[in] name The name of the global variable.
    //! \return A negative value on error, or the global variable index.
    //! \retval asERROR The module was not built successfully.
    //! \retval asNO_GLOBAL_VAR The matching global variable was found.
    //!
    //! This method should be used to retrieve the index of the script variable that you wish to access.
	virtual int         GetGlobalVarIndexByName(const char *name) = 0;
	//! \brief Returns the global variable index by declaration.
    //! \param[in] decl The global variable declaration.
    //! \return A negative value on error, or the global variable index.
    //! \retval asERROR The module was not built successfully.
    //! \retval asNO_GLOBAL_VAR The matching global variable was found.
    //!
    //! This method should be used to retrieve the index of the script variable that you wish to access.
    //!
    //! The method will find the script variable with the exact same declaration.
	virtual int         GetGlobalVarIndexByDecl(const char *decl) = 0;
	//! \brief Returns the global variable declaration.
    //! \param[in] index The index of the global variable.
    //! \return A null terminated string with the variable declaration, or null if not found.
    //!
    //! This method can be used to retrieve the variable declaration of the script variables 
    //! that the host application will access. Verifying the declaration is important because, 
    //! even though the script may compile correctly the user may not have used the variable 
    //! types as intended.
	virtual const char *GetGlobalVarDeclaration(int index) = 0;
	//! \brief Returns the global variable name.
    //! \param[in] index The index of the global variable.
    //! \return A null terminated string with the variable name, or null if not found.
	virtual const char *GetGlobalVarName(int index) = 0;
	//! \brief Returns the type id for the global variable.
	//! \param[in] index The index of the global variable.
	//! \param[out] isConst Receives the constness indicator of the variable.
	//! \return The type id of the global variable, or a negative value on error.
	//! \retval asINVALID_ARG The index is out of range.
	virtual int         GetGlobalVarTypeId(int index, bool *isConst = 0) = 0;
	//! \brief Returns the pointer to the global variable.
    //! \param[in] index The index of the global variable.
    //! \return A pointer to the global variable, or null if not found.
    //!
    //! This method should be used to retrieve the pointer of a variable that you wish to access.
	virtual void       *GetAddressOfGlobalVar(int index) = 0;
	//! \brief Remove the global variable from the scope of the module.
	//! \param[in] index The index of the global variable.
	//! \return A negative value on error.
	//! \retval asINVALID_ARG The index is out of range.
	//!
	//! The global variable is removed from the scope of the module, but 
	//! it is not destroyed until all functions that access it are freed.
	virtual int         RemoveGlobalVar(int index) = 0;
	//! \}

	// Type identification
    //! \name Type identification
    //! \{

	//! \brief Returns the number of object types.
    //! \return The number of object types declared in the module.
	virtual int            GetObjectTypeCount() = 0;
	//! \brief Returns the object type interface by index.
    //! \param[in] index The index of the type.
    //! \return The object type interface for the type, or null if not found.
	virtual asIObjectType *GetObjectTypeByIndex(asUINT index) = 0;
	//! \brief Returns a type id by declaration.
    //! \param[in] decl The declaration of the type.
    //! \return A negative value on error, or the type id of the type.
    //! \retval asINVALID_TYPE \a decl is not a valid type.
    //!
    //! Translates a type declaration into a type id. The returned type id is valid for as long as
    //! the type is valid, so you can safely store it for later use to avoid potential overhead by 
    //! calling this function each time. Just remember to update the type id, any time the type is 
    //! changed within the engine, e.g. when recompiling script declared classes, or changing the 
    //! engine configuration.
    //! 
    //! The type id is based on a sequence number and depends on the order in which the type ids are 
    //! queried, thus is not guaranteed to always be the same for each execution of the application.
    //! The \ref asETypeIdFlags can be used to obtain some information about the type directly from the id.
    //! 
    //! A base type yields the same type id whether the declaration is const or not, however if the 
    //! const is for the subtype then the type id is different, e.g. string@ isn't the same as const 
    //! string@ but string is the same as const string. 
	virtual int            GetTypeIdByDecl(const char *decl) = 0;
	//! \}

	// Enums
    //! \name Enums
    //! \{

	//! \brief Returns the number of enum types declared in the module.
	//! \return The number of enum types in the module.
	virtual int         GetEnumCount() = 0;
	//! \brief Returns the enum type.
	//! \param[in] index The index of the enum type.
	//! \param[out] enumTypeId Receives the type id of the enum type.
	//! \return The name of the enum type, or null on error.
	virtual const char *GetEnumByIndex(asUINT index, int *enumTypeId) = 0;
	//! \brief Returns the number of values defined for the enum type.
	//! \param[in] enumTypeId The type id of the enum type.
	//! \return The number of enum values or a negative value on error.
	//! \retval asINVALID_ARG \a enumTypeId is not an enum type.
	virtual int         GetEnumValueCount(int enumTypeId) = 0;
	//! \brief Returns the name and value of the enum value.
	//! \param[in] enumTypeId The type id of the enum type.
	//! \param[in] index The index of the enum value.
	//! \param[out] outValue Receives the numeric value.
	//! \return The name of the enum value.
	virtual const char *GetEnumValueByIndex(int enumTypeId, asUINT index, int *outValue) = 0;
	//! \}

	// Typedefs
    //! \name Typedefs
    //! \{

	//! \brief Returns the number of typedefs in the module.
	//! \return The number of typedefs in the module.
	virtual int         GetTypedefCount() = 0;
	//! \brief Returns the typedef.
	//! \param[in] index The index of the typedef.
	//! \param[out] typeId The type that the typedef aliases.
	//! \return The name of the typedef.
	virtual const char *GetTypedefByIndex(asUINT index, int *typeId) = 0;
	//! \}

	// Dynamic binding between modules
    //! \name Dynamic binding between modules
    //! \{

	//! \brief Returns the number of functions declared for import.
    //! \return A negative value on error, or the number of imported functions.
    //! \retval asERROR The module was not built successfully.
    //!
    //! This function returns the number of functions that are imported in a module. These 
    //! functions need to be bound before they can be used, or a script exception will be thrown.
	virtual int         GetImportedFunctionCount() = 0;
	//! \brief Returns the imported function index by declaration.
    //! \param[in] decl The function declaration of the imported function.
    //! \return A negative value on error, or the index of the imported function.
    //! \retval asERROR The module was not built successfully.
    //! \retval asMULTIPLE_FUNCTIONS Found multiple matching functions.
    //! \retval asNO_FUNCTION Didn't find any matching function.
    //!
    //! This function is used to find a specific imported function by its declaration.
	virtual int         GetImportedFunctionIndexByDecl(const char *decl) = 0;
	//! \brief Returns the imported function declaration.
    //! \param[in] importIndex The index of the imported function.
    //! \return A null terminated string with the function declaration, or null if not found.
    //!
    //! Use this function to get the declaration of the imported function. The returned 
    //! declaration can be used to find a matching function in another module that can be bound 
    //! to the imported function. 
	virtual const char *GetImportedFunctionDeclaration(int importIndex) = 0;
	//! \brief Returns the declared imported function source module.
    //! \param[in] importIndex The index of the imported function.
    //! \return A null terminated string with the name of the source module, or null if not found.
    //!
    //! Use this function to get the name of the suggested module to import the function from.
	virtual const char *GetImportedFunctionSourceModule(int importIndex) = 0;
	//! \brief Binds an imported function to the function from another module.
    //! \param[in] importIndex The index of the imported function.
    //! \param[in] funcId The function id of the function that will be bound to the imported function.
    //! \return A negative value on error.
    //! \retval asNO_FUNCTION \a importIndex or \a fundId is incorrect.
    //! \retval asINVALID_INTERFACE The signature of function doesn't match the import statement.
    //!
    //! The imported function is only bound if the functions have the exact same signature, 
    //! i.e the same return type, and parameters.
	virtual int         BindImportedFunction(int importIndex, int funcId) = 0;
	//! \brief Unbinds an imported function.
    //! \param[in] importIndex The index of the imported function.
    //! \return A negative value on error.
	//! \retval asINVALID_ARG The index is not valid.
    //!
    //! Unbinds the imported function.
	virtual int         UnbindImportedFunction(int importIndex) = 0;

	//! \brief Binds all imported functions in a module, by searching their equivalents in the declared source modules.
    //! \return A negative value on error.
    //! \retval asERROR An error occurred.
    //! \retval asCANT_BIND_ALL_FUNCTIONS Not all functions where bound.
    //!
    //! This functions tries to bind all imported functions in the module by searching for matching 
    //! functions in the suggested modules. If a function cannot be bound the function will give an 
    //! error \ref asCANT_BIND_ALL_FUNCTIONS, but it will continue binding the rest of the functions.
	virtual int         BindAllImportedFunctions() = 0;
	//! \brief Unbinds all imported functions.
    //! \return A negative value on error.
    //!
    //! Unbinds all imported functions in the module.
	virtual int         UnbindAllImportedFunctions() = 0;
	//! \}

	// Bytecode saving and loading
    //! \name Bytecode saving and loading
    //! \{

	//! \brief Save compiled bytecode to a binary stream.
    //! \param[in] out The output stream.
    //! \return A negative value on error.
    //! \retval asINVALID_ARG The stream object wasn't specified.
    //!
    //! This method is used to save pre-compiled byte code to disk or memory, for a later restoral.
    //! The application must implement an object that inherits from \ref asIBinaryStream to provide
    //! the necessary stream operations.
    //!
    //! The pre-compiled byte code is currently not platform independent, so you need to make
    //! sure the byte code is compiled on a platform that is compatible with the one that will load it.	
	virtual int SaveByteCode(asIBinaryStream *out) = 0;
	//! \brief Load pre-compiled bytecode from a binary stream.
    //!
    //! \param[in] in The input stream.
    //! \return A negative value on error.
    //! \retval asINVALID_ARG The stream object wasn't specified.
    //! \retval asBUILD_IN_PROGRESS Another thread is currently building.
    //!
    //! This method is used to load pre-compiled byte code from disk or memory. The application must
    //! implement an object that inherits from \ref asIBinaryStream to provide the necessary stream operations.
    //!
    //! It is expected that the application performs the necessary validations to make sure the
    //! pre-compiled byte code is from a trusted source. The application should also make sure the
    //! pre-compiled byte code is compatible with the current engine configuration, i.e. that the
    //! engine has been configured in the same way as when the byte code was first compiled.
	virtual int LoadByteCode(asIBinaryStream *in) = 0;
	//! \}

protected:
	virtual ~asIScriptModule() {}
};

//! \brief The interface to the virtual machine
class asIScriptContext
{
public:
	// Memory management
	//! \name Memory management
	//! \{

	//! \brief Increase reference counter.
    //!
    //! \return The number of references to this object.
    //!
    //! Call this method when storing an additional reference to the object.
    //! Remember that the first reference that is received from \ref asIScriptEngine::CreateContext
    //! is already accounted for.
	virtual int AddRef() = 0;
	//! \brief Decrease reference counter.
    //!
    //! \return The number of references to this object.
    //!
    //! Call this method when you will no longer use the references that you own.
	virtual int Release() = 0;
	//! \}

	// Miscellaneous
	//! \name Miscellaneous
	//! \{

	//! \brief Returns a pointer to the engine.
    //! \return A pointer to the engine.
	virtual asIScriptEngine *GetEngine() = 0;
	//! \}

	// Execution
	//! \name Execution
	//! \{

    //! \brief Prepares the context for execution of the function identified by funcId.
    //! \param[in] funcId The id of the function/method that will be executed.
    //! \return A negative value on error.
    //! \retval asCONTEXT_ACTIVE The context is still active or suspended.
    //! \retval asNO_FUNCTION The function id doesn't exist.
    //!
    //! This method prepares the context for execution of a script function. It allocates 
    //! the stack space required and reserves space for return value and parameters. The 
    //! default value for parameters and return value is 0.
    //!
    //! \see \ref doc_call_script_func
	virtual int             Prepare(int funcId) = 0;
	//! \brief Frees resources held by the context.
    //! \return A negative value on error.
    //! \retval asCONTEXT_ACTIVE The context is still active or suspended.
    //!
    //! This function frees resources held by the context. It's usually not necessary 
    //! to call this function as the resources are automatically freed when the context
    //! is released, or reused when \ref Prepare is called again.
	virtual int             Unprepare() = 0;
	//! \brief Sets the object for a class method call.
    //! \param[in] obj A pointer to the object.
    //! \return A negative value on error.
    //! \retval asCONTEXT_NOT_PREPARED The context is not in prepared state.
    //! \retval asERROR The prepared function is not a class method.
    //!
    //! This method sets object pointer when calling class methods.
	virtual int             SetObject(void *obj) = 0;
    //! \brief Executes the prepared function.
    //! \return A negative value on error, or one of \ref asEContextState.
    //! \retval asERROR Invalid context object, the context is not prepared, or it is not in suspended state.
    //! \retval asEXECUTION_ABORTED The execution was aborted with a call to \ref Abort.
    //! \retval asEXECUTION_SUSPENDED The execution was suspended with a call to \ref Suspend.
    //! \retval asEXECUTION_FINISHED The execution finished successfully.
    //! \retval asEXECUTION_EXCEPTION The execution ended with an exception.
    //!
    //! Executes the prepared function until the script returns. If the execution was previously 
    //! suspended the function resumes where it left of.
    //! 
    //! Note that if the script freezes, e.g. if trapped in a never ending loop, you may call 
    //! \ref Abort from another thread to stop execution.
    //! 
    //! \see \ref doc_call_script_func
	virtual int             Execute() = 0;
	//! \brief Aborts the execution.
    //! \return A negative value on error.
    //! \retval asERROR Invalid context object.
    //!
    //! Aborts the current execution of a script.
	virtual int             Abort() = 0;
	//! \brief Suspends the execution, which can then be resumed by calling Execute again.
    //! \return A negative value on error.
    //! \retval asERROR Invalid context object.
    //!
    //! Suspends the current execution of a script. The execution can then be resumed by calling \ref Execute again.
    //!
    //! \see \ref doc_call_script_func
	virtual int             Suspend() = 0;
	//! \brief Returns the state of the context.
    //! \return The current state of the context.
	virtual asEContextState GetState() = 0;
	//! \}

	// Arguments
	//! \name Arguments
	//! \{

	//! \brief Sets an 8-bit argument value.
    //! \param[in] arg The argument index.
    //! \param[in] value The value of the argument.
    //! \return A negative value on error.
    //! \retval asCONTEXT_NOT_PREPARED The context is not in prepared state.
    //! \retval asINVALID_ARG The \a arg is larger than the number of arguments in the prepared function.
    //! \retval asINVALID_TYPE The argument is not an 8-bit value.
    //!
    //! Sets a 1 byte argument.
	virtual int   SetArgByte(asUINT arg, asBYTE value) = 0;
	//! \brief Sets a 16-bit argument value.
    //! \param[in] arg The argument index.
    //! \param[in] value The value of the argument.
    //! \return A negative value on error.
    //! \retval asCONTEXT_NOT_PREPARED The context is not in prepared state.
    //! \retval asINVALID_ARG The \a arg is larger than the number of arguments in the prepared function.
    //! \retval asINVALID_TYPE The argument is not a 16-bit value.
    //!
    //! Sets a 2 byte argument.
	virtual int   SetArgWord(asUINT arg, asWORD value) = 0;
	//! \brief Sets a 32-bit integer argument value.
    //! \param[in] arg The argument index.
    //! \param[in] value The value of the argument.
    //! \return A negative value on error.
    //! \retval asCONTEXT_NOT_PREPARED The context is not in prepared state.
    //! \retval asINVALID_ARG The \a arg is larger than the number of arguments in the prepared function.
    //! \retval asINVALID_TYPE The argument is not a 32-bit value.
    //!
    //! Sets a 4 byte argument.
	virtual int   SetArgDWord(asUINT arg, asDWORD value) = 0;
	//! \brief Sets a 64-bit integer argument value.
    //! \param[in] arg The argument index.
    //! \param[in] value The value of the argument.
    //! \return A negative value on error.
    //! \retval asCONTEXT_NOT_PREPARED The context is not in prepared state.
    //! \retval asINVALID_ARG The \a arg is larger than the number of arguments in the prepared function.
    //! \retval asINVALID_TYPE The argument is not a 64-bit value.
    //!
    //! Sets an 8 byte argument.
	virtual int   SetArgQWord(asUINT arg, asQWORD value) = 0;
	//! \brief Sets a float argument value.
    //! \param[in] arg The argument index.
    //! \param[in] value The value of the argument.
    //! \return A negative value on error.
    //! \retval asCONTEXT_NOT_PREPARED The context is not in prepared state.
    //! \retval asINVALID_ARG The \a arg is larger than the number of arguments in the prepared function.
    //! \retval asINVALID_TYPE The argument is not a 32-bit value.
    //!
    //! Sets a float argument.
	virtual int   SetArgFloat(asUINT arg, float value) = 0;
	//! \brief Sets a double argument value.
    //! \param[in] arg The argument index.
    //! \param[in] value The value of the argument.
    //! \return A negative value on error.
    //! \retval asCONTEXT_NOT_PREPARED The context is not in prepared state.
    //! \retval asINVALID_ARG The \a arg is larger than the number of arguments in the prepared function.
    //! \retval asINVALID_TYPE The argument is not a 64-bit value.
    //!
    //! Sets a double argument.
	virtual int   SetArgDouble(asUINT arg, double value) = 0;
	//! \brief Sets the address of a reference or handle argument.
    //! \param[in] arg The argument index.
    //! \param[in] addr The address that should be passed in the argument.
    //! \return A negative value on error.
    //! \retval asCONTEXT_NOT_PREPARED The context is not in prepared state.
    //! \retval asINVALID_ARG The \a arg is larger than the number of arguments in the prepared function.
    //! \retval asINVALID_TYPE The argument is not a reference or an object handle.
    //!
    //! Sets an address argument, e.g. an object handle or a reference.
	virtual int   SetArgAddress(asUINT arg, void *addr) = 0;
	//! \brief Sets the object argument value.
    //! \param[in] arg The argument index.
    //! \param[in] obj A pointer to the object.
    //! \return A negative value on error.
    //! \retval asCONTEXT_NOT_PREPARED The context is not in prepared state.
    //! \retval asINVALID_ARG The \a arg is larger than the number of arguments in the prepared function.
    //! \retval asINVALID_TYPE The argument is not an object or handle.
    //!
    //! Sets an object argument. If the argument is an object handle AngelScript will increment the reference
    //! for the object. If the argument is an object value AngelScript will make a copy of the object.
	virtual int   SetArgObject(asUINT arg, void *obj) = 0;
	//! \brief Returns a pointer to the argument for assignment.
    //! \param[in] arg The argument index.
    //! \return A pointer to the argument on the stack.
    //!
    //! This method returns a pointer to the argument on the stack for assignment. For object handles, you
    //! should increment the reference counter. For object values, you should pass a pointer to a copy of the
    //! object.
	virtual void *GetAddressOfArg(asUINT arg) = 0;
	//! \}	

	// Return value
	//! \name Return value
	//! \{

	//! \brief Returns the 8-bit return value.
    //! \return The 1 byte value returned from the script function, or 0 on error.
	virtual asBYTE  GetReturnByte() = 0;
	//! \brief Returns the 16-bit return value.
    //! \return The 2 byte value returned from the script function, or 0 on error.
	virtual asWORD  GetReturnWord() = 0;
	//! \brief Returns the 32-bit return value.
    //! \return The 4 byte value returned from the script function, or 0 on error.
	virtual asDWORD GetReturnDWord() = 0;
	//! \brief Returns the 64-bit return value.
    //! \return The 8 byte value returned from the script function, or 0 on error.
	virtual asQWORD GetReturnQWord() = 0;
	//! \brief Returns the float return value.
    //! \return The float value returned from the script function, or 0 on error.
	virtual float   GetReturnFloat() = 0;
	//! \brief Returns the double return value.
    //! \return The double value returned from the script function, or 0 on error.
	virtual double  GetReturnDouble() = 0;
	//! \brief Returns the address for a reference or handle return type.
    //! \return The address value returned from the script function, or 0 on error.
	//! 
	//! The method doesn't increase the reference counter with this call, so if you store
	//! the pointer of a reference counted object you need to increase the reference manually
	//! otherwise the object will be released when the context is released or reused.
	virtual void   *GetReturnAddress() = 0;
	//! \brief Return a pointer to the returned object.
    //! \return A pointer to the object returned from the script function, or 0 on error.
	//! 
	//! The method doesn't increase the reference counter with this call, so if you store
	//! the pointer of a reference counted object you need to increase the reference manually
	//! otherwise the object will be released when the context is released or reused.
	virtual void   *GetReturnObject() = 0;
	//! \brief Returns the address of the returned value
	//! \return A pointer to the return value returned from the script function, or 0 on error.
	virtual void   *GetAddressOfReturnValue() = 0;
	//! \}

	// Exception handling
	//! \name Exception handling
	//! \{

 	//! \brief Sets an exception, which aborts the execution.
    //! \param[in] string A string that describes the exception that occurred.
    //! \return A negative value on error.
    //! \retval asERROR The context isn't currently calling an application registered function.
    //!
    //! This method sets a script exception in the context. This will only work if the 
    //! context is currently calling a system function, thus this method can only be 
    //! used for system functions.
    //!
    //! Note that if your system function sets an exception, it should not return any 
    //! object references because the engine will not release the returned reference.
	virtual int         SetException(const char *string) = 0;
	//! \brief Returns the line number where the exception occurred.
    //! \param[out] column The variable will receive the column number.
    //! \return The line number where the exception occurred.
    //!
    //! This method returns the line number where the exception ocurred. The line number 
    //! is relative to the script section where the function was implemented.
	virtual int         GetExceptionLineNumber(int *column = 0) = 0;
	//! \brief Returns the function id of the function where the exception occurred.
    //! \return The function id where the exception occurred.
	virtual int         GetExceptionFunction() = 0;
	//! \brief Returns the exception string text.
    //! \return A null terminated string describing the exception that occurred.
	virtual const char *GetExceptionString() = 0;
	//! \brief Sets an exception callback function. The function will be called if a script exception occurs.
    //! \param[in] callback The callback function/method that should be called upon an exception.
    //! \param[in] obj The object pointer on which the callback is called.
    //! \param[in] callConv The calling convention of the callback function/method.
    //! \return A negative value on error.
    //! \retval asNOT_SUPPORTED Calling convention must not be asCALL_GENERIC, or the routine's calling convention is not supported.
    //! \retval asINVALID_ARG   \a obj must not be null for class methods.
    //! \retval asWRONG_CALLING_CONV \a callConv isn't compatible with the routines' calling convention.
    //!
    //! This callback function will be called by the VM when a script exception is raised, which 
    //! allow the application to examine the callstack and generate a detailed report before the 
    //! callstack is cleaned up.
    //!
    //! See \ref SetLineCallback for details on the calling convention.
	virtual int         SetExceptionCallback(asSFuncPtr callback, void *obj, int callConv) = 0;
	//! \brief Removes a previously registered callback.
    //! Removes a previously registered callback.
	virtual void        ClearExceptionCallback() = 0;
	//! \}

	// Debugging
	//! \name Debugging
	//! \{

	//! \brief Sets a line callback function. The function will be called for each executed script statement.
    //! \param[in] callback The callback function/method that should be called for each script line executed.
    //! \param[in] obj The object pointer on which the callback is called.
    //! \param[in] callConv The calling convention of the callback function/method.
    //! \return A negative value on error.
    //! \retval asNOT_SUPPORTED Calling convention must not be asCALL_GENERIC, or the routine's calling convention is not supported.
    //! \retval asINVALID_ARG   \a obj must not be null for class methods.
    //! \retval asWRONG_CALLING_CONV \a callConv isn't compatible with the routines' calling convention.
    //!
    //! This function sets a callback function that will be called by the VM each time the SUSPEND 
    //! instruction is encounted. Generally this instruction is placed before each statement. Thus by 
    //! setting this callback function it is possible to monitor the execution, and suspend the execution 
    //! at application defined locations.
    //!
    //! The callback function can be either a global function or a class method. For a global function 
    //! the VM will pass two parameters, first the context pointer and then the object pointer specified 
    //! by the application. For a class method, the VM will call the method using the object pointer 
    //! as the owner.
    //!
    //! \code
    //! void Callback(asIScriptContext *ctx, void *obj);
    //! void Object::Callback(asIScriptContext *ctx);
    //! \endcode
    //!
    //! The global function can use either \ref asCALL_CDECL or \ref asCALL_STDCALL, and the class method can use either 
    //! \ref asCALL_THISCALL, \ref asCALL_CDECL_OBJLAST, or \ref asCALL_CDECL_OBJFIRST.
    //!
    //! \see \ref doc_debug
	virtual int         SetLineCallback(asSFuncPtr callback, void *obj, int callConv) = 0;
	//! \brief Removes a previously registered callback.
    //! Removes a previously registered callback.
	virtual void        ClearLineCallback() = 0;
	//! \brief Get the current line number that is being executed.
    //! \param[out] column The variable will receive the column number.
    //! \return The current line number.
    //!
    //! This method returns the line number where the context is currently located. 
    //! The line number is relative to the script section where the function was implemented.
	virtual int         GetCurrentLineNumber(int *column = 0) = 0;
	//! \brief Get the current function that is being executed.
    //! \return The function id of the current function.
	virtual int         GetCurrentFunction() = 0;
	//! \brief Returns the size of the callstack, i.e. the number of functions that have yet to complete.
    //! \return The number of functions on the call stack.
    //!
    //! This methods returns the size of the callstack, i.e. how many parent functions there are 
    //! above the current functions being called. It can be used to enumerate the callstack in order 
    //! to generate a detailed report when an exception occurs, or for debugging running scripts.
	virtual int         GetCallstackSize() = 0;
	//! \brief Returns the function id at the specified callstack level.
    //! \param[in] index The index on the call stack.
    //! \return The function id on the call stack referred to by the index.
	virtual int         GetCallstackFunction(int index) = 0;
	//! \brief Returns the line number at the specified callstack level.
    //! \param[in] index The index on the call stack.
    //! \param[out] column The variable will receive the column number.
    //! \return The line number for the call stack level referred to by the index.
	virtual int         GetCallstackLineNumber(int index, int *column = 0) = 0;
	//! \brief Returns the number of local variables at the specified callstack level.
    //! \param[in] stackLevel The index on the call stack.
    //! \return The number of variables in the function on the call stack level.
    //!
    //! Returns the number of declared variables, including the parameters, in the function on the stack.
	virtual int         GetVarCount(int stackLevel = -1) = 0;
	//! \brief Returns the name of local variable at the specified callstack level.
    //! \param[in] varIndex The index of the variable.
    //! \param[in] stackLevel The index on the call stack.
    //! \return A null terminated string with the name of the variable.
	virtual const char *GetVarName(int varIndex, int stackLevel = -1) = 0;
	//! \brief Returns the declaration of a local variable at the specified callstack level.
    //! \param[in] varIndex The index of the variable.
    //! \param[in] stackLevel The index on the call stack.
    //! \return A null terminated string with the declaration of the variable.
	virtual const char *GetVarDeclaration(int varIndex, int stackLevel = -1) = 0;
	//! \brief Returns the type id of a local variable at the specified callstack level.
    //! \param[in] varIndex The index of the variable.
    //! \param[in] stackLevel The index on the call stack.
    //! \return The type id of the variable, or a negative value on error.
	//! \retval asINVALID_ARG The index or stack level is invalid.
	virtual int         GetVarTypeId(int varIndex, int stackLevel = -1) = 0;
	//! \brief Returns a pointer to a local variable at the specified callstack level.
    //! \param[in] varIndex The index of the variable.
    //! \param[in] stackLevel The index on the call stack.
    //! \return A pointer to the variable.
    //!
    //! Returns a pointer to the variable, so that its value can be read and written. The 
    //! address is valid until the script function returns.
    //!
    //! Note that object variables may not be initalized at all moments, thus you must verify 
    //! if the address returned points to a null pointer, before you try to dereference it.
	virtual void       *GetAddressOfVar(int varIndex, int stackLevel = -1) = 0;
	//! \brief Returns the type id of the object, if a class method is being executed.
    //! \param[in] stackLevel The index on the call stack.
    //! \return Returns the type id of the object if it is a class method.
	virtual int         GetThisTypeId(int stackLevel = -1) = 0;
	//! \brief Returns a pointer to the object, if a class method is being executed.
    //! \param[in] stackLevel The index on the call stack.
    //! \return Returns a pointer to the object if it is a class method.
	virtual void       *GetThisPointer(int stackLevel = -1) = 0;
	//! \}

	// User data
	//! \name User data
	//! \{

	//! \brief Register the memory address of some user data.
    //! \param[in] data A pointer to the user data.
    //! \return The previous pointer stored in the context.
    //!
    //! This method allows the application to associate a value, e.g. a pointer, with the context instance.
	virtual void *SetUserData(void *data) = 0;
	//! \brief Returns the address of the previously registered user data.
    //! \return The pointer to the user data.
	virtual void *GetUserData() = 0;
	//! \}

protected:
	virtual ~asIScriptContext() {}
};

//! \brief The interface for the generic calling convention
class asIScriptGeneric
{
public:
	// Miscellaneous
	//! \name Miscellaneous
	//! \{

    //! \brief Returns a pointer to the script engine.
    //! \return A pointer to the engine.
	virtual asIScriptEngine *GetEngine() = 0;
    //! \brief Returns the function id of the called function.
    //! \return The function id of the function being called.
	virtual int              GetFunctionId() = 0;
	//! \}

	// Object
	//! \name Object
	//! \{

    //! \brief Returns the object pointer if this is a class method, or null if it not.
    //! \return A pointer to the object, if this is a method.
	virtual void   *GetObject() = 0;
    //! \brief Returns the type id of the object if this is a class method.
    //! \return The type id of the object if this is a method.
	virtual int     GetObjectTypeId() = 0;
	//! \}

	// Arguments
	//! \name Arguments
	//! \{

    //! \brief Returns the number of arguments.
    //! \return The number of arguments to the function.
	virtual int     GetArgCount() = 0;
    //! \brief Returns the type id of the argument.
    //! \param[in] arg The argument index.
    //! \return The type id of the argument.
	virtual int     GetArgTypeId(asUINT arg) = 0;
    //! \brief Returns the value of an 8-bit argument.
    //! \param[in] arg The argument index.
    //! \return The 1 byte argument value.
	virtual asBYTE  GetArgByte(asUINT arg) = 0;
    //! \brief Returns the value of a 16-bit argument.
    //! \param[in] arg The argument index.
    //! \return The 2 byte argument value.
	virtual asWORD  GetArgWord(asUINT arg) = 0;
    //! \brief Returns the value of a 32-bit integer argument.
    //! \param[in] arg The argument index.
    //! \return The 4 byte argument value.
	virtual asDWORD GetArgDWord(asUINT arg) = 0;
    //! \brief Returns the value of a 64-bit integer argument.
    //! \param[in] arg The argument index.
    //! \return The 8 byte argument value.
	virtual asQWORD GetArgQWord(asUINT arg) = 0;
    //! \brief Returns the value of a float argument.
    //! \param[in] arg The argument index.
    //! \return The float argument value.
	virtual float   GetArgFloat(asUINT arg) = 0;
    //! \brief Returns the value of a double argument.
    //! \param[in] arg The argument index.
    //! \return The double argument value.
	virtual double  GetArgDouble(asUINT arg) = 0;
    //! \brief Returns the address held in a reference or handle argument.
    //! \param[in] arg The argument index.
    //! \return The address argument value, which can be a reference or and object handle.
    //!
    //! Don't release the pointer if this is an object or object handle, the asIScriptGeneric object will 
    //! do that for you.
	virtual void   *GetArgAddress(asUINT arg) = 0;
    //! \brief Returns a pointer to the object in a object argument.
    //! \param[in] arg The argument index.
    //! \return A pointer to the object argument, which can be an object value or object handle.
    //!
    //! Don't release the pointer if this is an object handle, the asIScriptGeneric object will 
    //! do that for you.
	virtual void   *GetArgObject(asUINT arg) = 0;
    //! \brief Returns a pointer to the argument value.
    //! \param[in] arg The argument index.
    //! \return A pointer to the argument value.
	virtual void   *GetAddressOfArg(asUINT arg) = 0;
	//! \}

	// Return value
	//! \name Return value
	//! \{

    //! \brief Gets the type id of the return value.
    //! \return The type id of the return value.
	virtual int     GetReturnTypeId() = 0;
    //! \brief Sets the 8-bit return value.
    //! \param[in] val The return value.
    //! \return A negative value on error.
    //! \retval asINVALID_TYPE The return type is not an 8-bit value.
    //! Sets the 1 byte return value.
	virtual int     SetReturnByte(asBYTE val) = 0;
    //! \brief Sets the 16-bit return value.
    //! \param[in] val The return value.
    //! \return A negative value on error.
    //! \retval asINVALID_TYPE The return type is not a 16-bit value.
    //! Sets the 2 byte return value.
	virtual int     SetReturnWord(asWORD val) = 0;
    //! \brief Sets the 32-bit integer return value.
    //! \param[in] val The return value.
    //! \return A negative value on error.
    //! \retval asINVALID_TYPE The return type is not a 32-bit value.
    //! Sets the 4 byte return value.
	virtual int     SetReturnDWord(asDWORD val) = 0;
    //! \brief Sets the 64-bit integer return value.
    //! \param[in] val The return value.
    //! \return A negative value on error.
    //! \retval asINVALID_TYPE The return type is not a 64-bit value.
    //! Sets the 8 byte return value.
	virtual int     SetReturnQWord(asQWORD val) = 0;
    //! \brief Sets the float return value.
    //! \param[in] val The return value.
    //! \return A negative value on error.
    //! \retval asINVALID_TYPE The return type is not a 32-bit value.
    //! Sets the float return value.
	virtual int     SetReturnFloat(float val) = 0;
    //! \brief Sets the double return value.
    //! \param[in] val The return value.
    //! \return A negative value on error.
    //! \retval asINVALID_TYPE The return type is not a 64-bit value.
    //! Sets the double return value.
	virtual int     SetReturnDouble(double val) = 0;
    //! \brief Sets the address return value when the return is a reference or handle.
    //! \param[in] addr The return value, which is an address.
    //! \return A negative value on error.
    //! \retval asINVALID_TYPE The return type is not a reference or handle.
    //!
    //! Sets the address return value. If an object handle the application must first 
    //! increment the reference counter, unless it won't keep a reference itself.
	virtual int     SetReturnAddress(void *addr) = 0;
    //! \brief Sets the object return value.
    //! \param[in] obj A pointer to the object return value.
    //! \return A negative value on error.
    //! \retval asINVALID_TYPE The return type is not an object value or handle.
    //!
    //! If the function returns an object, the library will automatically do what is 
    //! necessary based on how the object was declared, i.e. if the function was 
    //! registered to return a handle then the library will call the addref behaviour. 
    //! If it was registered to return an object by value, then the library will make 
    //! a copy of the object.
	virtual int     SetReturnObject(void *obj) = 0;
	//! \brief Gets the address to the memory where the return value should be placed.
	//! \return A pointer to the memory where the return values is to be placed.
	//!
	//! The memory is not initialized, so if you're going to return a complex type by value,
	//! you shouldn't use the assignment operator to initialize it. Instead use the placement new
	//! operator to call the type's copy constructor to perform the initialization.
	//!
	//! \code
	//! new(gen->GetAddressOfReturnLocation()) std::string(myRetValue);
	//! \endcode
	//!
	//! The placement new operator works for primitive types too, so this method is ideal
	//! for writing automatically generated functions that works the same way for all types.
	virtual void   *GetAddressOfReturnLocation() = 0;
	//! \}

protected:
	virtual ~asIScriptGeneric() {}
};

//! \brief The interface for an instance of a script object
class asIScriptObject
{
public:
	// Memory management
	//! \name Memory management
	//! \{

	//! \brief Increase reference counter.
    //!
    //! \return The number of references to this object.
    //!
    //! Call this method when storing an additional reference to the object.
	virtual int AddRef() = 0;
	//! \brief Decrease reference counter.
    //!
    //! \return The number of references to this object.
    //!
    //! Call this method when you will no longer use the references that you own.
	virtual int Release() = 0;
	//! \}

	// Type info
	//! \name Type info
	//! \{

	//! \brief Returns the type id of the object.
    //! \return The type id of the script object.
	virtual int            GetTypeId() const = 0;
    //! \brief Returns the object type interface for the object.
    //! \return The object type interface of the script object.
	virtual asIObjectType *GetObjectType() const = 0;
	//! \}

	// Class properties
	//! \name Properties
	//! \{

	//! \brief Returns the number of properties that the object contains.
    //! \return The number of member properties of the script object.
	virtual int         GetPropertyCount() const = 0;
    //! \brief Returns the type id of the property referenced by prop.
    //! \param[in] prop The property index.
    //! \return The type id of the member property, or a negative value on error.
    //! \retval asINVALID_ARG \a prop is too large
	virtual int         GetPropertyTypeId(asUINT prop) const = 0;
    //! \brief Returns the name of the property referenced by prop.
    //! \param[in] prop The property index.
    //! \return A null terminated string with the property name.
	virtual const char *GetPropertyName(asUINT prop) const = 0;
    //! \brief Returns a pointer to the property referenced by prop.
    //! \param[in] prop The property index.
    //! \return A pointer to the property value.
    //!
    //! The method returns a pointer to the memory location for the property. Use the type 
    //! id for the property to determine the content of the pointer, and how to handle it.
	virtual void       *GetAddressOfProperty(asUINT prop) = 0;
	//! \}

	//! \name Miscellaneous
	//! \{

	//! \brief Return the script engine.
	//! \return The script engine.
	virtual asIScriptEngine *GetEngine() const = 0;
    //! \brief Copies the content from another object of the same type.
    //! \param[in] other A pointer to the source object.
    //! \return A negative value on error.
    //! \retval asINVALID_ARG  The argument is null.
    //! \retval asINVALID_TYPE The other object is of different type.
    //!
    //! This method copies the contents of the other object to this one.
	virtual int              CopyFrom(asIScriptObject *other) = 0;
	//! \}

protected:
	virtual ~asIScriptObject() {}
};


//! \brief The interface for a script array object
class asIScriptArray
{
public:
	//! \brief Return the script engine.
	//! \return The script engine.
	virtual asIScriptEngine *GetEngine() const = 0;

	// Memory management
	//! \brief Increase reference counter.
    //!
    //! \return The number of references to this object.
    //!
    //! Call this method when storing an additional reference to the object.
	virtual int AddRef() = 0;
	//! \brief Decrease reference counter.
    //!
    //! \return The number of references to this object.
    //!
    //! Call this method when you will no longer use the references that you own.
	virtual int Release() = 0;

	// Array type
	//! \brief Returns the type id of the array object.
    //! \return The type id of the array object.
	virtual int GetArrayTypeId() = 0;

	// Elements
	//! \brief Returns the type id of the contained elements.
    //! \return The type id of the array elements.
	virtual int    GetElementTypeId() = 0;

    //! \brief Returns the size of the array.
    //! \return The number of elements in the array.
	virtual asUINT GetElementCount() = 0;

    //! \brief Returns a pointer to the element referenced by index.
    //! \param[in] index The element index.
    //! \return A pointer to the element value.
    //!
    //! The method returns a pointer to the memory location for the element. Use the 
    //! type id for the element to determine the content of the pointer, and how to handle it.
	virtual void * GetElementPointer(asUINT index) = 0;

    //! \brief Resizes the array.
    //! \param[in] size The new size of the array.
    //!
    //! This method allows the application to resize the array.
	virtual void   Resize(asUINT size) = 0;

    //! \brief Copies the elements from another array, overwriting the current content.
    //! \param[in] other A pointer to the source array.
    //! \return A negative value on error.
    //! \retval asINVALID_ARG  The argument is null.
    //! \retval asINVALID_TYPE The other array is of different type.
    //! 
    //! This method copies the contents of the other object to this one.
	virtual int    CopyFrom(asIScriptArray *other) = 0;

protected:
	virtual ~asIScriptArray() {}
};

//! \brief The interface for an object type
class asIObjectType
{
public:
	//! \name Miscellaneous
	//! \{

	//! \brief Returns a pointer to the script engine.
    //! \return A pointer to the engine.
	virtual asIScriptEngine *GetEngine() const = 0;
	//! \brief Returns the config group in which the type was registered.
	//! \return The name of the config group, or null if not set.
	virtual const char      *GetConfigGroup() const = 0;
	//! \}

	// Memory management
	//! \name Memory management
	//! \{

	//! \brief Increases the reference counter.
    //!
    //! \return The number of references to this object.
    //!
    //! Call this method when storing an additional reference to the object.
	virtual int AddRef() = 0;
	//! \brief Decrease reference counter.
    //!
    //! \return The number of references to this object.
    //!
    //! Call this method when you will no longer use the references that you own.
	virtual int Release() = 0;
	//! \}

	// Type info
	//! \name Type info
	//! \{

	//! \brief Returns a temporary pointer to the name of the datatype.
	//! \return A null terminated string with the name of the object type.
	virtual const char      *GetName() const = 0;
	//! \brief Returns the object type that this type derives from.
	//! \return A pointer to the object type that this type derives from.
	//!
	//! This method will only return a pointer in case of script classes that derives from another script class.
	virtual asIObjectType   *GetBaseType() const = 0;
	//! \brief Returns the object type flags.
	//! \return A bit mask with the flags from \ref asEObjTypeFlags.
	//!
	//! Script classes are identified by having the asOBJ_SCRIPT_OBJECT flag set. 
	//! Interfaces are identified as a script class with a size of zero.
	//!
	//! \see GetSize
	virtual asDWORD          GetFlags() const = 0;
	//! \brief Returns the size of the object type.
	//! \return The number of bytes necessary to store instances of this type.
	//!
	//! Application registered reference types doesn't store this information, 
	//! as the script engine doesn't allocate memory for these itself.
	virtual asUINT           GetSize() const = 0;
	//! \brief Returns the type id for the object type.
	//! \return The type id for the object type.
	virtual int              GetTypeId() const = 0;
	//! \brief Returns the type id of the template sub type.
	//! \return The type id of the template sub type, or a negative value on error.
	//! \retval asERROR The type is not a template type.
	virtual int              GetSubTypeId() const = 0;
	//! \}

	// Interfaces
	//! \name Interfaces
	//! \{

	//! \brief Returns the number of interfaces implemented.
    //! \return The number of interfaces implemented by this type.
	virtual int              GetInterfaceCount() const = 0;
	//! \brief Returns a temporary pointer to the specified interface or null if none are found.
    //! \param[in] index The interface index.
    //! \return A pointer to the interface type.
	virtual asIObjectType   *GetInterface(asUINT index) const = 0;
	//! \}

	// Factories
	//! \name Factories
	//! \{

	//! \brief Returns the number of factory functions for the object type.
	//! \return A negative value on error, or the number of factory functions for this object.
	virtual int                GetFactoryCount() const = 0;
	//! \brief Returns the factory id by index.
	//! \param[in] index The index of the factory function.
	//! \return A negative value on error, or the factory id.
	//! \retval asINVALID_ARG \a index is out of bounds.
	virtual int                GetFactoryIdByIndex(int index) const = 0;
	//! \brief Returns the factory id by declaration.
	//! \param[in] decl The factory signature.
	//! \return A negative value on error, or the factory id.
	//! \retval asNO_FUNCTION Didn't find any matching functions.
	//! \retval asINVALID_DECLARATION \a decl is not a valid declaration.
	//! \retval asERROR The module for the type was not built successfully.
	//!
	//! The factory function is named after the object type and 
	//! returns a handle to the object. Example:
	//! 
	//! \code
	//! id = type->GetFactoryIdByDecl("object@ object(int arg1, int arg2)");
	//! \endcode
	virtual int                GetFactoryIdByDecl(const char *decl) const = 0;
	//! \}

	// Methods
	//! \name Methods
	//! \{

	//! \brief Returns the number of methods for the object type.
    //! \return A negative value on error, or the number of methods for this object.
	virtual int                GetMethodCount() const = 0;
	//! \brief Returns the method id by index.
    //! \param[in] index The index of the method.
    //! \return A negative value on error, or the method id.
    //! \retval asINVALID_ARG \a index is out of bounds.
    //!
    //! This method should be used to retrieve the ID of the script method for the object 
    //! that you wish to execute. The ID is then sent to the context's \ref asIScriptContext::Prepare "Prepare" method.
	virtual int                GetMethodIdByIndex(int index) const = 0;
	//! \brief Returns the method id by name.
    //! \param[in] name The name of the method.
    //! \return A negative value on error, or the method id.
    //! \retval asMULTIPLE_FUNCTIONS Found multiple matching methods.
    //! \retval asNO_FUNCTION Didn't find any matching method.
    //!
    //! This method should be used to retrieve the ID of the script method for the object 
    //! that you wish to execute. The ID is then sent to the context's \ref asIScriptContext::Prepare "Prepare" method.
	virtual int                GetMethodIdByName(const char *name) const = 0;
	//! \brief Returns the method id by declaration.
    //! \param[in] decl The method signature.
    //! \return A negative value on error, or the method id.
    //! \retval asMULTIPLE_FUNCTIONS Found multiple matching methods.
    //! \retval asNO_FUNCTION Didn't find any matching method.
    //! \retval asINVALID_DECLARATION \a decl is not a valid declaration.
    //! \retval asERROR The module for the type was not built successfully.
    //!
    //! This method should be used to retrieve the ID of the script method for the object 
    //! that you wish to execute. The ID is then sent to the context's \ref asIScriptContext::Prepare "Prepare" method.
    //!
    //! The method will find the script method with the exact same declaration.
	virtual int                GetMethodIdByDecl(const char *decl) const = 0;
	//! \brief Returns the function descriptor for the script method
    //! \param[in] index The index of the method.
    //! \return A pointer to the method description interface, or null if not found.
	virtual asIScriptFunction *GetMethodDescriptorByIndex(int index) const = 0;
	//! \}

	// Properties
	//! \name Properties
	//! \{

	//! \brief Returns the number of properties that the object contains.
    //! \return The number of member properties of the script object.
	virtual int         GetPropertyCount() const = 0;
    //! \brief Returns the type id of the property referenced by \a prop.
    //! \param[in] prop The property index.
    //! \return The type id of the member property, or a negative value on error.
    //! \retval asINVALID_ARG \a prop is too large
	virtual int         GetPropertyTypeId(asUINT prop) const = 0;
    //! \brief Returns the name of the property referenced by \a prop.
    //! \param[in] prop The property index.
    //! \return A null terminated string with the property name.
	virtual const char *GetPropertyName(asUINT prop) const = 0;
	//! \brief Returns the offset of the property in the memory layout.
	//! \param[in] prop The property index.
	//! \return The offset of the property in the memory layout.
	virtual int         GetPropertyOffset(asUINT prop) const = 0;
	//! \}

	// Behaviours
	//! \name Behaviours
	//! \{

	//! \brief Returns the number of behaviours.
	//! \return The number of behaviours for this type.
	virtual int GetBehaviourCount() const = 0;
	//! \brief Returns the function id and type of the behaviour.
	//! \param[in] index The index of the behaviour.
	//! \param[out] outBehaviour Receives the type of the behaviour.
	//! \return The function id of the behaviour.
	//! \retval asINVALID_ARG The \a index is too large.
	virtual int GetBehaviourByIndex(asUINT index, asEBehaviours *outBehaviour) const = 0;
	//! \}

protected:
	virtual ~asIObjectType() {}
};

//! \brief The interface for a script function description
class asIScriptFunction
{
public:
	//! \name Miscellaneous
	//! \{

	//! \brief Returns a pointer to the script engine.
    //! \return A pointer to the engine.
	virtual asIScriptEngine *GetEngine() const = 0;

	// Memory management
	//! \brief Increases the reference counter.
    //!
    //! \return The number of references to this object.
    //!
    //! Call this method when storing an additional reference to the object.
	virtual int AddRef() = 0;
	//! \brief Decrease reference counter.
    //!
    //! \return The number of references to this object.
    //!
    //! Call this method when you will no longer use the references that you own.
	virtual int Release() = 0;

	//! \brief Returns the id of the function
	//! \return The id of the function
	virtual int              GetId() const = 0;
	//! \brief Returns the name of the module where the function was implemented
    //! \return A null terminated string with the module name.
	virtual const char      *GetModuleName() const = 0;
	//! \brief Returns the name of the script section where the function was implemented.
    //! \return A null terminated string with the script section name where the function was implemented.
	virtual const char      *GetScriptSectionName() const = 0;
	//! \brief Returns the name of the config group in which the function was registered.
	//! \return The name of the config group, or null if not in any group.
	virtual const char      *GetConfigGroup() const = 0;
	//! \}

	//! \name Function info
	//! \{

	//! \brief Returns the object type for class or interface method
    //! \return A pointer to the object type interface if this is a method.
	virtual asIObjectType   *GetObjectType() const = 0;
	//! \brief Returns the name of the object for class or interface methods
    //! \return A null terminated string with the name of the object type if this a method.
	virtual const char      *GetObjectName() const = 0;
	//! \brief Returns the name of the function or method
    //! \return A null terminated string with the name of the function.
	virtual const char      *GetName() const = 0;
	//! \brief Returns the function declaration
	//! \param[in] includeObjectName Indicate whether the object name should be prepended to the function name
    //! \return A null terminated string with the function declaration.
	virtual const char      *GetDeclaration(bool includeObjectName = true) const = 0;
	//! \brief Returns true if it is a class method
    //! \return True if this a class method.
	virtual bool             IsClassMethod() const = 0;
	//! \brief Returns true if it is an interface method
    //! \return True if this is an interface method.
	virtual bool             IsInterfaceMethod() const = 0;
	//! \brief Returns true if the class method is read-only
	//! \return True if the class method is read-only
	virtual bool             IsReadOnly() const = 0;
	//! \}

	//! \name Parameter and return types
	//! \{

    //! \brief Returns the number of parameters for this function.
    //! \return The number of parameters.
	virtual int              GetParamCount() const = 0;
    //! \brief Returns the type id of the specified parameter.
    //! \param[in] index The zero based parameter index.
    //! \param[out] flags A combination of \ref asETypeModifiers.
    //! \return A negative value on error, or the type id of the specified parameter.
    //! \retval asINVALID_ARG The index is out of bounds.
	virtual int              GetParamTypeId(int index, asDWORD *flags = 0) const = 0;
    //! \brief Returns the type id of the return type.
    //! \return The type id of the return type.
	virtual int              GetReturnTypeId() const = 0;
	//! \}

	//! \name JIT compilation
	//! \{

	// For JIT compilation
	//! \brief Returns the byte code buffer and length.
	//! \param[out] length The length of the byte code buffer in DWORDs
	//! \return A pointer to the byte code buffer, or 0 if this is not a script function.
	//! 
	//! This function is used by the \ref asIJITCompiler to obtain the byte
	//!  code buffer for building the native machine code representation.
	virtual asDWORD         *GetByteCode(asUINT *length = 0) = 0;
	//! \}

protected:
	virtual ~asIScriptFunction() {};
};

//! \brief A binary stream interface.
//!
//! This interface is used when storing compiled bytecode to disk or memory, and then loading it into the engine again.
//!
//! \see \ref asIScriptModule::SaveByteCode, \ref asIScriptModule::LoadByteCode
class asIBinaryStream
{
public:
    //! \brief Read size bytes from the stream into the memory pointed to by ptr.
    //!
    //! \param[out] ptr A pointer to the buffer that will receive the data.
    //! \param[in] size The number of bytes to read.
    //!
    //! Read \a size bytes from the data stream into the memory pointed to by \a ptr.
	virtual void Read(void *ptr, asUINT size) = 0;
    //! \brief Write size bytes to the stream from the memory pointed to by ptr.
    //!
    //! \param[in] ptr A pointer to the buffer that the data should written from.
    //! \param[in] size The number of bytes to write.
    //!
    //! Write \a size bytes to the data stream from the memory pointed to by \a ptr.
	virtual void Write(const void *ptr, asUINT size) = 0;

public:
	virtual ~asIBinaryStream() {}
};

//-----------------------------------------------------------------
// Function pointers

// Use our own memset() and memcpy() implementations for better portability
inline void asMemClear(void *_p, int size)
{
	char *p = (char *)_p;
	const char *e = p + size;
	for( ; p < e; p++ )
		*p = 0;
}

inline void asMemCopy(void *_d, const void *_s, int size)
{
	char *d = (char *)_d;
	const char *s = (const char *)_s;
	const char *e = s + size;
	for( ; s < e; d++, s++ )
		*d = *s;
}

// Template function to capture all global functions,
// except the ones using the generic calling convention
template <class T>
inline asSFuncPtr asFunctionPtr(T func)
{
	asSFuncPtr p;
	asMemClear(&p, sizeof(p));
	p.ptr.f.func = (asFUNCTION_t)(size_t)func;

	// Mark this as a global function
	p.flag = 2;

	return p;
}

// Specialization for functions using the generic calling convention
template<>
inline asSFuncPtr asFunctionPtr<asGENFUNC_t>(asGENFUNC_t func)
{
	asSFuncPtr p;
	asMemClear(&p, sizeof(p));
	p.ptr.f.func = (asFUNCTION_t)func;

	// Mark this as a generic function
	p.flag = 1;

	return p;
}

#ifndef AS_NO_CLASS_METHODS

// Method pointers

// Declare a dummy class so that we can determine the size of a simple method pointer
class asCSimpleDummy {};
typedef void (asCSimpleDummy::*asSIMPLEMETHOD_t)();
const int SINGLE_PTR_SIZE = sizeof(asSIMPLEMETHOD_t);

// Define template
template <int N>
struct asSMethodPtr
{
	template<class M>
	static asSFuncPtr Convert(M Mthd)
	{
		// This version of the function should never be executed, nor compiled,
		// as it would mean that the size of the method pointer cannot be determined.

		int ERROR_UnsupportedMethodPtr[N-100];
		return 0;
	}
};

// Template specialization
template <>
struct asSMethodPtr<SINGLE_PTR_SIZE>
{
	template<class M>
	static asSFuncPtr Convert(M Mthd)
	{
		asSFuncPtr p;
		asMemClear(&p, sizeof(p));

		asMemCopy(&p, &Mthd, SINGLE_PTR_SIZE);

		// Mark this as a class method
		p.flag = 3;

		return p;
	}
};

#if defined(_MSC_VER) && !defined(__MWERKS__)

// MSVC and Intel uses different sizes for different class method pointers
template <>
struct asSMethodPtr<SINGLE_PTR_SIZE+1*sizeof(int)>
{
	template <class M>
	static asSFuncPtr Convert(M Mthd)
	{
		asSFuncPtr p;
		asMemClear(&p, sizeof(p));

		asMemCopy(&p, &Mthd, SINGLE_PTR_SIZE+sizeof(int));

		// Mark this as a class method
		p.flag = 3;

		return p;
	}
};

template <>
struct asSMethodPtr<SINGLE_PTR_SIZE+2*sizeof(int)>
{
	template <class M>
	static asSFuncPtr Convert(M Mthd)
	{
		// This is where a class with virtual inheritance falls, or
		// on 64bit platforms with 8byte data alignments.

		// Method pointers for virtual inheritance is not supported,
		// as it requires the location of the vbase table, which is 
		// only available to the C++ compiler, but not in the method
		// pointer. 

		// You can get around this by forward declaring the class and
		// storing the sizeof its method pointer in a constant. Example:

		// class ClassWithVirtualInheritance;
		// const int ClassWithVirtualInheritance_workaround = sizeof(void ClassWithVirtualInheritance::*());

		// This will force the compiler to use the unknown type
		// for the class, which falls under the next case

		// TODO: We need to try to identify if this is really a method pointer
		//       with virtual inheritance, or just a method pointer for multiple 
		//       inheritance with pad bytes to produce a 16byte structure.

		asSFuncPtr p;
		asMemClear(&p, sizeof(p));

		asMemCopy(&p, &Mthd, SINGLE_PTR_SIZE+2*sizeof(int));

		// Mark this as a class method
		p.flag = 3;

		return p;
	}
};

template <>
struct asSMethodPtr<SINGLE_PTR_SIZE+3*sizeof(int)>
{
	template <class M>
	static asSFuncPtr Convert(M Mthd)
	{
		asSFuncPtr p;
		asMemClear(&p, sizeof(p));

		asMemCopy(&p, &Mthd, SINGLE_PTR_SIZE+3*sizeof(int));

		// Mark this as a class method
		p.flag = 3;

		return p;
	}
};

template <>
struct asSMethodPtr<SINGLE_PTR_SIZE+4*sizeof(int)>
{
	template <class M>
	static asSFuncPtr Convert(M Mthd)
	{
		// On 64bit platforms with 8byte data alignment
		// the unknown class method pointers will come here.

		asSFuncPtr p;
		asMemClear(&p, sizeof(p));

		asMemCopy(&p, &Mthd, SINGLE_PTR_SIZE+4*sizeof(int));

		// Mark this as a class method
		p.flag = 3;

		return p;
	}
};

#endif

#endif // AS_NO_CLASS_METHODS

//----------------------------------------------------------------
// JIT compiler

//! \brief A struct with registers from the VM sent to a JIT compiled function
//!
//! The JIT compiled function will receive a pointer to this structure when called.
//! It is the responsibility of the JIT compiled function to make sure these
//! values are updated correctly before control is returned to the VM.
//!
//! \see \ref doc_adv_jit
struct asSVMRegisters
{
	//! \brief Points to the current bytecode instruction
	asDWORD          *programPointer;     // points to current bytecode instruction
	//! \brief Function stack frame. This doesn't change during the function execution.
	asDWORD          *stackFramePointer;  // function stack frame
	//! \brief Top of the stack (grows downward)
	asDWORD          *stackPointer;       // top of stack (grows downward)
	//! \brief Array of global variable pointers. This doesn't change during the function execution.
	void            **globalVarPointers;  // global variable pointers
	//! \brief Temporary register for primitives and unmanaged references
	asQWORD           valueRegister;      // temp register for primitives
	//! \brief Temporary register for managed object references/handles
	void             *objectRegister;     // temp register for objects and handles
	//! \brief Type of the object held in the object register
	asIObjectType    *objectType;         // type of object held in object register
	//! \brief Set to true if the SUSPEND instruction should be processed. Do not update this value.
	bool              doProcessSuspend;   // whether or not the JIT should break out when it encounters a suspend instruction
};

//! \brief The function signature of a JIT compiled function
//! \param [in] registers  A pointer to the virtual machine's registers.
//! \param [in] entryId    The value defined by the JIT compiler for the current entry point in the JIT function.
//!
//! A JIT function receives a pointer to the virtual machine's registers when called and 
//! an argument telling it where in the script function to continue the execution. The JIT
//! function must make sure to update the VM's registers according to the actions performed
//! before returning control to the VM.
//!
//! \see \ref doc_adv_jit
typedef void (*asJITFunction)(asSVMRegisters *registers, asDWORD entryId);

//! \brief The interface that AS use to interact with the JIT compiler
//!
//! This is the minimal interface that the JIT compiler must implement
//! so that AngelScript can request the compilation of the script functions.
//!
//! \see \ref doc_adv_jit
class asIJITCompiler
{
public:
	//! \brief Called by AngelScript to begin the compilation
	//!
	//! \param [in] function A pointer to the script function
	//! \param [out] output The JIT compiled function
	//! \return A negative value on error.
	//!
	//! AngelScript will call this function to request the compilation of
	//! a script function. The JIT compiler should produce the native machine
	//! code representation of the function and update the JitEntry instructions
	//! in the byte code to allow the VM to transfer the control to the JIT compiled
	//! function.
	virtual int  CompileFunction(asIScriptFunction *function, asJITFunction *output) = 0;
	//! \brief Called by AngelScript when the JIT function is released
	//! \param [in] func Pointer to the JIT function
    virtual void ReleaseJITFunction(asJITFunction func) = 0;
public:
    virtual ~asIJITCompiler() {}
};

// Byte code instructions
//! \brief The bytecode instructions used by the VM
//! \see \ref doc_adv_jit_1
enum asEBCInstr
{
	//! \brief Decrease the stack with the amount in the argument
	asBC_POP			= 0,
	//! \brief Increase the stack with the amount in the argument
	asBC_PUSH			= 1,
	//! \brief Push the 32bit value in the argument onto the stack
	asBC_PshC4			= 2,
	//! \brief Push the 32bit value from a variable onto the stack
	asBC_PshV4			= 3,
	//! \brief Push the address of the stack frame onto the stack
	asBC_PSF			= 4,
	//! \brief Swap the top two DWORDs on the stack
	asBC_SWAP4			= 5,
	//! \brief Perform a boolean not on the value in a variable
	asBC_NOT			= 6,
	//! \brief Push the 32bit value from a global variable onto the stack
	asBC_PshG4			= 7,
	//! \brief Perform the actions of \ref asBC_LDG followed by \ref asBC_RDR4
	asBC_LdGRdR4		= 8,
	//! \brief Jump to a script function, indexed by the argument
	asBC_CALL			= 9,
	//! \brief Return to the instruction after the last executed call
	asBC_RET			= 10,
	//! \brief Unconditional jump to a relative position in this function
	asBC_JMP			= 11,
	//! \brief If the value register is 0 jump to a relative position in this function
	asBC_JZ				= 12,
	//! \brief If the value register is not 0 jump to a relative position in this function
	asBC_JNZ			= 13,
	//! \brief If the value register is less than 0 jump to a relative position in this function
	asBC_JS				= 14,
	//! \brief If the value register is greater than or equal to 0 jump to a relative position in this function
	asBC_JNS			= 15,
	//! \brief If the value register is greater than 0 jump to a relative position in this function
	asBC_JP				= 16,
	//! \brief If the value register is less than or equal to 0 jump to a relative position in this function
	asBC_JNP			= 17,
	//! \brief If the value register is 0 set it to 1
	asBC_TZ				= 18,
	//! \brief If the value register is not 0 set it to 1
	asBC_TNZ			= 19,
	//! \brief If the value register is less than 0 set it to 1
	asBC_TS				= 20,
	//! \brief If the value register is greater than or equal to 0 set it to 1
	asBC_TNS			= 21,
	//! \brief If the value register is greater than 0 set it to 1
	asBC_TP				= 22,
	//! \brief If the value register is less than or equal to 0 set it to 1
	asBC_TNP			= 23,
	//! \brief Negate the 32bit integer value in the variable
	asBC_NEGi			= 24,
	//! \brief Negate the float value in the variable
	asBC_NEGf			= 25,
	//! \brief Negate the double value in the variable
	asBC_NEGd			= 26,
	//! \brief Increment the 16bit integer value that is stored at the address pointed to by the reference in the value register
	asBC_INCi16			= 27,
	//! \brief Increment the 8bit integer value that is stored at the address pointed to by the reference in the value register
	asBC_INCi8			= 28,
	//! \brief Decrement the 16bit integer value that is stored at the address pointed to by the reference in the value register
	asBC_DECi16			= 29,
	//! \brief Increment the 8bit integer value that is stored at the address pointed to by the reference in the value register
	asBC_DECi8			= 30,
	//! \brief Increment the 32bit integer value that is stored at the address pointed to by the reference in the value register
	asBC_INCi			= 31,
	//! \brief Decrement the 32bit integer value that is stored at the address pointed to by the reference in the value register
	asBC_DECi			= 32,
	//! \brief Increment the float value that is stored at the address pointed to by the reference in the value register
	asBC_INCf			= 33,
	//! \brief Decrement the float value that is stored at the address pointed to by the reference in the value register
	asBC_DECf			= 34,
	//! \brief Increment the double value that is stored at the address pointed to by the reference in the value register
	asBC_INCd			= 35,
	//! \brief Decrement the double value that is stored at the address pointed to by the reference in the value register
	asBC_DECd			= 36,
	//! \brief Increment the 32bit integer value in the variable
	asBC_IncVi			= 37,
	//! \brief Decrement the 32bit integer value in the variable
	asBC_DecVi			= 38,
	//! \brief Perform a bitwise complement on the 32bit value in the variable
	asBC_BNOT			= 39,
	//! \brief Perform a bitwise and of two 32bit values and store the result in a third variable
	asBC_BAND			= 40,
	//! \brief Perform a bitwise or of two 32bit values and store the result in a third variable
	asBC_BOR			= 41,
	//! \brief Perform a bitwise exclusive or of two 32bit values and store the result in a third variable
	asBC_BXOR			= 42,
	//! \brief Perform a logical left shift of a 32bit value and store the result in a third variable
	asBC_BSLL			= 43,
	//! \brief Perform a logical right shift of a 32bit value and store the result in a third variable
	asBC_BSRL			= 44,
	//! \brief Perform a arithmetical right shift of a 32bit value and store the result in a third variable
	asBC_BSRA			= 45,
	//! \brief Pop the destination and source addresses from the stack. Perform a bitwise copy of the referred object. Push the destination address on the stack.
	asBC_COPY			= 46,
	//! \brief Push a 64bit value on the stack
	asBC_PshC8			= 47,
	//! \brief Pop an address from the stack, then read a 64bit value from that address and push it on the stack
	asBC_RDS8			= 48,
	//! \brief Swap the top two QWORDs on the stack
	asBC_SWAP8			= 49,
	//! \brief Compare two double variables and store the result in the value register
	asBC_CMPd			= 50,
	//! \brief Compare two unsigned 32bit integer variables and store the result in the value register
	asBC_CMPu			= 51,
	//! \brief Compare two float variables and store the result in the value register
	asBC_CMPf			= 52,
	//! \brief Compare two 32bit integer variables and store the result in the value register
	asBC_CMPi			= 53,
	//! \brief Compare 32bit integer variable with constant and store the result in value register
	asBC_CMPIi			= 54,
	//! \brief Compare float variable with constant and store the result in value register
	asBC_CMPIf			= 55,
	//! \brief Compare unsigned 32bit integer variable with constant and store the result in value register
	asBC_CMPIu			= 56,
	//! \brief Jump to relative position in the function where the offset is stored in a variable
	asBC_JMPP			= 57,
	//! \brief Pop a pointer from the stack and store it in the value register
	asBC_PopRPtr		= 58,
	//! \brief Push a pointer from the value register onto the stack
	asBC_PshRPtr		= 59,
	//! \brief Push string address and length on the stack
	asBC_STR			= 60,
	//! \brief Call registered function. Suspend further execution if requested.
	asBC_CALLSYS		= 61,
	//! \brief Jump to an imported script function, indexed by the argument
	asBC_CALLBND		= 62,
	//! \brief Call line callback function if set. Suspend execution if requested.
	asBC_SUSPEND		= 63,
	//! \brief Allocate the memory for the object. If the type is a script object then jump to the constructor, else call the registered constructor behaviour. Suspend further execution if requested.
	asBC_ALLOC			= 64,
	//! \brief Pop the address of the object variable from the stack. If ref type, call the release method, else call the destructor then free the memory. Clear the pointer in the variable.
	asBC_FREE			= 65,
	//! \brief Copy the object pointer from a variable to the object register. Clear the variable.
	asBC_LOADOBJ		= 66,
	//! \brief Copy the object pointer from the object register to the variable. Clear the object register.
	asBC_STOREOBJ		= 67,
	//! \brief Move object pointer from variable onto stack location.
	asBC_GETOBJ			= 68,
	//! \brief Pop destination handle reference. Perform a handle assignment, while updating the reference count for both previous and new objects.
	asBC_REFCPY			= 69,
	//! \brief Throw an exception if the pointer on the top of the stack is null.
	asBC_CHKREF			= 70,
	//! \brief Replace a variable index on the stack with the object handle stored in that variable.
	asBC_GETOBJREF		= 71,
	//! \brief Replace a variable index on the stack with the address of the variable.
	asBC_GETREF			= 72,
	//! \brief Swap the top DWORD with the QWORD below it
	asBC_SWAP48			= 73,
	//! \brief Swap the top QWORD with the DWORD below it
	asBC_SWAP84			= 74,
	//! \brief Push the pointer argument onto the stack. The pointer is a pointer to an object type structure
	asBC_OBJTYPE		= 75,
	//! \brief Push the type id onto the stack. Equivalent to \ref asBC_PshC4 "PshC4".
	asBC_TYPEID			= 76,
	//! \brief Initialize the variable with a DWORD.
	asBC_SetV4			= 77,
	//! \brief Initialize the variable with a QWORD.
	asBC_SetV8			= 78,
	//! \brief Add a value to the top pointer on the stack, thus updating the address itself.
	asBC_ADDSi			= 79,
	//! \brief Copy a DWORD from one variable to another
	asBC_CpyVtoV4		= 80,
	//! \brief Copy a QWORD from one variable to another
	asBC_CpyVtoV8		= 81,
	//! \brief Copy a DWORD from a variable into the value register
	asBC_CpyVtoR4		= 82,
	//! \brief Copy a QWORD from a variable into the value register
	asBC_CpyVtoR8		= 83,
	//! \brief Copy a DWORD from a local variable to a global variable
	asBC_CpyVtoG4		= 84,
	//! \brief Copy a DWORD from the value register into a variable
	asBC_CpyRtoV4		= 85,
	//! \brief Copy a QWORD from the value register into a variable
	asBC_CpyRtoV8		= 86,
	//! \brief Copy a DWORD from a global variable to a local variable
	asBC_CpyGtoV4		= 87,
	//! \brief Copy a BYTE from a variable to the address held in the value register
	asBC_WRTV1			= 88,
	//! \brief Copy a WORD from a variable to the address held in the value register
	asBC_WRTV2			= 89,
	//! \brief Copy a DWORD from a variable to the address held in the value register
	asBC_WRTV4			= 90,
	//! \brief Copy a QWORD from a variable to the address held in the value register
	asBC_WRTV8			= 91,
	//! \brief Copy a BYTE from address held in the value register to a variable. Clear the top bytes in the variable
	asBC_RDR1			= 92,
	//! \brief Copy a WORD from address held in the value register to a variable. Clear the top word in the variable
	asBC_RDR2			= 93,
	//! \brief Copy a DWORD from address held in the value register to a variable.
	asBC_RDR4			= 94,
	//! \brief Copy a QWORD from address held in the value register to a variable.
	asBC_RDR8			= 95,
	//! \brief Load the address of a global variable into the value register
	asBC_LDG			= 96,
	//! \brief Load the address of a local variable into the value register
	asBC_LDV			= 97,
	//! \brief Push the address of a global variable on the stack
	asBC_PGA			= 98,
	//! \brief Pop an address from the stack. Read a DWORD from the address, and push it on the stack.
	asBC_RDS4			= 99,
	//! \brief Push the index of the variable on the stack, with the size of a pointer.
	asBC_VAR			= 100,
	//! \brief Convert the 32bit integer value to a float in the variable
	asBC_iTOf			= 101,
	//! \brief Convert the float value to a 32bit integer in the variable
	asBC_fTOi			= 102,
	//! \brief Convert the unsigned 32bit integer value to a float in the variable
	asBC_uTOf			= 103,
	//! \brief Convert the float value to an unsigned 32bit integer in the variable
	asBC_fTOu			= 104,
	//! \brief Expand the low byte as a signed value to a full 32bit integer in the variable
	asBC_sbTOi			= 105,
	//! \brief Expand the low word as a signed value to a full 32bit integer in the variable
	asBC_swTOi			= 106,
	//! \brief Expand the low byte as an unsigned value to a full 32bit integer in the variable
	asBC_ubTOi			= 107,
	//! \brief Expand the low word as an unsigned value to a full 32bit integer in the variable
	asBC_uwTOi			= 108,
	//! \brief Convert the double value in one variable to a 32bit integer in another variable
	asBC_dTOi			= 109,
	//! \brief Convert the double value in one variable to a 32bit unsigned integer in another variable
	asBC_dTOu			= 110,
	//! \brief Convert the double value in one variable to a float in another variable
	asBC_dTOf			= 111,
	//! \brief Convert the 32bit integer value in one variable to a double in another variable
	asBC_iTOd			= 112,
	//! \brief Convert the 32bit unsigned integer value in one variable to a double in another variable
	asBC_uTOd			= 113,
	//! \brief Convert the float value in one variable to a double in another variable
	asBC_fTOd			= 114,
	//! \brief Add the values of two 32bit integer variables and store in a third variable
	asBC_ADDi			= 115,
	//! \brief Subtract the values of two 32bit integer variables and store in a third variable
	asBC_SUBi			= 116,
	//! \brief Multiply the values of two 32bit integer variables and store in a third variable
	asBC_MULi			= 117,
	//! \brief Divide the values of two 32bit integer variables and store in a third variable
	asBC_DIVi			= 118,
	//! \brief Calculate the modulo of values of two 32bit integer variables and store in a third variable
	asBC_MODi			= 119,
	//! \brief Add the values of two float variables and store in a third variable
	asBC_ADDf			= 120,
	//! \brief Subtract the values of two float variables and store in a third variable
	asBC_SUBf			= 121,
	//! \brief Multiply the values of two float variables and store in a third variable
	asBC_MULf			= 122,
	//! \brief Divide the values of two float variables and store in a third variable
	asBC_DIVf			= 123,
	//! \brief Calculate the modulo of values of two float variables and store in a third variable
	asBC_MODf			= 124,
	//! \brief Add the values of two double variables and store in a third variable
	asBC_ADDd			= 125,
	//! \brief Subtract the values of two double variables and store in a third variable
	asBC_SUBd			= 126,
	//! \brief Multiply the values of two double variables and store in a third variable
	asBC_MULd			= 127,
	//! \brief Divide the values of two double variables and store in a third variable
	asBC_DIVd			= 128,
	//! \brief Calculate the modulo of values of two double variables and store in a third variable
	asBC_MODd			= 129,
	//! \brief Add a 32bit integer variable with a constant value and store the result in another variable
	asBC_ADDIi			= 130,
	//! \brief Subtract a 32bit integer variable with a constant value and store the result in another variable
	asBC_SUBIi			= 131,
	//! \brief Multiply a 32bit integer variable with a constant value and store the result in another variable
	asBC_MULIi			= 132,
	//! \brief Add a float variable with a constant value and store the result in another variable
	asBC_ADDIf			= 133,
	//! \brief Subtract a float variable with a constant value and store the result in another variable
	asBC_SUBIf			= 134,
	//! \brief Multiply a float variable with a constant value and store the result in another variable
	asBC_MULIf			= 135,
	//! \brief Set the value of global variable to a 32bit word
	asBC_SetG4			= 136,
	//! \brief Throw an exception if the address stored on the stack points to a null pointer
	asBC_ChkRefS		= 137,
	//! \brief Throw an exception if the variable is null
	asBC_ChkNullV		= 138,
	//! \brief Jump to an interface method, indexed by the argument
	asBC_CALLINTF		= 139,
	//! \brief Convert a 32bit integer in a variable to a byte, clearing the top bytes
	asBC_iTOb			= 140,
	//! \brief Convert a 32bit integer in a variable to a word, clearing the top word
	asBC_iTOw			= 141,
	//! \brief Same as \ref asBC_SetV4 "SetV4"
	asBC_SetV1			= 142,
	//! \brief Same as \ref asBC_SetV4 "SetV4"
	asBC_SetV2			= 143,
	//! \brief Pop an object handle to a script class from the stack. Perform a dynamic cast on it and store the result in the object register.
	asBC_Cast			= 144,
	//! \brief Convert the 64bit integer value in one variable to a 32bit integer in another variable
	asBC_i64TOi			= 145,
	//! \brief Convert the 32bit unsigned integer value in one variable to a 64bit integer in another variable
	asBC_uTOi64			= 146,
	//! \brief Convert the 32bit integer value in one variable to a 64bit integer in another variable
	asBC_iTOi64			= 147,
	//! \brief Convert the float value in one variable to a 64bit integer in another variable
	asBC_fTOi64			= 148,
	//! \brief Convert the double value in the variable to a 64bit integer
	asBC_dTOi64			= 149,
	//! \brief Convert the float value in one variable to a 64bit unsigned integer in another variable
	asBC_fTOu64			= 150,
	//! \brief Convert the double value in the variable to a 64bit unsigned integer
	asBC_dTOu64			= 151,
	//! \brief Convert the 64bit integer value in one variable to a float in another variable
	asBC_i64TOf			= 152,
	//! \brief Convert the 64bit unsigned integer value in one variable to a float in another variable
	asBC_u64TOf			= 153,
	//! \brief Convert the 32bit integer value in the variable to a double
	asBC_i64TOd			= 154,
	//! \brief Convert the 32bit unsigned integer value in the variable to a double
	asBC_u64TOd			= 155,
	//! \brief Negate the 64bit integer value in the variable
	asBC_NEGi64			= 156,
	//! \brief Increment the 64bit integer value that is stored at the address pointed to by the reference in the value register
	asBC_INCi64			= 157,
	//! \brief Decrement the 64bit integer value that is stored at the address pointed to by the reference in the value register
	asBC_DECi64			= 158,
	//! \brief Perform a bitwise complement on the 64bit value in the variable
	asBC_BNOT64			= 159,
	//! \brief Perform an addition with two 64bit integer variables and store the result in a third variable
	asBC_ADDi64			= 160,
	//! \brief Perform a subtraction with two 64bit integer variables and store the result in a third variable
	asBC_SUBi64			= 161,
	//! \brief Perform a multiplication with two 64bit integer variables and store the result in a third variable
	asBC_MULi64			= 162,
	//! \brief Perform a division with two 64bit integer variables and store the result in a third variable
	asBC_DIVi64			= 163,
	//! \brief Perform the modulo operation with two 64bit integer variables and store the result in a third variable
	asBC_MODi64			= 164,
	//! \brief Perform a bitwise and of two 64bit values and store the result in a third variable
	asBC_BAND64			= 165,
	//! \brief Perform a bitwise or of two 64bit values and store the result in a third variable
	asBC_BOR64			= 166,
	//! \brief Perform a bitwise exclusive or of two 64bit values and store the result in a third variable
	asBC_BXOR64			= 167,
	//! \brief Perform a logical left shift of a 64bit value and store the result in a third variable
	asBC_BSLL64			= 168,
	//! \brief Perform a logical right shift of a 64bit value and store the result in a third variable
	asBC_BSRL64			= 169,
	//! \brief Perform a arithmetical right shift of a 64bit value and store the result in a third variable
	asBC_BSRA64			= 170,
	//! \brief Compare two 64bit integer variables and store the result in the value register
	asBC_CMPi64			= 171,
	//! \brief Compare two unsigned 64bit integer variables and store the result in the value register
	asBC_CMPu64			= 172,
	//! \brief Check if a pointer on the stack is null, and if it is throw an exception. The argument is relative to the top of the stack
	asBC_ChkNullS		= 173,
	//! \brief Clear the upper bytes of the value register so that only the value in the lowest byte is kept
	asBC_ClrHi			= 174,
	//! \brief If a JIT function is available and the argument is not 0 then call the JIT function
	asBC_JitEntry		= 175,

	asBC_MAXBYTECODE	= 176,

	// Temporary tokens. Can't be output to the final program
	asBC_PSP			= 253,
	asBC_LINE			= 254,
	asBC_LABEL			= 255
};

// Instruction types
//! \brief Describes the structure of a bytecode instruction
enum asEBCType
{
	asBCTYPE_INFO         = 0,
	//! \brief Instruction + no args
	asBCTYPE_NO_ARG       = 1,
	//! \brief Instruction + WORD arg
	asBCTYPE_W_ARG        = 2,
	//! \brief Instruction + WORD arg (dest var)
	asBCTYPE_wW_ARG       = 3,
	//! \brief Instruction + DWORD arg
	asBCTYPE_DW_ARG       = 4,
	//! \brief Instruction + WORD arg (source var) + DWORD arg
	asBCTYPE_rW_DW_ARG    = 5,
	//! \brief Instruction + QWORD arg
	asBCTYPE_QW_ARG       = 6,
	//! \brief Instruction + DWORD arg + DWORD arg
	asBCTYPE_DW_DW_ARG    = 7,
	//! \brief Instruction + WORD arg (dest var) + WORD arg (source var) + WORD arg (source var)
	asBCTYPE_wW_rW_rW_ARG = 8,
	//! \brief Instruction + WORD arg (dest var) + QWORD arg
	asBCTYPE_wW_QW_ARG    = 9,
	//! \brief Instruction + WORD arg (dest var) + WORD arg (source var)
	asBCTYPE_wW_rW_ARG    = 10,
	//! \brief Instruction + WORD arg (source var)
	asBCTYPE_rW_ARG       = 11,
	//! \brief Instruction + WORD arg (dest var) + DWORD arg
	asBCTYPE_wW_DW_ARG    = 12,
	//! \brief Instruction + WORD arg (dest var) + WORD arg (source var) + DWORD arg
	asBCTYPE_wW_rW_DW_ARG = 13,
	//! \brief Instruction + WORD arg (source var) + WORD arg (source var)
	asBCTYPE_rW_rW_ARG    = 14,
	//! \brief Instruction + WORD arg + WORD arg (source var)
	asBCTYPE_W_rW_ARG     = 15,
	//! \brief Instruction + WORD arg (dest var) + WORD arg
	asBCTYPE_wW_W_ARG     = 16,
	//! \brief Instruction + WORD arg + DWORD arg
	asBCTYPE_W_DW_ARG     = 17,
	//! \brief Instruction + QWORD arg + DWORD arg
	asBCTYPE_QW_DW_ARG    = 18
};

// Instruction type sizes
//! \brief Lookup table for determining the size of each \ref asEBCType "type" of bytecode instruction.
const int asBCTypeSize[19] =
{
    0, // asBCTYPE_INFO        
    1, // asBCTYPE_NO_ARG      
    1, // asBCTYPE_W_ARG       
    1, // asBCTYPE_wW_ARG      
    2, // asBCTYPE_DW_ARG      
    2, // asBCTYPE_rW_DW_ARG   
    3, // asBCTYPE_QW_ARG      
    3, // asBCTYPE_DW_DW_ARG   
    2, // asBCTYPE_wW_rW_rW_ARG
    3, // asBCTYPE_wW_QW_ARG   
    2, // asBCTYPE_wW_rW_ARG   
    1, // asBCTYPE_rW_ARG      
    2, // asBCTYPE_wW_DW_ARG   
    3, // asBCTYPE_wW_rW_DW_ARG
    2, // asBCTYPE_rW_rW_ARG   
    2, // asBCTYPE_W_rW_ARG    
    2, // asBCTYPE_wW_W_ARG    
    2, // asBCTYPE_W_DW_ARG    
    4  // asBCTYPE_QW_DW_ARG    
};

// Instruction info
//! \brief Information on a bytecode instruction
//!
//! This structure can be obtained for each bytecode instruction
//! by looking it up in the \ref asBCInfo array.
//!
//! The size of the instruction can be obtained by looking up the 
//! type in the \ref asBCTypeSize array.
//!
//! \see \ref doc_adv_jit
struct asSBCInfo
{
	//! \brief Bytecode instruction id
	asEBCInstr  bc;
	//! \brief Instruction argument layout
	asEBCType   type;
	//! \brief How much this argument increments the stack pointer. 0xFFFF if it depends on the arguments.
	int         stackInc;
	//! \brief Name of the instruction for debugging
	const char *name;
};

#ifndef AS_64BIT_PTR
	#define asBCTYPE_PTR_ARG    asBCTYPE_DW_ARG
	#define asBCTYPE_PTR_DW_ARG asBCTYPE_DW_DW_ARG
	#ifndef AS_PTR_SIZE
		#define AS_PTR_SIZE 1
	#endif
#else
	#define asBCTYPE_PTR_ARG    asBCTYPE_QW_ARG
	#define asBCTYPE_PTR_DW_ARG asBCTYPE_QW_DW_ARG
	#ifndef AS_PTR_SIZE
		#define AS_PTR_SIZE 2
	#endif
#endif

#define asBCINFO(b,t,s) {asBC_##b, asBCTYPE_##t, s, #b}
#define asBCINFO_DUMMY(b) {asEBCInstr(b), asBCTYPE_INFO, 0, "BC_" #b}

//! \brief Information on each bytecode instruction
const asSBCInfo asBCInfo[256] =
{
	asBCINFO(POP,		W_ARG,			0xFFFF),
	asBCINFO(PUSH,		W_ARG,			0xFFFF),
	asBCINFO(PshC4,		DW_ARG,			1),
	asBCINFO(PshV4,		rW_ARG,			1),
	asBCINFO(PSF,		rW_ARG,			AS_PTR_SIZE),
	asBCINFO(SWAP4,		NO_ARG,			0),
	asBCINFO(NOT,		rW_ARG,			0),
	asBCINFO(PshG4,		W_ARG,			1),
	asBCINFO(LdGRdR4,	wW_W_ARG,		0),
	asBCINFO(CALL,		DW_ARG,			0xFFFF),
	asBCINFO(RET,		W_ARG,			0xFFFF),
	asBCINFO(JMP,		DW_ARG,			0),
	asBCINFO(JZ,		DW_ARG,			0),
	asBCINFO(JNZ,		DW_ARG,			0),
	asBCINFO(JS,		DW_ARG,			0),
	asBCINFO(JNS,		DW_ARG,			0),
	asBCINFO(JP,		DW_ARG,			0),
	asBCINFO(JNP,		DW_ARG,			0),
	asBCINFO(TZ,		NO_ARG,			0),
	asBCINFO(TNZ,		NO_ARG,			0),
	asBCINFO(TS,		NO_ARG,			0),
	asBCINFO(TNS,		NO_ARG,			0),
	asBCINFO(TP,		NO_ARG,			0),
	asBCINFO(TNP,		NO_ARG,			0),
	asBCINFO(NEGi,		rW_ARG,			0),
	asBCINFO(NEGf,		rW_ARG,			0),
	asBCINFO(NEGd,		rW_ARG,			0),
	asBCINFO(INCi16,	NO_ARG,			0),
	asBCINFO(INCi8,		NO_ARG,			0),
	asBCINFO(DECi16,	NO_ARG,			0),
	asBCINFO(DECi8,		NO_ARG,			0),
	asBCINFO(INCi,		NO_ARG,			0),
	asBCINFO(DECi,		NO_ARG,			0),
	asBCINFO(INCf,		NO_ARG,			0),
	asBCINFO(DECf,		NO_ARG,			0),
	asBCINFO(INCd,		NO_ARG,			0),
	asBCINFO(DECd,		NO_ARG,			0),
	asBCINFO(IncVi,		rW_ARG,			0),
	asBCINFO(DecVi,		rW_ARG,			0),
	asBCINFO(BNOT,		rW_ARG,			0),
	asBCINFO(BAND,		wW_rW_rW_ARG,	0),
	asBCINFO(BOR,		wW_rW_rW_ARG,	0),
	asBCINFO(BXOR,		wW_rW_rW_ARG,	0),
	asBCINFO(BSLL,		wW_rW_rW_ARG,	0),
	asBCINFO(BSRL,		wW_rW_rW_ARG,	0),
	asBCINFO(BSRA,		wW_rW_rW_ARG,	0),
	asBCINFO(COPY,		W_ARG,			-AS_PTR_SIZE),
	asBCINFO(PshC8,		QW_ARG,			2),
	asBCINFO(RDS8,		NO_ARG,			2-AS_PTR_SIZE),
	asBCINFO(SWAP8,		NO_ARG,			0),
	asBCINFO(CMPd,		rW_rW_ARG,		0),
	asBCINFO(CMPu,		rW_rW_ARG,		0),
	asBCINFO(CMPf,		rW_rW_ARG,		0),
	asBCINFO(CMPi,		rW_rW_ARG,		0),
	asBCINFO(CMPIi,		rW_DW_ARG,		0),
	asBCINFO(CMPIf,		rW_DW_ARG,		0),
	asBCINFO(CMPIu,		rW_DW_ARG,		0),
	asBCINFO(JMPP,		rW_ARG,			0),
	asBCINFO(PopRPtr,	NO_ARG,			-AS_PTR_SIZE),
	asBCINFO(PshRPtr,	NO_ARG,			AS_PTR_SIZE),
	asBCINFO(STR,		W_ARG,			1+AS_PTR_SIZE),
	asBCINFO(CALLSYS,	DW_ARG,			0xFFFF),
	asBCINFO(CALLBND,	DW_ARG,			0xFFFF),
	asBCINFO(SUSPEND,	NO_ARG,			0),
	asBCINFO(ALLOC,		PTR_DW_ARG,		0xFFFF),
	asBCINFO(FREE,		PTR_ARG,		-AS_PTR_SIZE),
	asBCINFO(LOADOBJ,	rW_ARG,			0),
	asBCINFO(STOREOBJ,	wW_ARG,			0),
	asBCINFO(GETOBJ,	W_ARG,			0),
	asBCINFO(REFCPY,	PTR_ARG,		-AS_PTR_SIZE),
	asBCINFO(CHKREF,	NO_ARG,			0),
	asBCINFO(GETOBJREF,	W_ARG,			0),
	asBCINFO(GETREF,	W_ARG,			0),
	asBCINFO(SWAP48,	NO_ARG,			0),
	asBCINFO(SWAP84,	NO_ARG,			0),
	asBCINFO(OBJTYPE,	PTR_ARG,		AS_PTR_SIZE),
	asBCINFO(TYPEID,	DW_ARG,			1),
	asBCINFO(SetV4,		wW_DW_ARG,		0),
	asBCINFO(SetV8,		wW_QW_ARG,		0),
	asBCINFO(ADDSi,		DW_ARG,			0),
	asBCINFO(CpyVtoV4,	wW_rW_ARG,		0),
	asBCINFO(CpyVtoV8,	wW_rW_ARG,		0),
	asBCINFO(CpyVtoR4,	rW_ARG,			0),
	asBCINFO(CpyVtoR8,	rW_ARG,			0),
	asBCINFO(CpyVtoG4,	W_rW_ARG,		0),
	asBCINFO(CpyRtoV4,	wW_ARG,			0),
	asBCINFO(CpyRtoV8,	wW_ARG,			0),
	asBCINFO(CpyGtoV4,	wW_W_ARG,		0),
	asBCINFO(WRTV1,		rW_ARG,			0),
	asBCINFO(WRTV2,		rW_ARG,			0),
	asBCINFO(WRTV4,		rW_ARG,			0),
	asBCINFO(WRTV8,		rW_ARG,			0),
	asBCINFO(RDR1,		wW_ARG,			0),
	asBCINFO(RDR2,		wW_ARG,			0),
	asBCINFO(RDR4,		wW_ARG,			0),
	asBCINFO(RDR8,		wW_ARG,			0),
	asBCINFO(LDG,		W_ARG,			0),
	asBCINFO(LDV,		rW_ARG,			0),
	asBCINFO(PGA,		W_ARG,			AS_PTR_SIZE),
	asBCINFO(RDS4,		NO_ARG,			1-AS_PTR_SIZE),
	asBCINFO(VAR,		rW_ARG,			AS_PTR_SIZE),
	asBCINFO(iTOf,		rW_ARG,			0),
	asBCINFO(fTOi,		rW_ARG,			0),
	asBCINFO(uTOf,		rW_ARG,			0),
	asBCINFO(fTOu,		rW_ARG,			0),
	asBCINFO(sbTOi,		rW_ARG,			0),
	asBCINFO(swTOi,		rW_ARG,			0),
	asBCINFO(ubTOi,		rW_ARG,			0),
	asBCINFO(uwTOi,		rW_ARG,			0),
	asBCINFO(dTOi,		wW_rW_ARG,		0),
	asBCINFO(dTOu,		wW_rW_ARG,		0),
	asBCINFO(dTOf,		wW_rW_ARG,		0),
	asBCINFO(iTOd,		wW_rW_ARG,		0),
	asBCINFO(uTOd,		wW_rW_ARG,		0),
	asBCINFO(fTOd,		wW_rW_ARG,		0),
	asBCINFO(ADDi,		wW_rW_rW_ARG,	0),
	asBCINFO(SUBi,		wW_rW_rW_ARG,	0),
	asBCINFO(MULi,		wW_rW_rW_ARG,	0),
	asBCINFO(DIVi,		wW_rW_rW_ARG,	0),
	asBCINFO(MODi,		wW_rW_rW_ARG,	0),
	asBCINFO(ADDf,		wW_rW_rW_ARG,	0),
	asBCINFO(SUBf,		wW_rW_rW_ARG,	0),
	asBCINFO(MULf,		wW_rW_rW_ARG,	0),
	asBCINFO(DIVf,		wW_rW_rW_ARG,	0),
	asBCINFO(MODf,		wW_rW_rW_ARG,	0),
	asBCINFO(ADDd,		wW_rW_rW_ARG,	0),
	asBCINFO(SUBd,		wW_rW_rW_ARG,	0),
	asBCINFO(MULd,		wW_rW_rW_ARG,	0),
	asBCINFO(DIVd,		wW_rW_rW_ARG,	0),
	asBCINFO(MODd,		wW_rW_rW_ARG,	0),
	asBCINFO(ADDIi,		wW_rW_DW_ARG,	0),
	asBCINFO(SUBIi,		wW_rW_DW_ARG,	0),
	asBCINFO(MULIi,		wW_rW_DW_ARG,	0),
	asBCINFO(ADDIf,		wW_rW_DW_ARG,	0),
	asBCINFO(SUBIf,		wW_rW_DW_ARG,	0),
	asBCINFO(MULIf,		wW_rW_DW_ARG,	0),
	asBCINFO(SetG4,		W_DW_ARG,		0),
	asBCINFO(ChkRefS,	NO_ARG,			0),
	asBCINFO(ChkNullV,	rW_ARG,			0),
	asBCINFO(CALLINTF,	DW_ARG,			0xFFFF),
	asBCINFO(iTOb,		rW_ARG,			0),
	asBCINFO(iTOw,		rW_ARG,			0),
	asBCINFO(SetV1,		wW_DW_ARG,		0),
	asBCINFO(SetV2,		wW_DW_ARG,		0),
	asBCINFO(Cast,		DW_ARG,			-AS_PTR_SIZE),
	asBCINFO(i64TOi,	wW_rW_ARG,		0),
	asBCINFO(uTOi64,	wW_rW_ARG,		0),
	asBCINFO(iTOi64,	wW_rW_ARG,		0),
	asBCINFO(fTOi64,	wW_rW_ARG,		0),
	asBCINFO(dTOi64,	rW_ARG,			0),
	asBCINFO(fTOu64,	wW_rW_ARG,		0),
	asBCINFO(dTOu64,	rW_ARG,			0),
	asBCINFO(i64TOf,	wW_rW_ARG,		0),
	asBCINFO(u64TOf,	wW_rW_ARG,		0),
	asBCINFO(i64TOd,	rW_ARG,			0),
	asBCINFO(u64TOd,	rW_ARG,			0),
	asBCINFO(NEGi64,	rW_ARG,			0),
	asBCINFO(INCi64,	NO_ARG,			0),
	asBCINFO(DECi64,	NO_ARG,			0),
	asBCINFO(BNOT64,	rW_ARG,			0),
	asBCINFO(ADDi64,	wW_rW_rW_ARG,	0),
	asBCINFO(SUBi64,	wW_rW_rW_ARG,	0),
	asBCINFO(MULi64,	wW_rW_rW_ARG,	0),
	asBCINFO(DIVi64,	wW_rW_rW_ARG,	0),
	asBCINFO(MODi64,	wW_rW_rW_ARG,	0),
	asBCINFO(BAND64,	wW_rW_rW_ARG,	0),
	asBCINFO(BOR64,		wW_rW_rW_ARG,	0),
	asBCINFO(BXOR64,	wW_rW_rW_ARG,	0),
	asBCINFO(BSLL64,	wW_rW_rW_ARG,	0),
	asBCINFO(BSRL64,	wW_rW_rW_ARG,	0),
	asBCINFO(BSRA64,	wW_rW_rW_ARG,	0),
	asBCINFO(CMPi64,	rW_rW_ARG,		0),
	asBCINFO(CMPu64,	rW_rW_ARG,		0),
	asBCINFO(ChkNullS,	W_ARG,			0),
	asBCINFO(ClrHi,		NO_ARG,			0),
	asBCINFO(JitEntry,	W_ARG,			0),

	asBCINFO_DUMMY(176),
	asBCINFO_DUMMY(177),
	asBCINFO_DUMMY(178),
	asBCINFO_DUMMY(179),
	asBCINFO_DUMMY(180),
	asBCINFO_DUMMY(181),
	asBCINFO_DUMMY(182),
	asBCINFO_DUMMY(183),
	asBCINFO_DUMMY(184),
	asBCINFO_DUMMY(185),
	asBCINFO_DUMMY(186),
	asBCINFO_DUMMY(187),
	asBCINFO_DUMMY(188),
	asBCINFO_DUMMY(189),
	asBCINFO_DUMMY(190),
	asBCINFO_DUMMY(191),
	asBCINFO_DUMMY(192),
	asBCINFO_DUMMY(193),
	asBCINFO_DUMMY(194),
	asBCINFO_DUMMY(195),
	asBCINFO_DUMMY(196),
	asBCINFO_DUMMY(197),
	asBCINFO_DUMMY(198),
	asBCINFO_DUMMY(199),
	asBCINFO_DUMMY(200),
	asBCINFO_DUMMY(201),
	asBCINFO_DUMMY(202),
	asBCINFO_DUMMY(203),
	asBCINFO_DUMMY(204),
	asBCINFO_DUMMY(205),
	asBCINFO_DUMMY(206),
	asBCINFO_DUMMY(207),
	asBCINFO_DUMMY(208),
	asBCINFO_DUMMY(209),
	asBCINFO_DUMMY(210),
	asBCINFO_DUMMY(211),
	asBCINFO_DUMMY(212),
	asBCINFO_DUMMY(213),
	asBCINFO_DUMMY(214),
	asBCINFO_DUMMY(215),
	asBCINFO_DUMMY(216),
	asBCINFO_DUMMY(217),
	asBCINFO_DUMMY(218),
	asBCINFO_DUMMY(219),
	asBCINFO_DUMMY(220),
	asBCINFO_DUMMY(221),
	asBCINFO_DUMMY(222),
	asBCINFO_DUMMY(223),
	asBCINFO_DUMMY(224),
	asBCINFO_DUMMY(225),
	asBCINFO_DUMMY(226),
	asBCINFO_DUMMY(227),
	asBCINFO_DUMMY(228),
	asBCINFO_DUMMY(229),
	asBCINFO_DUMMY(230),
	asBCINFO_DUMMY(231),
	asBCINFO_DUMMY(232),
	asBCINFO_DUMMY(233),
	asBCINFO_DUMMY(234),
	asBCINFO_DUMMY(235),
	asBCINFO_DUMMY(236),
	asBCINFO_DUMMY(237),
	asBCINFO_DUMMY(238),
	asBCINFO_DUMMY(239),
	asBCINFO_DUMMY(240),
	asBCINFO_DUMMY(241),
	asBCINFO_DUMMY(242),
	asBCINFO_DUMMY(243),
	asBCINFO_DUMMY(244),
	asBCINFO_DUMMY(245),
	asBCINFO_DUMMY(246),
	asBCINFO_DUMMY(247),
	asBCINFO_DUMMY(248),
	asBCINFO_DUMMY(249),
	asBCINFO_DUMMY(250),
	asBCINFO_DUMMY(251),
	asBCINFO_DUMMY(252),

	asBCINFO(PSP,		W_ARG,			AS_PTR_SIZE),
	asBCINFO(LINE,		INFO,			0xFFFF),
	asBCINFO(LABEL,		INFO,			0xFFFF)
};

// Macros to access bytecode instruction arguments
//! \brief Macro to access the first DWORD argument in the bytecode instruction
#define asBC_DWORDARG(x)  (asDWORD(*(x+1)))
//! \brief Macro to access the first 32bit integer argument in the bytecode instruction
#define asBC_INTARG(x)    (int(*(x+1)))
//! \brief Macro to access the first QWORD argument in the bytecode instruction
#define asBC_QWORDARG(x)  (*(asQWORD*)(x+1))
//! \brief Macro to access the first float argument in the bytecode instruction
#define asBC_FLOATARG(x)  (*(float*)(x+1))
//! \brief Macro to access the first pointer argument in the bytecode instruction
#define asBC_PTRARG(x)    (asPTRWORD(*(x+1)))
//! \brief Macro to access the first WORD argument in the bytecode instruction
#define asBC_WORDARG0(x)  (*(((asWORD*)x)+1))
//! \brief Macro to access the second WORD argument in the bytecode instruction
#define asBC_WORDARG1(x)  (*(((asWORD*)x)+2))
//! \brief Macro to access the first signed WORD argument in the bytecode instruction
#define asBC_SWORDARG0(x) (*(((short*)x)+1))
//! \brief Macro to access the second signed WORD argument in the bytecode instruction
#define asBC_SWORDARG1(x) (*(((short*)x)+2))
//! \brief Macro to access the third signed WORD argument in the bytecode instruction
#define asBC_SWORDARG2(x) (*(((short*)x)+3))


END_AS_NAMESPACE

#endif
