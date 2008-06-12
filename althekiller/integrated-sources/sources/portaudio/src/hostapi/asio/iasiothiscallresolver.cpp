/*
	IASIOThiscallResolver.cpp see the comments in iasiothiscallresolver.h for
    the top level description - this comment describes the technical details of
    the implementation.

    The latest version of this file is available from:
    http://www.audiomulch.com/~rossb/code/calliasio

    please email comments to Ross Bencina <rossb@audiomulch.com>

    BACKGROUND

    The IASIO interface declared in the Steinberg ASIO 2 SDK declares
    functions with no explicit calling convention. This causes MSVC++ to default
    to using the thiscall convention, which is a proprietary convention not
    implemented by some non-microsoft compilers - notably borland BCC,
    C++Builder, and gcc. MSVC++ is the defacto standard compiler used by
    Steinberg. As a result of this situation, the ASIO sdk will compile with
    any compiler, however attempting to execute the compiled code will cause a
    crash due to different default calling conventions on non-Microsoft
    compilers.

    IASIOThiscallResolver solves the problem by providing an adapter class that
    delegates to the IASIO interface using the correct calling convention
    (thiscall). Due to the lack of support for thiscall in the Borland and GCC
    compilers, the calls have been implemented in assembly language.

    A number of macros are defined for thiscall function calls with different
    numbers of parameters, with and without return values - it may be possible
    to modify the format of these macros to make them work with other inline
    assemblers.


    THISCALL DEFINITION

    A number of definitions of the thiscall calling convention are floating
    around the internet. The following definition has been validated against
    output from the MSVC++ compiler:

    For non-vararg functions, thiscall works as follows: the object (this)
    pointer is passed in ECX. All arguments are passed on the stack in
    right to left order. The return value is placed in EAX. The callee
    clears the passed arguments from the stack.


    FINDING FUNCTION POINTERS FROM AN IASIO POINTER

    The first field of a COM object is a pointer to its vtble. Thus a pointer
    to an object implementing the IASIO interface also points to a pointer to
    that object's vtbl. The vtble is a table of function pointers for all of
    the virtual functions exposed by the implemented interfaces.

    If we consider a variable declared as a pointer to IASO:

    IASIO *theAsioDriver

    theAsioDriver points to:

    object implementing IASIO
    {
        IASIOvtbl *vtbl
        other data
    }

    in other words, theAsioDriver points to a pointer to an IASIOvtbl

    vtbl points to a table of function pointers:

    IASIOvtbl ( interface IASIO : public IUnknown )
    {
    (IUnknown functions)
    0   virtual HRESULT STDMETHODCALLTYPE (*QueryInterface)(REFIID riid, void **ppv) = 0;
    4   virtual ULONG STDMETHODCALLTYPE (*AddRef)() = 0;
    8   virtual ULONG STDMETHODCALLTYPE (*Release)() = 0;      

    (IASIO functions)
    12	virtual ASIOBool (*init)(void *sysHandle) = 0;
    16	virtual void (*getDriverName)(char *name) = 0;
    20	virtual long (*getDriverVersion)() = 0;
    24	virtual void (*getErrorMessage)(char *string) = 0;
    28	virtual ASIOError (*start)() = 0;
    32	virtual ASIOError (*stop)() = 0;
    36	virtual ASIOError (*getChannels)(long *numInputChannels, long *numOutputChannels) = 0;
    40	virtual ASIOError (*getLatencies)(long *inputLatency, long *outputLatency) = 0;
    44	virtual ASIOError (*getBufferSize)(long *minSize, long *maxSize,
            long *preferredSize, long *granularity) = 0;
    48	virtual ASIOError (*canSampleRate)(ASIOSampleRate sampleRate) = 0;
    52	virtual ASIOError (*getSampleRate)(ASIOSampleRate *sampleRate) = 0;
    56	virtual ASIOError (*setSampleRate)(ASIOSampleRate sampleRate) = 0;
    60	virtual ASIOError (*getClockSources)(ASIOClockSource *clocks, long *numSources) = 0;
    64	virtual ASIOError (*setClockSource)(long reference) = 0;
    68	virtual ASIOError (*getSamplePosition)(ASIOSamples *sPos, ASIOTimeStamp *tStamp) = 0;
    72	virtual ASIOError (*getChannelInfo)(ASIOChannelInfo *info) = 0;
    76	virtual ASIOError (*createBuffers)(ASIOBufferInfo *bufferInfos, long numChannels,
            long bufferSize, ASIOCallbacks *callbacks) = 0;
    80	virtual ASIOError (*disposeBuffers)() = 0;
    84	virtual ASIOError (*controlPanel)() = 0;
    88	virtual ASIOError (*future)(long selector,void *opt) = 0;
    92	virtual ASIOError (*outputReady)() = 0;
    };

    The numbers in the left column show the byte offset of each function ptr
    from the beginning of the vtbl. These numbers are used in the code below
    to select different functions.

    In order to find the address of a particular function, theAsioDriver
    must first be dereferenced to find the value of the vtbl pointer:

    mov     eax, theAsioDriver
    mov     edx, [theAsioDriver]  // edx now points to vtbl[0]

    Then an offset must be added to the vtbl pointer to select a
    particular function, for example vtbl+44 points to the slot containing
    a pointer to the getBufferSize function.

    Finally vtbl+x must be dereferenced to obtain the value of the function
    pointer stored in that address:

    call    [edx+44]    // call the function pointed to by
                        // the value in the getBufferSize field of the vtbl


    SEE ALSO

    Martin Fay's OpenASIO DLL at http://www.martinfay.com solves the same
    problem by providing a new COM interface which wraps IASIO with an
    interface that uses portable calling conventions. OpenASIO must be compiled
    with MSVC, and requires that you ship the OpenASIO DLL with your
    application.

    
    ACKNOWLEDGEMENTS

    Ross Bencina: worked out the thiscall details above, wrote the original
    Borland asm macros, and a patch for asio.cpp (which is no longer needed).
    Thanks to Martin Fay for introducing me to the issues discussed here,
    and to Rene G. Ceballos for assisting with asm dumps from MSVC++.

    Antti Silvast: converted the original calliasio to work with gcc and NASM
    by implementing the asm code in a separate file.

	Fraser Adams: modified the original calliasio containing the Borland inline
    asm to add inline asm for gcc i.e. Intel syntax for Borland and AT&T syntax
    for gcc. This seems a neater approach for gcc than to have a separate .asm
    file and it means that we only need one version of the thiscall patch.

    Fraser Adams: rewrote the original calliasio patch in the form of the
    IASIOThiscallResolver class in order to avoid modifications to files from
    the Steinberg SDK, which may have had potential licence issues.

    Andrew Baldwin: contributed fixes for compatibility problems with more
    recent versions of the gcc assembler.
*/


// We only need IASIOThiscallResolver at all if we are on Win32. For other
// platforms we simply bypass the IASIOThiscallResolver definition to allow us
// to be safely #include'd whatever the platform to keep client code portable
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)


// If microsoft compiler we can call IASIO directly so IASIOThiscallResolver
// is not used.
#if !defined(_MSC_VER)


#include <new>
#include <assert.h>

// We have a mechanism in iasiothiscallresolver.h to ensure that asio.h is
// #include'd before it in client code, we do NOT want to do this test here.
#define iasiothiscallresolver_sourcefile 1
#include "iasiothiscallresolver.h"
#undef iasiothiscallresolver_sourcefile

// iasiothiscallresolver.h redefines ASIOInit for clients, but we don't want
// this macro defined in this translation unit.
#undef ASIOInit


// theAsioDriver is a global pointer to the current IASIO instance which the
// ASIO SDK uses to perform all actions on the IASIO interface. We substitute
// our own forwarding interface into this pointer.
extern IASIO* theAsioDriver;


// The following macros define the inline assembler for BORLAND first then gcc

#if defined(__BCPLUSPLUS__) || defined(__BORLANDC__)          


#define CALL_THISCALL_0( resultName, thisPtr, funcOffset )\
    void *this_ = (thisPtr);                                                \
    __asm {                                                                 \
        mov     ecx, this_            ;                                     \
        mov     eax, [ecx]            ;                                     \
        call    [eax+funcOffset]      ;                                     \
        mov     resultName, eax       ;                                     \
    }


#define CALL_VOID_THISCALL_1( thisPtr, funcOffset, param1 )\
    void *this_ = (thisPtr);                                                \
    __asm {                                                                 \
        mov     eax, param1           ;                                     \
        push    eax                   ;                                     \
        mov     ecx, this_            ;                                     \
        mov     eax, [ecx]            ;                                     \
        call    [eax+funcOffset]      ;                                     \
    }


#define CALL_THISCALL_1( resultName, thisPtr, funcOffset, param1 )\
    void *this_ = (thisPtr);                                                \
    __asm {                                                                 \
        mov     eax, param1           ;                                     \
        push    eax                   ;                                     \
        mov     ecx, this_            ;                                     \
        mov     eax, [ecx]            ;                                     \
        call    [eax+funcOffset]      ;                                     \
        mov     resultName, eax       ;                                     \
    }


#define CALL_THISCALL_1_DOUBLE( resultName, thisPtr, funcOffset, param1 )\
    void *this_ = (thisPtr);                                                \
    void *doubleParamPtr_ (&param1);                                        \
    __asm {                                                                 \
        mov     eax, doubleParamPtr_  ;                                     \
        push    [eax+4]               ;                                     \
        push    [eax]                 ;                                     \
        mov     ecx, this_            ;                                     \
        mov     eax, [ecx]            ;                                     \
        call    [eax+funcOffset]      ;                                     \
        mov     resultName, eax       ;                                     \
    }


#define CALL_THISCALL_2( resultName, thisPtr, funcOffset, param1, param2 )\
    void *this_ = (thisPtr);                                                \
    __asm {                                                                 \
        mov     eax, param2           ;                                     \
        push    eax                   ;                                     \
        mov     eax, param1           ;                                     \
        push    eax                   ;                                     \
        mov     ecx, this_            ;                                     \
        mov     eax, [ecx]            ;                                     \
        call    [eax+funcOffset]      ;                                     \
        mov     resultName, eax       ;                                     \
    }


#define CALL_THISCALL_4( resultName, thisPtr, funcOffset, param1, param2, param3, param4 )\
    void *this_ = (thisPtr);                                                \
    __asm {                                                                 \
        mov     eax, param4           ;                                     \
        push    eax                   ;                                     \
        mov     eax, param3           ;                                     \
        push    eax                   ;                                     \
        mov     eax, param2           ;                                     \
        push    eax                   ;                                     \
        mov     eax, param1           ;                                     \
        push    eax                   ;                                     \
        mov     ecx, this_            ;                                     \
        mov     eax, [ecx]            ;                                     \
        call    [eax+funcOffset]      ;                                     \
        mov     resultName, eax       ;                                     \
    }


#elif defined(__GNUC__)


#define CALL_THISCALL_0( resultName, thisPtr, funcOffset )                  \
    __asm__ __volatile__ ("movl (%1), %%edx\n\t"                            \
                          "call *"#funcOffset"(%%edx)\n\t"                  \
                          :"=a"(resultName) /* Output Operands */           \
                          :"c"(thisPtr)     /* Input Operands */            \
                         );                                                 \


#define CALL_VOID_THISCALL_1( thisPtr, funcOffset, param1 )                 \
    __asm__ __volatile__ ("pushl %0\n\t"                                    \
                          "movl (%1), %%edx\n\t"                            \
                          "call *"#funcOffset"(%%edx)\n\t"                  \
                          :                 /* Output Operands */           \
                          :"r"(param1),     /* Input Operands */            \
                           "c"(thisPtr)                                     \
                         );                                                 \


#define CALL_THISCALL_1( resultName, thisPtr, funcOffset, param1 )          \
    __asm__ __volatile__ ("pushl %1\n\t"                                    \
                          "movl (%2), %%edx\n\t"                            \
                          "call *"#funcOffset"(%%edx)\n\t"                  \
                          :"=a"(resultName) /* Output Operands */           \
                          :"r"(param1),     /* Input Operands */            \
                           "c"(thisPtr)                                     \
                          );                                                \


#define CALL_THISCALL_1_DOUBLE( resultName, thisPtr, funcOffset, param1 )   \
    __asm__ __volatile__ ("pushl 4(%1)\n\t"                                 \
                          "pushl (%1)\n\t"                                  \
                          "movl (%2), %%edx\n\t"                            \
                          "call *"#funcOffset"(%%edx);\n\t"                 \
                          :"=a"(resultName) /* Output Operands */           \
                          :"a"(&param1),    /* Input Operands */            \
                           /* Note: Using "r" above instead of "a" fails */ \
                           /* when using GCC 3.3.3, and maybe later versions*/\
                           "c"(thisPtr)                                     \
                          );                                                \


#define CALL_THISCALL_2( resultName, thisPtr, funcOffset, param1, param2 )  \
    __asm__ __volatile__ ("pushl %1\n\t"                                    \
                          "pushl %2\n\t"                                    \
                          "movl (%3), %%edx\n\t"                            \
                          "call *"#funcOffset"(%%edx)\n\t"                  \
                          :"=a"(resultName) /* Output Operands */           \
                          :"r"(param2),     /* Input Operands */            \
                           "r"(param1),                                     \
                           "c"(thisPtr)                                     \
                          );                                                \


#define CALL_THISCALL_4( resultName, thisPtr, funcOffset, param1, param2, param3, param4 )\
    __asm__ __volatile__ ("pushl %1\n\t"                                    \
                          "pushl %2\n\t"                                    \
                          "pushl %3\n\t"                                    \
                          "pushl %4\n\t"                                    \
                          "movl (%5), %%edx\n\t"                            \
                          "call *"#funcOffset"(%%edx)\n\t"                  \
                          :"=a"(resultName) /* Output Operands */           \
                          :"r"(param4),     /* Input Operands  */           \
                           "r"(param3),                                     \
                           "r"(param2),                                     \
                           "r"(param1),                                     \
                           "c"(thisPtr)                                     \
                          );                                                \

#endif



// Our static singleton instance.
IASIOThiscallResolver IASIOThiscallResolver::instance;

// Constructor called to initialize static Singleton instance above. Note that
// it is important not to clear that_ incase it has already been set by the call
// to placement new in ASIOInit().
IASIOThiscallResolver::IASIOThiscallResolver()
{
}

// Constructor called from ASIOInit() below
IASIOThiscallResolver::IASIOThiscallResolver(IASIO* that)
: that_( that )
{
}

// Implement IUnknown methods as assert(false). IASIOThiscallResolver is not
// really a COM object, just a wrapper which will work with the ASIO SDK.
// If you wanted to use ASIO without the SDK you might want to implement COM
// aggregation in these methods.
HRESULT STDMETHODCALLTYPE IASIOThiscallResolver::QueryInterface(REFIID riid, void **ppv)
{
    (void)riid;     // suppress unused variable warning

    assert( false ); // this function should never be called by the ASIO SDK.

    *ppv = NULL;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE IASIOThiscallResolver::AddRef()
{
    assert( false ); // this function should never be called by the ASIO SDK.

    return 1;
}

ULONG STDMETHODCALLTYPE IASIOThiscallResolver::Release()
{
    assert( false ); // this function should never be called by the ASIO SDK.
    
    return 1;
}


// Implement the IASIO interface methods by performing the vptr manipulation
// described above then delegating to the real implementation.
ASIOBool IASIOThiscallResolver::init(void *sysHandle)
{
    ASIOBool result;
    CALL_THISCALL_1( result, that_, 12, sysHandle );
    return result;
}

void IASIOThiscallResolver::getDriverName(char *name)
{
    CALL_VOID_THISCALL_1( that_, 16, name );
}

long IASIOThiscallResolver::getDriverVersion()
{
    ASIOBool result;
    CALL_THISCALL_0( result, that_, 20 );
    return result;
}

void IASIOThiscallResolver::getErrorMessage(char *string)
{
     CALL_VOID_THISCALL_1( that_, 24, string );
}

ASIOError IASIOThiscallResolver::start()
{
    ASIOBool result;
    CALL_THISCALL_0( result, that_, 28 );
    return result;
}

ASIOError IASIOThiscallResolver::stop()
{
    ASIOBool result;
    CALL_THISCALL_0( result, that_, 32 );
    return result;
}

ASIOError IASIOThiscallResolver::getChannels(long *numInputChannels, long *numOutputChannels)
{
    ASIOBool result;
    CALL_THISCALL_2( result, that_, 36, numInputChannels, numOutputChannels );
    return result;
}

ASIOError IASIOThiscallResolver::getLatencies(long *inputLatency, long *outputLatency)
{
    ASIOBool result;
    CALL_THISCALL_2( result, that_, 40, inputLatency, outputLatency );
    return result;
}

ASIOError IASIOThiscallResolver::getBufferSize(long *minSize, long *maxSize,
        long *preferredSize, long *granularity)
{
    ASIOBool result;
    CALL_THISCALL_4( result, that_, 44, minSize, maxSize, preferredSize, granularity );
    return result;
}

ASIOError IASIOThiscallResolver::canSampleRate(ASIOSampleRate sampleRate)
{
    ASIOBool result;
    CALL_THISCALL_1_DOUBLE( result, that_, 48, sampleRate );
    return result;
}

ASIOError IASIOThiscallResolver::getSampleRate(ASIOSampleRate *sampleRate)
{
    ASIOBool result;
    CALL_THISCALL_1( result, that_, 52, sampleRate );
    return result;
}

ASIOError IASIOThiscallResolver::setSampleRate(ASIOSampleRate sampleRate)
{    
    ASIOBool result;
    CALL_THISCALL_1_DOUBLE( result, that_, 56, sampleRate );
    return result;
}

ASIOError IASIOThiscallResolver::getClockSources(ASIOClockSource *clocks, long *numSources)
{
    ASIOBool result;
    CALL_THISCALL_2( result, that_, 60, clocks, numSources );
    return result;
}

ASIOError IASIOThiscallResolver::setClockSource(long reference)
{
    ASIOBool result;
    CALL_THISCALL_1( result, that_, 64, reference );
    return result;
}

ASIOError IASIOThiscallResolver::getSamplePosition(ASIOSamples *sPos, ASIOTimeStamp *tStamp)
{
    ASIOBool result;
    CALL_THISCALL_2( result, that_, 68, sPos, tStamp );
    return result;
}

ASIOError IASIOThiscallResolver::getChannelInfo(ASIOChannelInfo *info)
{
    ASIOBool result;
    CALL_THISCALL_1( result, that_, 72, info );
    return result;
}

ASIOError IASIOThiscallResolver::createBuffers(ASIOBufferInfo *bufferInfos,
        long numChannels, long bufferSize, ASIOCallbacks *callbacks)
{
    ASIOBool result;
    CALL_THISCALL_4( result, that_, 76, bufferInfos, numChannels, bufferSize, callbacks );
    return result;
}

ASIOError IASIOThiscallResolver::disposeBuffers()
{
    ASIOBool result;
    CALL_THISCALL_0( result, that_, 80 );
    return result;
}

ASIOError IASIOThiscallResolver::controlPanel()
{
    ASIOBool result;
    CALL_THISCALL_0( result, that_, 84 );
    return result;
}

ASIOError IASIOThiscallResolver::future(long selector,void *opt)
{
    ASIOBool result;
    CALL_THISCALL_2( result, that_, 88, selector, opt );
    return result;
}

ASIOError IASIOThiscallResolver::outputReady()
{
    ASIOBool result;
    CALL_THISCALL_0( result, that_, 92 );
    return result;
}


// Implement our substitute ASIOInit() method
ASIOError IASIOThiscallResolver::ASIOInit(ASIODriverInfo *info)
{
    // To ensure that our instance's vptr is correctly constructed, even if
    // ASIOInit is called prior to main(), we explicitly call its constructor
    // (potentially over the top of an existing instance). Note that this is
    // pretty ugly, and is only safe because IASIOThiscallResolver has no
    // destructor and contains no objects with destructors.
    new((void*)&instance) IASIOThiscallResolver( theAsioDriver );

    // Interpose between ASIO client code and the real driver.
    theAsioDriver = &instance;

    // Note that we never need to switch theAsioDriver back to point to the
    // real driver because theAsioDriver is reset to zero in ASIOExit().

    // Delegate to the real ASIOInit
	return ::ASIOInit(info);
}


#endif /* !defined(_MSC_VER) */

#endif /* Win32 */

