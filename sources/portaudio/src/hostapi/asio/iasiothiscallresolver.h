// ****************************************************************************
// File:			IASIOThiscallResolver.h
// Description:     The IASIOThiscallResolver class implements the IASIO
//					interface and acts as a proxy to the real IASIO interface by
//                  calling through its vptr table using the thiscall calling
//                  convention. To put it another way, we interpose
//                  IASIOThiscallResolver between ASIO SDK code and the driver.
//                  This is necessary because most non-Microsoft compilers don't
//                  implement the thiscall calling convention used by IASIO.
//
//					iasiothiscallresolver.cpp contains the background of this
//					problem plus a technical description of the vptr
//                  manipulations.
//
//					In order to use this mechanism one simply has to add
//					iasiothiscallresolver.cpp to the list of files to compile
//                  and #include <iasiothiscallresolver.h>
//
//					Note that this #include must come after the other ASIO SDK
//                  #includes, for example:
//
//					#include <windows.h>
//					#include <asiosys.h>
//					#include <asio.h>
//					#include <asiodrivers.h>
//					#include <iasiothiscallresolver.h>
//
//					Actually the important thing is to #include
//                  <iasiothiscallresolver.h> after <asio.h>. We have
//                  incorporated a test to enforce this ordering.
//
//					The code transparently takes care of the interposition by
//                  using macro substitution to intercept calls to ASIOInit()
//                  and ASIOExit(). We save the original ASIO global
//                  "theAsioDriver" in our "that" variable, and then set
//                  "theAsioDriver" to equal our IASIOThiscallResolver instance.
//
// 					Whilst this method of resolving the thiscall problem requires
//					the addition of #include <iasiothiscallresolver.h> to client
//                  code it has the advantage that it does not break the terms
//                  of the ASIO licence by publishing it. We are NOT modifying
//                  any Steinberg code here, we are merely implementing the IASIO
//					interface in the same way that we would need to do if we
//					wished to provide an open source ASIO driver.
//
//					For compilation with MinGW -lole32 needs to be added to the
//                  linker options. For BORLAND, linking with Import32.lib is
//                  sufficient.
//
//					The dependencies are with: CoInitialize, CoUninitialize,
//					CoCreateInstance, CLSIDFromString - used by asiolist.cpp
//					and are required on Windows whether ThiscallResolver is used
//					or not.
//
//					Searching for the above strings in the root library path
//					of your compiler should enable the correct libraries to be
//					identified if they aren't immediately obvious.
//
//                  Note that the current implementation of IASIOThiscallResolver
//                  is not COM compliant - it does not correctly implement the
//                  IUnknown interface. Implementing it is not necessary because
//                  it is not called by parts of the ASIO SDK which call through
//                  theAsioDriver ptr. The IUnknown methods are implemented as
//                  assert(false) to ensure that the code fails if they are
//                  ever called.
// Restrictions:	None. Public Domain & Open Source distribute freely
//					You may use IASIOThiscallResolver commercially as well as
//                  privately.
//					You the user assume the responsibility for the use of the
//					files, binary or text, and there is no guarantee or warranty,
//					expressed or implied, including but not limited to the
//					implied warranties of merchantability and fitness for a
//					particular purpose. You assume all responsibility and agree
//					to hold no entity, copyright holder or distributors liable
//					for any loss of data or inaccurate representations of data
//					as a result of using IASIOThiscallResolver.
// Version:         1.4 Added separate macro CALL_THISCALL_1_DOUBLE from
//                  Andrew Baldwin, and volatile for whole gcc asm blocks,
//                  both for compatibility with newer gcc versions. Cleaned up
//                  Borland asm to use one less register.
//                  1.3 Switched to including assert.h for better compatibility.
//                  Wrapped entire .h and .cpp contents with a check for
//                  _MSC_VER to provide better compatibility with MS compilers.
//                  Changed Singleton implementation to use static instance
//                  instead of freestore allocated instance. Removed ASIOExit
//                  macro as it is no longer needed.
//                  1.2 Removed semicolons from ASIOInit and ASIOExit macros to
//                  allow them to be embedded in expressions (if statements).
//                  Cleaned up some comments. Removed combase.c dependency (it
//                  doesn't compile with BCB anyway) by stubbing IUnknown.
//                  1.1 Incorporated comments from Ross Bencina including things
//					such as changing name from ThiscallResolver to
//					IASIOThiscallResolver, tidying up the constructor, fixing
//					a bug in IASIOThiscallResolver::ASIOExit() and improving
//					portability through the use of conditional compilation
//					1.0 Initial working version.
// Created:			6/09/2003
// Authors:         Fraser Adams
//                  Ross Bencina
//                  Rene G. Ceballos
//                  Martin Fay
//                  Antti Silvast
//                  Andrew Baldwin
//
// ****************************************************************************


#ifndef included_iasiothiscallresolver_h
#define included_iasiothiscallresolver_h

// We only need IASIOThiscallResolver at all if we are on Win32. For other
// platforms we simply bypass the IASIOThiscallResolver definition to allow us
// to be safely #include'd whatever the platform to keep client code portable
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)


// If microsoft compiler we can call IASIO directly so IASIOThiscallResolver
// is not used.
#if !defined(_MSC_VER)


// The following is in order to ensure that this header is only included after
// the other ASIO headers (except for the case of iasiothiscallresolver.cpp).
// We need to do this because IASIOThiscallResolver works by eclipsing the
// original definition of ASIOInit() with a macro (see below).
#if !defined(iasiothiscallresolver_sourcefile)
	#if !defined(__ASIO_H)
	#error iasiothiscallresolver.h must be included AFTER asio.h
	#endif
#endif

#include <windows.h>
#include <asiodrvr.h> /* From ASIO SDK */


class IASIOThiscallResolver : public IASIO {
private:
	IASIO* that_; // Points to the real IASIO

	static IASIOThiscallResolver instance; // Singleton instance

	// Constructors - declared private so construction is limited to
    // our Singleton instance
    IASIOThiscallResolver();
	IASIOThiscallResolver(IASIO* that);
public:

    // Methods from the IUnknown interface. We don't fully implement IUnknown
    // because the ASIO SDK never calls these methods through theAsioDriver ptr.
    // These methods are implemented as assert(false).
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    // Methods from the IASIO interface, implemented as forwarning calls to that.
	virtual ASIOBool init(void *sysHandle);
	virtual void getDriverName(char *name);
	virtual long getDriverVersion();
	virtual void getErrorMessage(char *string);
	virtual ASIOError start();
	virtual ASIOError stop();
	virtual ASIOError getChannels(long *numInputChannels, long *numOutputChannels);
	virtual ASIOError getLatencies(long *inputLatency, long *outputLatency);
	virtual ASIOError getBufferSize(long *minSize, long *maxSize, long *preferredSize, long *granularity);
	virtual ASIOError canSampleRate(ASIOSampleRate sampleRate);
	virtual ASIOError getSampleRate(ASIOSampleRate *sampleRate);
	virtual ASIOError setSampleRate(ASIOSampleRate sampleRate);
	virtual ASIOError getClockSources(ASIOClockSource *clocks, long *numSources);
	virtual ASIOError setClockSource(long reference);
	virtual ASIOError getSamplePosition(ASIOSamples *sPos, ASIOTimeStamp *tStamp);
	virtual ASIOError getChannelInfo(ASIOChannelInfo *info);
	virtual ASIOError createBuffers(ASIOBufferInfo *bufferInfos, long numChannels, long bufferSize, ASIOCallbacks *callbacks);
	virtual ASIOError disposeBuffers();
	virtual ASIOError controlPanel();
	virtual ASIOError future(long selector,void *opt);
	virtual ASIOError outputReady();

    // Class method, see ASIOInit() macro below.
    static ASIOError ASIOInit(ASIODriverInfo *info); // Delegates to ::ASIOInit
};


// Replace calls to ASIOInit with our interposing version.
// This macro enables us to perform thiscall resolution simply by #including
// <iasiothiscallresolver.h> after the asio #includes (this file _must_ be
// included _after_ the asio #includes)

#define ASIOInit(name) IASIOThiscallResolver::ASIOInit((name))


#endif /* !defined(_MSC_VER) */

#endif /* Win32 */

#endif /* included_iasiothiscallresolver_h */


