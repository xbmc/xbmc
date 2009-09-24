/*==========================================================================;
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsound.h
 *  Content:    DirectSound include file
 *
 **************************************************************************/

#define COM_NO_WINDOWS_H
#include <objbase.h>
#include <float.h>

#ifndef DIRECTSOUND_VERSION
#define DIRECTSOUND_VERSION 0x0800  /* Version 8.0 */
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifndef __DSOUND_INCLUDED__
#define __DSOUND_INCLUDED__

/* Type definitions shared with Direct3D */

#ifndef DX_SHARED_DEFINES

typedef float D3DVALUE, *LPD3DVALUE;

#ifndef D3DCOLOR_DEFINED
typedef DWORD D3DCOLOR;
#define D3DCOLOR_DEFINED
#endif

#ifndef LPD3DCOLOR_DEFINED
typedef DWORD *LPD3DCOLOR;
#define LPD3DCOLOR_DEFINED
#endif

#ifndef D3DVECTOR_DEFINED
typedef struct _D3DVECTOR {
    float x;
    float y;
    float z;
} D3DVECTOR;
#define D3DVECTOR_DEFINED
#endif

#ifndef LPD3DVECTOR_DEFINED
typedef D3DVECTOR *LPD3DVECTOR;
#define LPD3DVECTOR_DEFINED
#endif

#define DX_SHARED_DEFINES
#endif // DX_SHARED_DEFINES

#define _FACDS  0x878   /* DirectSound's facility code */
#define MAKE_DSHRESULT(code)  MAKE_HRESULT(1, _FACDS, code)

// DirectSound Component GUID {47D4D946-62E8-11CF-93BC-444553540000}
DEFINE_GUID(CLSID_DirectSound, 0x47d4d946, 0x62e8, 0x11cf, 0x93, 0xbc, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);

// DirectSound 8.0 Component GUID {3901CC3F-84B5-4FA4-BA35-AA8172B8A09B}
DEFINE_GUID(CLSID_DirectSound8, 0x3901cc3f, 0x84b5, 0x4fa4, 0xba, 0x35, 0xaa, 0x81, 0x72, 0xb8, 0xa0, 0x9b);

// DirectSound Capture Component GUID {B0210780-89CD-11D0-AF08-00A0C925CD16}
DEFINE_GUID(CLSID_DirectSoundCapture, 0xb0210780, 0x89cd, 0x11d0, 0xaf, 0x8, 0x0, 0xa0, 0xc9, 0x25, 0xcd, 0x16);

// DirectSound 8.0 Capture Component GUID {E4BCAC13-7F99-4908-9A8E-74E3BF24B6E1}
DEFINE_GUID(CLSID_DirectSoundCapture8, 0xe4bcac13, 0x7f99, 0x4908, 0x9a, 0x8e, 0x74, 0xe3, 0xbf, 0x24, 0xb6, 0xe1);

// DirectSound Full Duplex Component GUID {FEA4300C-7959-4147-B26A-2377B9E7A91D}
DEFINE_GUID(CLSID_DirectSoundFullDuplex, 0xfea4300c, 0x7959, 0x4147, 0xb2, 0x6a, 0x23, 0x77, 0xb9, 0xe7, 0xa9, 0x1d);


// DirectSound default playback device GUID {DEF00000-9C6D-47ED-AAF1-4DDA8F2B5C03}
DEFINE_GUID(DSDEVID_DefaultPlayback, 0xdef00000, 0x9c6d, 0x47ed, 0xaa, 0xf1, 0x4d, 0xda, 0x8f, 0x2b, 0x5c, 0x03);

// DirectSound default capture device GUID {DEF00001-9C6D-47ED-AAF1-4DDA8F2B5C03}
DEFINE_GUID(DSDEVID_DefaultCapture, 0xdef00001, 0x9c6d, 0x47ed, 0xaa, 0xf1, 0x4d, 0xda, 0x8f, 0x2b, 0x5c, 0x03);

// DirectSound default device for voice playback {DEF00002-9C6D-47ED-AAF1-4DDA8F2B5C03}
DEFINE_GUID(DSDEVID_DefaultVoicePlayback, 0xdef00002, 0x9c6d, 0x47ed, 0xaa, 0xf1, 0x4d, 0xda, 0x8f, 0x2b, 0x5c, 0x03);

// DirectSound default device for voice capture {DEF00003-9C6D-47ED-AAF1-4DDA8F2B5C03}
DEFINE_GUID(DSDEVID_DefaultVoiceCapture, 0xdef00003, 0x9c6d, 0x47ed, 0xaa, 0xf1, 0x4d, 0xda, 0x8f, 0x2b, 0x5c, 0x03);


//
// Forward declarations for interfaces.
// 'struct' not 'class' per the way DECLARE_INTERFACE_ is defined
//

#ifdef __cplusplus
struct IDirectSound;
struct IDirectSoundBuffer;
struct IDirectSound3DListener;
struct IDirectSound3DBuffer;
struct IDirectSoundCapture;
struct IDirectSoundCaptureBuffer;
struct IDirectSoundNotify;
#endif // __cplusplus


//
// DirectSound 8.0 interfaces.
//

#if DIRECTSOUND_VERSION >= 0x0800

#ifdef __cplusplus
struct IDirectSound8;
struct IDirectSoundBuffer8;
struct IDirectSoundCaptureBuffer8;
struct IDirectSoundFXGargle;
struct IDirectSoundFXChorus;
struct IDirectSoundFXFlanger;
struct IDirectSoundFXEcho;
struct IDirectSoundFXDistortion;
struct IDirectSoundFXCompressor;
struct IDirectSoundFXParamEq;
struct IDirectSoundFXWavesReverb;
struct IDirectSoundFXI3DL2Reverb;
struct IDirectSoundCaptureFXAec;
struct IDirectSoundCaptureFXNoiseSuppress;
struct IDirectSoundFullDuplex;
#endif // __cplusplus

// IDirectSound8, IDirectSoundBuffer8 and IDirectSoundCaptureBuffer8 are the
// only DirectSound 7.0 interfaces with changed functionality in version 8.0.
// The other level 8 interfaces as equivalent to their level 7 counterparts:

#define IDirectSoundCapture8            IDirectSoundCapture
#define IDirectSound3DListener8         IDirectSound3DListener
#define IDirectSound3DBuffer8           IDirectSound3DBuffer
#define IDirectSoundNotify8             IDirectSoundNotify
#define IDirectSoundFXGargle8           IDirectSoundFXGargle
#define IDirectSoundFXChorus8           IDirectSoundFXChorus
#define IDirectSoundFXFlanger8          IDirectSoundFXFlanger
#define IDirectSoundFXEcho8             IDirectSoundFXEcho
#define IDirectSoundFXDistortion8       IDirectSoundFXDistortion
#define IDirectSoundFXCompressor8       IDirectSoundFXCompressor
#define IDirectSoundFXParamEq8          IDirectSoundFXParamEq
#define IDirectSoundFXWavesReverb8      IDirectSoundFXWavesReverb
#define IDirectSoundFXI3DL2Reverb8      IDirectSoundFXI3DL2Reverb
#define IDirectSoundCaptureFXAec8       IDirectSoundCaptureFXAec
#define IDirectSoundCaptureFXNoiseSuppress8 IDirectSoundCaptureFXNoiseSuppress
#define IDirectSoundFullDuplex8         IDirectSoundFullDuplex

#endif // DIRECTSOUND_VERSION >= 0x0800


typedef struct IDirectSound                 *LPDIRECTSOUND;
typedef struct IDirectSoundBuffer           *LPDIRECTSOUNDBUFFER;
typedef struct IDirectSound3DListener       *LPDIRECTSOUND3DLISTENER;
typedef struct IDirectSound3DBuffer         *LPDIRECTSOUND3DBUFFER;
typedef struct IDirectSoundCapture          *LPDIRECTSOUNDCAPTURE;
typedef struct IDirectSoundCaptureBuffer    *LPDIRECTSOUNDCAPTUREBUFFER;
typedef struct IDirectSoundNotify           *LPDIRECTSOUNDNOTIFY;


#if DIRECTSOUND_VERSION >= 0x0800
typedef struct IDirectSoundFXGargle         *LPDIRECTSOUNDFXGARGLE;
typedef struct IDirectSoundFXChorus         *LPDIRECTSOUNDFXCHORUS;
typedef struct IDirectSoundFXFlanger        *LPDIRECTSOUNDFXFLANGER;
typedef struct IDirectSoundFXEcho           *LPDIRECTSOUNDFXECHO;
typedef struct IDirectSoundFXDistortion     *LPDIRECTSOUNDFXDISTORTION;
typedef struct IDirectSoundFXCompressor     *LPDIRECTSOUNDFXCOMPRESSOR;
typedef struct IDirectSoundFXParamEq        *LPDIRECTSOUNDFXPARAMEQ;
typedef struct IDirectSoundFXWavesReverb    *LPDIRECTSOUNDFXWAVESREVERB;
typedef struct IDirectSoundFXI3DL2Reverb    *LPDIRECTSOUNDFXI3DL2REVERB;
typedef struct IDirectSoundCaptureFXAec     *LPDIRECTSOUNDCAPTUREFXAEC;
typedef struct IDirectSoundCaptureFXNoiseSuppress *LPDIRECTSOUNDCAPTUREFXNOISESUPPRESS;
typedef struct IDirectSoundFullDuplex       *LPDIRECTSOUNDFULLDUPLEX;

typedef struct IDirectSound8                *LPDIRECTSOUND8;
typedef struct IDirectSoundBuffer8          *LPDIRECTSOUNDBUFFER8;
typedef struct IDirectSound3DListener8      *LPDIRECTSOUND3DLISTENER8;
typedef struct IDirectSound3DBuffer8        *LPDIRECTSOUND3DBUFFER8;
typedef struct IDirectSoundCapture8         *LPDIRECTSOUNDCAPTURE8;
typedef struct IDirectSoundCaptureBuffer8   *LPDIRECTSOUNDCAPTUREBUFFER8;
typedef struct IDirectSoundNotify8          *LPDIRECTSOUNDNOTIFY8;
typedef struct IDirectSoundFXGargle8        *LPDIRECTSOUNDFXGARGLE8;
typedef struct IDirectSoundFXChorus8        *LPDIRECTSOUNDFXCHORUS8;
typedef struct IDirectSoundFXFlanger8       *LPDIRECTSOUNDFXFLANGER8;
typedef struct IDirectSoundFXEcho8          *LPDIRECTSOUNDFXECHO8;
typedef struct IDirectSoundFXDistortion8    *LPDIRECTSOUNDFXDISTORTION8;
typedef struct IDirectSoundFXCompressor8    *LPDIRECTSOUNDFXCOMPRESSOR8;
typedef struct IDirectSoundFXParamEq8       *LPDIRECTSOUNDFXPARAMEQ8;
typedef struct IDirectSoundFXWavesReverb8   *LPDIRECTSOUNDFXWAVESREVERB8;
typedef struct IDirectSoundFXI3DL2Reverb8   *LPDIRECTSOUNDFXI3DL2REVERB8;
typedef struct IDirectSoundCaptureFXAec8    *LPDIRECTSOUNDCAPTUREFXAEC8;
typedef struct IDirectSoundCaptureFXNoiseSuppress8 *LPDIRECTSOUNDCAPTUREFXNOISESUPPRESS8;
typedef struct IDirectSoundFullDuplex8      *LPDIRECTSOUNDFULLDUPLEX8;

#endif // DIRECTSOUND_VERSION >= 0x0800

//
// IID definitions for the unchanged DirectSound 8.0 interfaces
//

#if DIRECTSOUND_VERSION >= 0x0800
#define IID_IDirectSoundCapture8            IID_IDirectSoundCapture
#define IID_IDirectSound3DListener8         IID_IDirectSound3DListener
#define IID_IDirectSound3DBuffer8           IID_IDirectSound3DBuffer
#define IID_IDirectSoundNotify8             IID_IDirectSoundNotify
#define IID_IDirectSoundFXGargle8           IID_IDirectSoundFXGargle
#define IID_IDirectSoundFXChorus8           IID_IDirectSoundFXChorus
#define IID_IDirectSoundFXFlanger8          IID_IDirectSoundFXFlanger
#define IID_IDirectSoundFXEcho8             IID_IDirectSoundFXEcho
#define IID_IDirectSoundFXDistortion8       IID_IDirectSoundFXDistortion
#define IID_IDirectSoundFXCompressor8       IID_IDirectSoundFXCompressor
#define IID_IDirectSoundFXParamEq8          IID_IDirectSoundFXParamEq
#define IID_IDirectSoundFXWavesReverb8      IID_IDirectSoundFXWavesReverb
#define IID_IDirectSoundFXI3DL2Reverb8      IID_IDirectSoundFXI3DL2Reverb
#define IID_IDirectSoundCaptureFXAec8       IID_IDirectSoundCaptureFXAec
#define IID_IDirectSoundCaptureFXNoiseSuppress8 IID_IDirectSoundCaptureFXNoiseSuppress
#define IID_IDirectSoundFullDuplex8         IID_IDirectSoundFullDuplex
#endif // DIRECTSOUND_VERSION >= 0x0800

//
// Compatibility typedefs
//

#ifndef _LPCWAVEFORMATEX_DEFINED
#define _LPCWAVEFORMATEX_DEFINED
typedef const WAVEFORMATEX *LPCWAVEFORMATEX;
#endif // _LPCWAVEFORMATEX_DEFINED

#ifndef __LPCGUID_DEFINED__
#define __LPCGUID_DEFINED__
typedef const GUID *LPCGUID;
#endif // __LPCGUID_DEFINED__

typedef LPDIRECTSOUND *LPLPDIRECTSOUND;
typedef LPDIRECTSOUNDBUFFER *LPLPDIRECTSOUNDBUFFER;
typedef LPDIRECTSOUND3DLISTENER *LPLPDIRECTSOUND3DLISTENER;
typedef LPDIRECTSOUND3DBUFFER *LPLPDIRECTSOUND3DBUFFER;
typedef LPDIRECTSOUNDCAPTURE *LPLPDIRECTSOUNDCAPTURE;
typedef LPDIRECTSOUNDCAPTUREBUFFER *LPLPDIRECTSOUNDCAPTUREBUFFER;
typedef LPDIRECTSOUNDNOTIFY *LPLPDIRECTSOUNDNOTIFY;

#if DIRECTSOUND_VERSION >= 0x0800
typedef LPDIRECTSOUND8 *LPLPDIRECTSOUND8;
typedef LPDIRECTSOUNDBUFFER8 *LPLPDIRECTSOUNDBUFFER8;
typedef LPDIRECTSOUNDCAPTURE8 *LPLPDIRECTSOUNDCAPTURE8;
typedef LPDIRECTSOUNDCAPTUREBUFFER8 *LPLPDIRECTSOUNDCAPTUREBUFFER8;
#endif // DIRECTSOUND_VERSION >= 0x0800

//
// Structures
//

typedef struct _DSCAPS
{
    DWORD           dwSize;
    DWORD           dwFlags;
    DWORD           dwMinSecondarySampleRate;
    DWORD           dwMaxSecondarySampleRate;
    DWORD           dwPrimaryBuffers;
    DWORD           dwMaxHwMixingAllBuffers;
    DWORD           dwMaxHwMixingStaticBuffers;
    DWORD           dwMaxHwMixingStreamingBuffers;
    DWORD           dwFreeHwMixingAllBuffers;
    DWORD           dwFreeHwMixingStaticBuffers;
    DWORD           dwFreeHwMixingStreamingBuffers;
    DWORD           dwMaxHw3DAllBuffers;
    DWORD           dwMaxHw3DStaticBuffers;
    DWORD           dwMaxHw3DStreamingBuffers;
    DWORD           dwFreeHw3DAllBuffers;
    DWORD           dwFreeHw3DStaticBuffers;
    DWORD           dwFreeHw3DStreamingBuffers;
    DWORD           dwTotalHwMemBytes;
    DWORD           dwFreeHwMemBytes;
    DWORD           dwMaxContigFreeHwMemBytes;
    DWORD           dwUnlockTransferRateHwBuffers;
    DWORD           dwPlayCpuOverheadSwBuffers;
    DWORD           dwReserved1;
    DWORD           dwReserved2;
} DSCAPS, *LPDSCAPS;

typedef const DSCAPS *LPCDSCAPS;

typedef struct _DSBCAPS
{
    DWORD           dwSize;
    DWORD           dwFlags;
    DWORD           dwBufferBytes;
    DWORD           dwUnlockTransferRate;
    DWORD           dwPlayCpuOverhead;
} DSBCAPS, *LPDSBCAPS;

typedef const DSBCAPS *LPCDSBCAPS;

#if DIRECTSOUND_VERSION >= 0x0800

    typedef struct _DSEFFECTDESC
    {
        DWORD       dwSize;
        DWORD       dwFlags;
        GUID        guidDSFXClass;
        DWORD_PTR   dwReserved1;
        DWORD_PTR   dwReserved2;
    } DSEFFECTDESC, *LPDSEFFECTDESC;
    typedef const DSEFFECTDESC *LPCDSEFFECTDESC;

    #define DSFX_LOCHARDWARE    0x00000001
    #define DSFX_LOCSOFTWARE    0x00000002

    enum
    {
        DSFXR_PRESENT,          // 0
        DSFXR_LOCHARDWARE,      // 1
        DSFXR_LOCSOFTWARE,      // 2
        DSFXR_UNALLOCATED,      // 3
        DSFXR_FAILED,           // 4
        DSFXR_UNKNOWN,          // 5
        DSFXR_SENDLOOP          // 6
    };

    typedef struct _DSCEFFECTDESC
    {
        DWORD       dwSize;
        DWORD       dwFlags;
        GUID        guidDSCFXClass;
        GUID        guidDSCFXInstance;
        DWORD       dwReserved1;
        DWORD       dwReserved2;
    } DSCEFFECTDESC, *LPDSCEFFECTDESC;
    typedef const DSCEFFECTDESC *LPCDSCEFFECTDESC;

    #define DSCFX_LOCHARDWARE   0x00000001
    #define DSCFX_LOCSOFTWARE   0x00000002

    #define DSCFXR_LOCHARDWARE  0x00000010
    #define DSCFXR_LOCSOFTWARE  0x00000020

#endif // DIRECTSOUND_VERSION >= 0x0800

typedef struct _DSBUFFERDESC
{
    DWORD           dwSize;
    DWORD           dwFlags;
    DWORD           dwBufferBytes;
    DWORD           dwReserved;
    LPWAVEFORMATEX  lpwfxFormat;
#if DIRECTSOUND_VERSION >= 0x0700
    GUID            guid3DAlgorithm;
#endif
} DSBUFFERDESC, *LPDSBUFFERDESC;

typedef const DSBUFFERDESC *LPCDSBUFFERDESC;

// Older version of this structure:

typedef struct _DSBUFFERDESC1
{
    DWORD           dwSize;
    DWORD           dwFlags;
    DWORD           dwBufferBytes;
    DWORD           dwReserved;
    LPWAVEFORMATEX  lpwfxFormat;
} DSBUFFERDESC1, *LPDSBUFFERDESC1;

typedef const DSBUFFERDESC1 *LPCDSBUFFERDESC1;

typedef struct _DS3DBUFFER
{
    DWORD           dwSize;
    D3DVECTOR       vPosition;
    D3DVECTOR       vVelocity;
    DWORD           dwInsideConeAngle;
    DWORD           dwOutsideConeAngle;
    D3DVECTOR       vConeOrientation;
    LONG            lConeOutsideVolume;
    D3DVALUE        flMinDistance;
    D3DVALUE        flMaxDistance;
    DWORD           dwMode;
} DS3DBUFFER, *LPDS3DBUFFER;

typedef const DS3DBUFFER *LPCDS3DBUFFER;

typedef struct _DS3DLISTENER
{
    DWORD           dwSize;
    D3DVECTOR       vPosition;
    D3DVECTOR       vVelocity;
    D3DVECTOR       vOrientFront;
    D3DVECTOR       vOrientTop;
    D3DVALUE        flDistanceFactor;
    D3DVALUE        flRolloffFactor;
    D3DVALUE        flDopplerFactor;
} DS3DLISTENER, *LPDS3DLISTENER;

typedef const DS3DLISTENER *LPCDS3DLISTENER;

typedef struct _DSCCAPS
{
    DWORD           dwSize;
    DWORD           dwFlags;
    DWORD           dwFormats;
    DWORD           dwChannels;
} DSCCAPS, *LPDSCCAPS;

typedef const DSCCAPS *LPCDSCCAPS;

typedef struct _DSCBUFFERDESC1
{
    DWORD           dwSize;
    DWORD           dwFlags;
    DWORD           dwBufferBytes;
    DWORD           dwReserved;
    LPWAVEFORMATEX  lpwfxFormat;
} DSCBUFFERDESC1, *LPDSCBUFFERDESC1;

typedef struct _DSCBUFFERDESC
{
    DWORD           dwSize;
    DWORD           dwFlags;
    DWORD           dwBufferBytes;
    DWORD           dwReserved;
    LPWAVEFORMATEX  lpwfxFormat;
#if DIRECTSOUND_VERSION >= 0x0800
    DWORD           dwFXCount;
    LPDSCEFFECTDESC lpDSCFXDesc;
#endif
} DSCBUFFERDESC, *LPDSCBUFFERDESC;

typedef const DSCBUFFERDESC *LPCDSCBUFFERDESC;

typedef struct _DSCBCAPS
{
    DWORD           dwSize;
    DWORD           dwFlags;
    DWORD           dwBufferBytes;
    DWORD           dwReserved;
} DSCBCAPS, *LPDSCBCAPS;

typedef const DSCBCAPS *LPCDSCBCAPS;

typedef struct _DSBPOSITIONNOTIFY
{
    DWORD           dwOffset;
    HANDLE          hEventNotify;
} DSBPOSITIONNOTIFY, *LPDSBPOSITIONNOTIFY;

typedef const DSBPOSITIONNOTIFY *LPCDSBPOSITIONNOTIFY;

//
// DirectSound API
//

typedef BOOL (CALLBACK *LPDSENUMCALLBACKA)(LPGUID, LPCSTR, LPCSTR, LPVOID);
typedef BOOL (CALLBACK *LPDSENUMCALLBACKW)(LPGUID, LPCWSTR, LPCWSTR, LPVOID);

extern HRESULT WINAPI DirectSoundCreate(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DirectSoundEnumerateA(LPDSENUMCALLBACKA pDSEnumCallback, LPVOID pContext);
extern HRESULT WINAPI DirectSoundEnumerateW(LPDSENUMCALLBACKW pDSEnumCallback, LPVOID pContext);

extern HRESULT WINAPI DirectSoundCaptureCreate(LPCGUID pcGuidDevice, LPDIRECTSOUNDCAPTURE *ppDSC, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DirectSoundCaptureEnumerateA(LPDSENUMCALLBACKA pDSEnumCallback, LPVOID pContext);
extern HRESULT WINAPI DirectSoundCaptureEnumerateW(LPDSENUMCALLBACKW pDSEnumCallback, LPVOID pContext);

#if DIRECTSOUND_VERSION >= 0x0800
extern HRESULT WINAPI DirectSoundCreate8(LPCGUID pcGuidDevice, LPDIRECTSOUND8 *ppDS8, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DirectSoundCaptureCreate8(LPCGUID pcGuidDevice, LPDIRECTSOUNDCAPTURE8 *ppDSC8, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DirectSoundFullDuplexCreate(LPCGUID pcGuidCaptureDevice, LPCGUID pcGuidRenderDevice,
        LPCDSCBUFFERDESC pcDSCBufferDesc, LPCDSBUFFERDESC pcDSBufferDesc, HWND hWnd,
        DWORD dwLevel, LPDIRECTSOUNDFULLDUPLEX* ppDSFD, LPDIRECTSOUNDCAPTUREBUFFER8 *ppDSCBuffer8,
        LPDIRECTSOUNDBUFFER8 *ppDSBuffer8, LPUNKNOWN pUnkOuter);
#define DirectSoundFullDuplexCreate8 DirectSoundFullDuplexCreate

extern HRESULT WINAPI GetDeviceID(LPCGUID pGuidSrc, LPGUID pGuidDest);
#endif // DIRECTSOUND_VERSION >= 0x0800

#ifdef UNICODE
#define LPDSENUMCALLBACK            LPDSENUMCALLBACKW
#define DirectSoundEnumerate        DirectSoundEnumerateW
#define DirectSoundCaptureEnumerate DirectSoundCaptureEnumerateW
#else // UNICODE
#define LPDSENUMCALLBACK            LPDSENUMCALLBACKA
#define DirectSoundEnumerate        DirectSoundEnumerateA
#define DirectSoundCaptureEnumerate DirectSoundCaptureEnumerateA
#endif // UNICODE

//
// IUnknown
//

#if !defined(__cplusplus) || defined(CINTERFACE)
#ifndef IUnknown_QueryInterface
#define IUnknown_QueryInterface(p,a,b)  (p)->lpVtbl->QueryInterface(p,a,b)
#endif // IUnknown_QueryInterface
#ifndef IUnknown_AddRef
#define IUnknown_AddRef(p)              (p)->lpVtbl->AddRef(p)
#endif // IUnknown_AddRef
#ifndef IUnknown_Release
#define IUnknown_Release(p)             (p)->lpVtbl->Release(p)
#endif // IUnknown_Release
#else // !defined(__cplusplus) || defined(CINTERFACE)
#ifndef IUnknown_QueryInterface
#define IUnknown_QueryInterface(p,a,b)  (p)->QueryInterface(a,b)
#endif // IUnknown_QueryInterface
#ifndef IUnknown_AddRef
#define IUnknown_AddRef(p)              (p)->AddRef()
#endif // IUnknown_AddRef
#ifndef IUnknown_Release
#define IUnknown_Release(p)             (p)->Release()
#endif // IUnknown_Release
#endif // !defined(__cplusplus) || defined(CINTERFACE)

#ifndef __IReferenceClock_INTERFACE_DEFINED__
#define __IReferenceClock_INTERFACE_DEFINED__

typedef LONGLONG REFERENCE_TIME;
typedef REFERENCE_TIME *LPREFERENCE_TIME;

DEFINE_GUID(IID_IReferenceClock, 0x56a86897, 0x0ad4, 0x11ce, 0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);

#undef INTERFACE
#define INTERFACE IReferenceClock

DECLARE_INTERFACE_(IReferenceClock, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // IReferenceClock methods
    STDMETHOD(GetTime)              (THIS_ REFERENCE_TIME *pTime) PURE;
    STDMETHOD(AdviseTime)           (THIS_ REFERENCE_TIME rtBaseTime, REFERENCE_TIME rtStreamTime,
                                           HANDLE hEvent, LPDWORD pdwAdviseCookie) PURE;
    STDMETHOD(AdvisePeriodic)       (THIS_ REFERENCE_TIME rtStartTime, REFERENCE_TIME rtPeriodTime,
                                           HANDLE hSemaphore, LPDWORD pdwAdviseCookie) PURE;
    STDMETHOD(Unadvise)             (THIS_ DWORD dwAdviseCookie) PURE;
};

#endif // __IReferenceClock_INTERFACE_DEFINED__

#ifndef IReferenceClock_QueryInterface

#define IReferenceClock_QueryInterface(p,a,b)      IUnknown_QueryInterface(p,a,b)
#define IReferenceClock_AddRef(p)                  IUnknown_AddRef(p)
#define IReferenceClock_Release(p)                 IUnknown_Release(p)

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IReferenceClock_GetTime(p,a)               (p)->lpVtbl->GetTime(p,a)
#define IReferenceClock_AdviseTime(p,a,b,c,d)      (p)->lpVtbl->AdviseTime(p,a,b,c,d)
#define IReferenceClock_AdvisePeriodic(p,a,b,c,d)  (p)->lpVtbl->AdvisePeriodic(p,a,b,c,d)
#define IReferenceClock_Unadvise(p,a)              (p)->lpVtbl->Unadvise(p,a)
#else // !defined(__cplusplus) || defined(CINTERFACE)
#define IReferenceClock_GetTime(p,a)               (p)->GetTime(a)
#define IReferenceClock_AdviseTime(p,a,b,c,d)      (p)->AdviseTime(a,b,c,d)
#define IReferenceClock_AdvisePeriodic(p,a,b,c,d)  (p)->AdvisePeriodic(a,b,c,d)
#define IReferenceClock_Unadvise(p,a)              (p)->Unadvise(a)
#endif // !defined(__cplusplus) || defined(CINTERFACE)

#endif // IReferenceClock_QueryInterface

//
// IDirectSound
//

DEFINE_GUID(IID_IDirectSound, 0x279AFA83, 0x4981, 0x11CE, 0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60);

#undef INTERFACE
#define INTERFACE IDirectSound

DECLARE_INTERFACE_(IDirectSound, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // IDirectSound methods
    STDMETHOD(CreateSoundBuffer)    (THIS_ LPCDSBUFFERDESC pcDSBufferDesc, LPDIRECTSOUNDBUFFER *ppDSBuffer, LPUNKNOWN pUnkOuter) PURE;
    STDMETHOD(GetCaps)              (THIS_ LPDSCAPS pDSCaps) PURE;
    STDMETHOD(DuplicateSoundBuffer) (THIS_ LPDIRECTSOUNDBUFFER pDSBufferOriginal, LPDIRECTSOUNDBUFFER *ppDSBufferDuplicate) PURE;
    STDMETHOD(SetCooperativeLevel)  (THIS_ HWND hwnd, DWORD dwLevel) PURE;
    STDMETHOD(Compact)              (THIS) PURE;
    STDMETHOD(GetSpeakerConfig)     (THIS_ LPDWORD pdwSpeakerConfig) PURE;
    STDMETHOD(SetSpeakerConfig)     (THIS_ DWORD dwSpeakerConfig) PURE;
    STDMETHOD(Initialize)           (THIS_ LPCGUID pcGuidDevice) PURE;
};

#define IDirectSound_QueryInterface(p,a,b)       IUnknown_QueryInterface(p,a,b)
#define IDirectSound_AddRef(p)                   IUnknown_AddRef(p)
#define IDirectSound_Release(p)                  IUnknown_Release(p)

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSound_CreateSoundBuffer(p,a,b,c)  (p)->lpVtbl->CreateSoundBuffer(p,a,b,c)
#define IDirectSound_GetCaps(p,a)                (p)->lpVtbl->GetCaps(p,a)
#define IDirectSound_DuplicateSoundBuffer(p,a,b) (p)->lpVtbl->DuplicateSoundBuffer(p,a,b)
#define IDirectSound_SetCooperativeLevel(p,a,b)  (p)->lpVtbl->SetCooperativeLevel(p,a,b)
#define IDirectSound_Compact(p)                  (p)->lpVtbl->Compact(p)
#define IDirectSound_GetSpeakerConfig(p,a)       (p)->lpVtbl->GetSpeakerConfig(p,a)
#define IDirectSound_SetSpeakerConfig(p,b)       (p)->lpVtbl->SetSpeakerConfig(p,b)
#define IDirectSound_Initialize(p,a)             (p)->lpVtbl->Initialize(p,a)
#else // !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSound_CreateSoundBuffer(p,a,b,c)  (p)->CreateSoundBuffer(a,b,c)
#define IDirectSound_GetCaps(p,a)                (p)->GetCaps(a)
#define IDirectSound_DuplicateSoundBuffer(p,a,b) (p)->DuplicateSoundBuffer(a,b)
#define IDirectSound_SetCooperativeLevel(p,a,b)  (p)->SetCooperativeLevel(a,b)
#define IDirectSound_Compact(p)                  (p)->Compact()
#define IDirectSound_GetSpeakerConfig(p,a)       (p)->GetSpeakerConfig(a)
#define IDirectSound_SetSpeakerConfig(p,b)       (p)->SetSpeakerConfig(b)
#define IDirectSound_Initialize(p,a)             (p)->Initialize(a)
#endif // !defined(__cplusplus) || defined(CINTERFACE)

#if DIRECTSOUND_VERSION >= 0x0800

//
// IDirectSound8
//

DEFINE_GUID(IID_IDirectSound8, 0xC50A7E93, 0xF395, 0x4834, 0x9E, 0xF6, 0x7F, 0xA9, 0x9D, 0xE5, 0x09, 0x66);

#undef INTERFACE
#define INTERFACE IDirectSound8

DECLARE_INTERFACE_(IDirectSound8, IDirectSound)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // IDirectSound methods
    STDMETHOD(CreateSoundBuffer)    (THIS_ LPCDSBUFFERDESC pcDSBufferDesc, LPDIRECTSOUNDBUFFER *ppDSBuffer, LPUNKNOWN pUnkOuter) PURE;
    STDMETHOD(GetCaps)              (THIS_ LPDSCAPS pDSCaps) PURE;
    STDMETHOD(DuplicateSoundBuffer) (THIS_ LPDIRECTSOUNDBUFFER pDSBufferOriginal, LPDIRECTSOUNDBUFFER *ppDSBufferDuplicate) PURE;
    STDMETHOD(SetCooperativeLevel)  (THIS_ HWND hwnd, DWORD dwLevel) PURE;
    STDMETHOD(Compact)              (THIS) PURE;
    STDMETHOD(GetSpeakerConfig)     (THIS_ LPDWORD pdwSpeakerConfig) PURE;
    STDMETHOD(SetSpeakerConfig)     (THIS_ DWORD dwSpeakerConfig) PURE;
    STDMETHOD(Initialize)           (THIS_ LPCGUID pcGuidDevice) PURE;

    // IDirectSound8 methods
    STDMETHOD(VerifyCertification)  (THIS_ LPDWORD pdwCertified) PURE;
};

#define IDirectSound8_QueryInterface(p,a,b)       IDirectSound_QueryInterface(p,a,b)
#define IDirectSound8_AddRef(p)                   IDirectSound_AddRef(p)
#define IDirectSound8_Release(p)                  IDirectSound_Release(p)
#define IDirectSound8_CreateSoundBuffer(p,a,b,c)  IDirectSound_CreateSoundBuffer(p,a,b,c)
#define IDirectSound8_GetCaps(p,a)                IDirectSound_GetCaps(p,a)
#define IDirectSound8_DuplicateSoundBuffer(p,a,b) IDirectSound_DuplicateSoundBuffer(p,a,b)
#define IDirectSound8_SetCooperativeLevel(p,a,b)  IDirectSound_SetCooperativeLevel(p,a,b)
#define IDirectSound8_Compact(p)                  IDirectSound_Compact(p)
#define IDirectSound8_GetSpeakerConfig(p,a)       IDirectSound_GetSpeakerConfig(p,a)
#define IDirectSound8_SetSpeakerConfig(p,a)       IDirectSound_SetSpeakerConfig(p,a)
#define IDirectSound8_Initialize(p,a)             IDirectSound_Initialize(p,a)

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSound8_VerifyCertification(p,a)           (p)->lpVtbl->VerifyCertification(p,a)
#else // !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSound8_VerifyCertification(p,a)           (p)->VerifyCertification(a)
#endif // !defined(__cplusplus) || defined(CINTERFACE)

#endif // DIRECTSOUND_VERSION >= 0x0800

//
// IDirectSoundBuffer
//

DEFINE_GUID(IID_IDirectSoundBuffer, 0x279AFA85, 0x4981, 0x11CE, 0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60);

#undef INTERFACE
#define INTERFACE IDirectSoundBuffer

DECLARE_INTERFACE_(IDirectSoundBuffer, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // IDirectSoundBuffer methods
    STDMETHOD(GetCaps)              (THIS_ LPDSBCAPS pDSBufferCaps) PURE;
    STDMETHOD(GetCurrentPosition)   (THIS_ LPDWORD pdwCurrentPlayCursor, LPDWORD pdwCurrentWriteCursor) PURE;
    STDMETHOD(GetFormat)            (THIS_ LPWAVEFORMATEX pwfxFormat, DWORD dwSizeAllocated, LPDWORD pdwSizeWritten) PURE;
    STDMETHOD(GetVolume)            (THIS_ LPLONG plVolume) PURE;
    STDMETHOD(GetPan)               (THIS_ LPLONG plPan) PURE;
    STDMETHOD(GetFrequency)         (THIS_ LPDWORD pdwFrequency) PURE;
    STDMETHOD(GetStatus)            (THIS_ LPDWORD pdwStatus) PURE;
    STDMETHOD(Initialize)           (THIS_ LPDIRECTSOUND pDirectSound, LPCDSBUFFERDESC pcDSBufferDesc) PURE;
    STDMETHOD(Lock)                 (THIS_ DWORD dwOffset, DWORD dwBytes, LPVOID *ppvAudioPtr1, LPDWORD pdwAudioBytes1,
                                           LPVOID *ppvAudioPtr2, LPDWORD pdwAudioBytes2, DWORD dwFlags) PURE;
    STDMETHOD(Play)                 (THIS_ DWORD dwReserved1, DWORD dwPriority, DWORD dwFlags) PURE;
    STDMETHOD(SetCurrentPosition)   (THIS_ DWORD dwNewPosition) PURE;
    STDMETHOD(SetFormat)            (THIS_ LPCWAVEFORMATEX pcfxFormat) PURE;
    STDMETHOD(SetVolume)            (THIS_ LONG lVolume) PURE;
    STDMETHOD(SetPan)               (THIS_ LONG lPan) PURE;
    STDMETHOD(SetFrequency)         (THIS_ DWORD dwFrequency) PURE;
    STDMETHOD(Stop)                 (THIS) PURE;
    STDMETHOD(Unlock)               (THIS_ LPVOID pvAudioPtr1, DWORD dwAudioBytes1, LPVOID pvAudioPtr2, DWORD dwAudioBytes2) PURE;
    STDMETHOD(Restore)              (THIS) PURE;
};

#define IDirectSoundBuffer_QueryInterface(p,a,b)        IUnknown_QueryInterface(p,a,b)
#define IDirectSoundBuffer_AddRef(p)                    IUnknown_AddRef(p)
#define IDirectSoundBuffer_Release(p)                   IUnknown_Release(p)

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundBuffer_GetCaps(p,a)                 (p)->lpVtbl->GetCaps(p,a)
#define IDirectSoundBuffer_GetCurrentPosition(p,a,b)    (p)->lpVtbl->GetCurrentPosition(p,a,b)
#define IDirectSoundBuffer_GetFormat(p,a,b,c)           (p)->lpVtbl->GetFormat(p,a,b,c)
#define IDirectSoundBuffer_GetVolume(p,a)               (p)->lpVtbl->GetVolume(p,a)
#define IDirectSoundBuffer_GetPan(p,a)                  (p)->lpVtbl->GetPan(p,a)
#define IDirectSoundBuffer_GetFrequency(p,a)            (p)->lpVtbl->GetFrequency(p,a)
#define IDirectSoundBuffer_GetStatus(p,a)               (p)->lpVtbl->GetStatus(p,a)
#define IDirectSoundBuffer_Initialize(p,a,b)            (p)->lpVtbl->Initialize(p,a,b)
#define IDirectSoundBuffer_Lock(p,a,b,c,d,e,f,g)        (p)->lpVtbl->Lock(p,a,b,c,d,e,f,g)
#define IDirectSoundBuffer_Play(p,a,b,c)                (p)->lpVtbl->Play(p,a,b,c)
#define IDirectSoundBuffer_SetCurrentPosition(p,a)      (p)->lpVtbl->SetCurrentPosition(p,a)
#define IDirectSoundBuffer_SetFormat(p,a)               (p)->lpVtbl->SetFormat(p,a)
#define IDirectSoundBuffer_SetVolume(p,a)               (p)->lpVtbl->SetVolume(p,a)
#define IDirectSoundBuffer_SetPan(p,a)                  (p)->lpVtbl->SetPan(p,a)
#define IDirectSoundBuffer_SetFrequency(p,a)            (p)->lpVtbl->SetFrequency(p,a)
#define IDirectSoundBuffer_Stop(p)                      (p)->lpVtbl->Stop(p)
#define IDirectSoundBuffer_Unlock(p,a,b,c,d)            (p)->lpVtbl->Unlock(p,a,b,c,d)
#define IDirectSoundBuffer_Restore(p)                   (p)->lpVtbl->Restore(p)
#else // !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundBuffer_GetCaps(p,a)                 (p)->GetCaps(a)
#define IDirectSoundBuffer_GetCurrentPosition(p,a,b)    (p)->GetCurrentPosition(a,b)
#define IDirectSoundBuffer_GetFormat(p,a,b,c)           (p)->GetFormat(a,b,c)
#define IDirectSoundBuffer_GetVolume(p,a)               (p)->GetVolume(a)
#define IDirectSoundBuffer_GetPan(p,a)                  (p)->GetPan(a)
#define IDirectSoundBuffer_GetFrequency(p,a)            (p)->GetFrequency(a)
#define IDirectSoundBuffer_GetStatus(p,a)               (p)->GetStatus(a)
#define IDirectSoundBuffer_Initialize(p,a,b)            (p)->Initialize(a,b)
#define IDirectSoundBuffer_Lock(p,a,b,c,d,e,f,g)        (p)->Lock(a,b,c,d,e,f,g)
#define IDirectSoundBuffer_Play(p,a,b,c)                (p)->Play(a,b,c)
#define IDirectSoundBuffer_SetCurrentPosition(p,a)      (p)->SetCurrentPosition(a)
#define IDirectSoundBuffer_SetFormat(p,a)               (p)->SetFormat(a)
#define IDirectSoundBuffer_SetVolume(p,a)               (p)->SetVolume(a)
#define IDirectSoundBuffer_SetPan(p,a)                  (p)->SetPan(a)
#define IDirectSoundBuffer_SetFrequency(p,a)            (p)->SetFrequency(a)
#define IDirectSoundBuffer_Stop(p)                      (p)->Stop()
#define IDirectSoundBuffer_Unlock(p,a,b,c,d)            (p)->Unlock(a,b,c,d)
#define IDirectSoundBuffer_Restore(p)                   (p)->Restore()
#endif // !defined(__cplusplus) || defined(CINTERFACE)

#if DIRECTSOUND_VERSION >= 0x0800

//
// IDirectSoundBuffer8
//

DEFINE_GUID(IID_IDirectSoundBuffer8, 0x6825a449, 0x7524, 0x4d82, 0x92, 0x0f, 0x50, 0xe3, 0x6a, 0xb3, 0xab, 0x1e);

#undef INTERFACE
#define INTERFACE IDirectSoundBuffer8

DECLARE_INTERFACE_(IDirectSoundBuffer8, IDirectSoundBuffer)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // IDirectSoundBuffer methods
    STDMETHOD(GetCaps)              (THIS_ LPDSBCAPS pDSBufferCaps) PURE;
    STDMETHOD(GetCurrentPosition)   (THIS_ LPDWORD pdwCurrentPlayCursor, LPDWORD pdwCurrentWriteCursor) PURE;
    STDMETHOD(GetFormat)            (THIS_ LPWAVEFORMATEX pwfxFormat, DWORD dwSizeAllocated, LPDWORD pdwSizeWritten) PURE;
    STDMETHOD(GetVolume)            (THIS_ LPLONG plVolume) PURE;
    STDMETHOD(GetPan)               (THIS_ LPLONG plPan) PURE;
    STDMETHOD(GetFrequency)         (THIS_ LPDWORD pdwFrequency) PURE;
    STDMETHOD(GetStatus)            (THIS_ LPDWORD pdwStatus) PURE;
    STDMETHOD(Initialize)           (THIS_ LPDIRECTSOUND pDirectSound, LPCDSBUFFERDESC pcDSBufferDesc) PURE;
    STDMETHOD(Lock)                 (THIS_ DWORD dwOffset, DWORD dwBytes, LPVOID *ppvAudioPtr1, LPDWORD pdwAudioBytes1,
                                           LPVOID *ppvAudioPtr2, LPDWORD pdwAudioBytes2, DWORD dwFlags) PURE;
    STDMETHOD(Play)                 (THIS_ DWORD dwReserved1, DWORD dwPriority, DWORD dwFlags) PURE;
    STDMETHOD(SetCurrentPosition)   (THIS_ DWORD dwNewPosition) PURE;
    STDMETHOD(SetFormat)            (THIS_ LPCWAVEFORMATEX pcfxFormat) PURE;
    STDMETHOD(SetVolume)            (THIS_ LONG lVolume) PURE;
    STDMETHOD(SetPan)               (THIS_ LONG lPan) PURE;
    STDMETHOD(SetFrequency)         (THIS_ DWORD dwFrequency) PURE;
    STDMETHOD(Stop)                 (THIS) PURE;
    STDMETHOD(Unlock)               (THIS_ LPVOID pvAudioPtr1, DWORD dwAudioBytes1, LPVOID pvAudioPtr2, DWORD dwAudioBytes2) PURE;
    STDMETHOD(Restore)              (THIS) PURE;

    // IDirectSoundBuffer8 methods
    STDMETHOD(SetFX)                (THIS_ DWORD dwEffectsCount, LPDSEFFECTDESC pDSFXDesc, LPDWORD pdwResultCodes) PURE;
    STDMETHOD(AcquireResources)     (THIS_ DWORD dwFlags, DWORD dwEffectsCount, LPDWORD pdwResultCodes) PURE;
    STDMETHOD(GetObjectInPath)      (THIS_ REFGUID rguidObject, DWORD dwIndex, REFGUID rguidInterface, LPVOID *ppObject) PURE;
};

// Special GUID meaning "select all objects" for use in GetObjectInPath()
DEFINE_GUID(GUID_All_Objects, 0xaa114de5, 0xc262, 0x4169, 0xa1, 0xc8, 0x23, 0xd6, 0x98, 0xcc, 0x73, 0xb5);

#define IDirectSoundBuffer8_QueryInterface(p,a,b)           IUnknown_QueryInterface(p,a,b)
#define IDirectSoundBuffer8_AddRef(p)                       IUnknown_AddRef(p)
#define IDirectSoundBuffer8_Release(p)                      IUnknown_Release(p)

#define IDirectSoundBuffer8_GetCaps(p,a)                    IDirectSoundBuffer_GetCaps(p,a)
#define IDirectSoundBuffer8_GetCurrentPosition(p,a,b)       IDirectSoundBuffer_GetCurrentPosition(p,a,b)
#define IDirectSoundBuffer8_GetFormat(p,a,b,c)              IDirectSoundBuffer_GetFormat(p,a,b,c)
#define IDirectSoundBuffer8_GetVolume(p,a)                  IDirectSoundBuffer_GetVolume(p,a)
#define IDirectSoundBuffer8_GetPan(p,a)                     IDirectSoundBuffer_GetPan(p,a)
#define IDirectSoundBuffer8_GetFrequency(p,a)               IDirectSoundBuffer_GetFrequency(p,a)
#define IDirectSoundBuffer8_GetStatus(p,a)                  IDirectSoundBuffer_GetStatus(p,a)
#define IDirectSoundBuffer8_Initialize(p,a,b)               IDirectSoundBuffer_Initialize(p,a,b)
#define IDirectSoundBuffer8_Lock(p,a,b,c,d,e,f,g)           IDirectSoundBuffer_Lock(p,a,b,c,d,e,f,g)
#define IDirectSoundBuffer8_Play(p,a,b,c)                   IDirectSoundBuffer_Play(p,a,b,c)
#define IDirectSoundBuffer8_SetCurrentPosition(p,a)         IDirectSoundBuffer_SetCurrentPosition(p,a)
#define IDirectSoundBuffer8_SetFormat(p,a)                  IDirectSoundBuffer_SetFormat(p,a)
#define IDirectSoundBuffer8_SetVolume(p,a)                  IDirectSoundBuffer_SetVolume(p,a)
#define IDirectSoundBuffer8_SetPan(p,a)                     IDirectSoundBuffer_SetPan(p,a)
#define IDirectSoundBuffer8_SetFrequency(p,a)               IDirectSoundBuffer_SetFrequency(p,a)
#define IDirectSoundBuffer8_Stop(p)                         IDirectSoundBuffer_Stop(p)
#define IDirectSoundBuffer8_Unlock(p,a,b,c,d)               IDirectSoundBuffer_Unlock(p,a,b,c,d)
#define IDirectSoundBuffer8_Restore(p)                      IDirectSoundBuffer_Restore(p)

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundBuffer8_SetFX(p,a,b,c)                  (p)->lpVtbl->SetFX(p,a,b,c)
#define IDirectSoundBuffer8_AcquireResources(p,a,b,c)       (p)->lpVtbl->AcquireResources(p,a,b,c)
#define IDirectSoundBuffer8_GetObjectInPath(p,a,b,c,d)      (p)->lpVtbl->GetObjectInPath(p,a,b,c,d)
#else // !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundBuffer8_SetFX(p,a,b,c)                  (p)->SetFX(a,b,c)
#define IDirectSoundBuffer8_AcquireResources(p,a,b,c)       (p)->AcquireResources(a,b,c)
#define IDirectSoundBuffer8_GetObjectInPath(p,a,b,c,d)      (p)->GetObjectInPath(a,b,c,d)
#endif // !defined(__cplusplus) || defined(CINTERFACE)

#endif // DIRECTSOUND_VERSION >= 0x0800

//
// IDirectSound3DListener
//

DEFINE_GUID(IID_IDirectSound3DListener, 0x279AFA84, 0x4981, 0x11CE, 0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60);

#undef INTERFACE
#define INTERFACE IDirectSound3DListener

DECLARE_INTERFACE_(IDirectSound3DListener, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)           (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)            (THIS) PURE;
    STDMETHOD_(ULONG,Release)           (THIS) PURE;

    // IDirectSound3DListener methods
    STDMETHOD(GetAllParameters)         (THIS_ LPDS3DLISTENER pListener) PURE;
    STDMETHOD(GetDistanceFactor)        (THIS_ D3DVALUE* pflDistanceFactor) PURE;
    STDMETHOD(GetDopplerFactor)         (THIS_ D3DVALUE* pflDopplerFactor) PURE;
    STDMETHOD(GetOrientation)           (THIS_ D3DVECTOR* pvOrientFront, D3DVECTOR* pvOrientTop) PURE;
    STDMETHOD(GetPosition)              (THIS_ D3DVECTOR* pvPosition) PURE;
    STDMETHOD(GetRolloffFactor)         (THIS_ D3DVALUE* pflRolloffFactor) PURE;
    STDMETHOD(GetVelocity)              (THIS_ D3DVECTOR* pvVelocity) PURE;
    STDMETHOD(SetAllParameters)         (THIS_ LPCDS3DLISTENER pcListener, DWORD dwApply) PURE;
    STDMETHOD(SetDistanceFactor)        (THIS_ D3DVALUE flDistanceFactor, DWORD dwApply) PURE;
    STDMETHOD(SetDopplerFactor)         (THIS_ D3DVALUE flDopplerFactor, DWORD dwApply) PURE;
    STDMETHOD(SetOrientation)           (THIS_ D3DVALUE xFront, D3DVALUE yFront, D3DVALUE zFront,
                                               D3DVALUE xTop, D3DVALUE yTop, D3DVALUE zTop, DWORD dwApply) PURE;
    STDMETHOD(SetPosition)              (THIS_ D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply) PURE;
    STDMETHOD(SetRolloffFactor)         (THIS_ D3DVALUE flRolloffFactor, DWORD dwApply) PURE;
    STDMETHOD(SetVelocity)              (THIS_ D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply) PURE;
    STDMETHOD(CommitDeferredSettings)   (THIS) PURE;
};

#define IDirectSound3DListener_QueryInterface(p,a,b)            IUnknown_QueryInterface(p,a,b)
#define IDirectSound3DListener_AddRef(p)                        IUnknown_AddRef(p)
#define IDirectSound3DListener_Release(p)                       IUnknown_Release(p)

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSound3DListener_GetAllParameters(p,a)            (p)->lpVtbl->GetAllParameters(p,a)
#define IDirectSound3DListener_GetDistanceFactor(p,a)           (p)->lpVtbl->GetDistanceFactor(p,a)
#define IDirectSound3DListener_GetDopplerFactor(p,a)            (p)->lpVtbl->GetDopplerFactor(p,a)
#define IDirectSound3DListener_GetOrientation(p,a,b)            (p)->lpVtbl->GetOrientation(p,a,b)
#define IDirectSound3DListener_GetPosition(p,a)                 (p)->lpVtbl->GetPosition(p,a)
#define IDirectSound3DListener_GetRolloffFactor(p,a)            (p)->lpVtbl->GetRolloffFactor(p,a)
#define IDirectSound3DListener_GetVelocity(p,a)                 (p)->lpVtbl->GetVelocity(p,a)
#define IDirectSound3DListener_SetAllParameters(p,a,b)          (p)->lpVtbl->SetAllParameters(p,a,b)
#define IDirectSound3DListener_SetDistanceFactor(p,a,b)         (p)->lpVtbl->SetDistanceFactor(p,a,b)
#define IDirectSound3DListener_SetDopplerFactor(p,a,b)          (p)->lpVtbl->SetDopplerFactor(p,a,b)
#define IDirectSound3DListener_SetOrientation(p,a,b,c,d,e,f,g)  (p)->lpVtbl->SetOrientation(p,a,b,c,d,e,f,g)
#define IDirectSound3DListener_SetPosition(p,a,b,c,d)           (p)->lpVtbl->SetPosition(p,a,b,c,d)
#define IDirectSound3DListener_SetRolloffFactor(p,a,b)          (p)->lpVtbl->SetRolloffFactor(p,a,b)
#define IDirectSound3DListener_SetVelocity(p,a,b,c,d)           (p)->lpVtbl->SetVelocity(p,a,b,c,d)
#define IDirectSound3DListener_CommitDeferredSettings(p)        (p)->lpVtbl->CommitDeferredSettings(p)
#else // !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSound3DListener_GetAllParameters(p,a)            (p)->GetAllParameters(a)
#define IDirectSound3DListener_GetDistanceFactor(p,a)           (p)->GetDistanceFactor(a)
#define IDirectSound3DListener_GetDopplerFactor(p,a)            (p)->GetDopplerFactor(a)
#define IDirectSound3DListener_GetOrientation(p,a,b)            (p)->GetOrientation(a,b)
#define IDirectSound3DListener_GetPosition(p,a)                 (p)->GetPosition(a)
#define IDirectSound3DListener_GetRolloffFactor(p,a)            (p)->GetRolloffFactor(a)
#define IDirectSound3DListener_GetVelocity(p,a)                 (p)->GetVelocity(a)
#define IDirectSound3DListener_SetAllParameters(p,a,b)          (p)->SetAllParameters(a,b)
#define IDirectSound3DListener_SetDistanceFactor(p,a,b)         (p)->SetDistanceFactor(a,b)
#define IDirectSound3DListener_SetDopplerFactor(p,a,b)          (p)->SetDopplerFactor(a,b)
#define IDirectSound3DListener_SetOrientation(p,a,b,c,d,e,f,g)  (p)->SetOrientation(a,b,c,d,e,f,g)
#define IDirectSound3DListener_SetPosition(p,a,b,c,d)           (p)->SetPosition(a,b,c,d)
#define IDirectSound3DListener_SetRolloffFactor(p,a,b)          (p)->SetRolloffFactor(a,b)
#define IDirectSound3DListener_SetVelocity(p,a,b,c,d)           (p)->SetVelocity(a,b,c,d)
#define IDirectSound3DListener_CommitDeferredSettings(p)        (p)->CommitDeferredSettings()
#endif // !defined(__cplusplus) || defined(CINTERFACE)

//
// IDirectSound3DBuffer
//

DEFINE_GUID(IID_IDirectSound3DBuffer, 0x279AFA86, 0x4981, 0x11CE, 0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60);

#undef INTERFACE
#define INTERFACE IDirectSound3DBuffer

DECLARE_INTERFACE_(IDirectSound3DBuffer, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // IDirectSound3DBuffer methods
    STDMETHOD(GetAllParameters)     (THIS_ LPDS3DBUFFER pDs3dBuffer) PURE;
    STDMETHOD(GetConeAngles)        (THIS_ LPDWORD pdwInsideConeAngle, LPDWORD pdwOutsideConeAngle) PURE;
    STDMETHOD(GetConeOrientation)   (THIS_ D3DVECTOR* pvOrientation) PURE;
    STDMETHOD(GetConeOutsideVolume) (THIS_ LPLONG plConeOutsideVolume) PURE;
    STDMETHOD(GetMaxDistance)       (THIS_ D3DVALUE* pflMaxDistance) PURE;
    STDMETHOD(GetMinDistance)       (THIS_ D3DVALUE* pflMinDistance) PURE;
    STDMETHOD(GetMode)              (THIS_ LPDWORD pdwMode) PURE;
    STDMETHOD(GetPosition)          (THIS_ D3DVECTOR* pvPosition) PURE;
    STDMETHOD(GetVelocity)          (THIS_ D3DVECTOR* pvVelocity) PURE;
    STDMETHOD(SetAllParameters)     (THIS_ LPCDS3DBUFFER pcDs3dBuffer, DWORD dwApply) PURE;
    STDMETHOD(SetConeAngles)        (THIS_ DWORD dwInsideConeAngle, DWORD dwOutsideConeAngle, DWORD dwApply) PURE;
    STDMETHOD(SetConeOrientation)   (THIS_ D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply) PURE;
    STDMETHOD(SetConeOutsideVolume) (THIS_ LONG lConeOutsideVolume, DWORD dwApply) PURE;
    STDMETHOD(SetMaxDistance)       (THIS_ D3DVALUE flMaxDistance, DWORD dwApply) PURE;
    STDMETHOD(SetMinDistance)       (THIS_ D3DVALUE flMinDistance, DWORD dwApply) PURE;
    STDMETHOD(SetMode)              (THIS_ DWORD dwMode, DWORD dwApply) PURE;
    STDMETHOD(SetPosition)          (THIS_ D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply) PURE;
    STDMETHOD(SetVelocity)          (THIS_ D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply) PURE;
};

#define IDirectSound3DBuffer_QueryInterface(p,a,b)          IUnknown_QueryInterface(p,a,b)
#define IDirectSound3DBuffer_AddRef(p)                      IUnknown_AddRef(p)
#define IDirectSound3DBuffer_Release(p)                     IUnknown_Release(p)

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSound3DBuffer_GetAllParameters(p,a)          (p)->lpVtbl->GetAllParameters(p,a)
#define IDirectSound3DBuffer_GetConeAngles(p,a,b)           (p)->lpVtbl->GetConeAngles(p,a,b)
#define IDirectSound3DBuffer_GetConeOrientation(p,a)        (p)->lpVtbl->GetConeOrientation(p,a)
#define IDirectSound3DBuffer_GetConeOutsideVolume(p,a)      (p)->lpVtbl->GetConeOutsideVolume(p,a)
#define IDirectSound3DBuffer_GetPosition(p,a)               (p)->lpVtbl->GetPosition(p,a)
#define IDirectSound3DBuffer_GetMinDistance(p,a)            (p)->lpVtbl->GetMinDistance(p,a)
#define IDirectSound3DBuffer_GetMaxDistance(p,a)            (p)->lpVtbl->GetMaxDistance(p,a)
#define IDirectSound3DBuffer_GetMode(p,a)                   (p)->lpVtbl->GetMode(p,a)
#define IDirectSound3DBuffer_GetVelocity(p,a)               (p)->lpVtbl->GetVelocity(p,a)
#define IDirectSound3DBuffer_SetAllParameters(p,a,b)        (p)->lpVtbl->SetAllParameters(p,a,b)
#define IDirectSound3DBuffer_SetConeAngles(p,a,b,c)         (p)->lpVtbl->SetConeAngles(p,a,b,c)
#define IDirectSound3DBuffer_SetConeOrientation(p,a,b,c,d)  (p)->lpVtbl->SetConeOrientation(p,a,b,c,d)
#define IDirectSound3DBuffer_SetConeOutsideVolume(p,a,b)    (p)->lpVtbl->SetConeOutsideVolume(p,a,b)
#define IDirectSound3DBuffer_SetPosition(p,a,b,c,d)         (p)->lpVtbl->SetPosition(p,a,b,c,d)
#define IDirectSound3DBuffer_SetMinDistance(p,a,b)          (p)->lpVtbl->SetMinDistance(p,a,b)
#define IDirectSound3DBuffer_SetMaxDistance(p,a,b)          (p)->lpVtbl->SetMaxDistance(p,a,b)
#define IDirectSound3DBuffer_SetMode(p,a,b)                 (p)->lpVtbl->SetMode(p,a,b)
#define IDirectSound3DBuffer_SetVelocity(p,a,b,c,d)         (p)->lpVtbl->SetVelocity(p,a,b,c,d)
#else // !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSound3DBuffer_GetAllParameters(p,a)          (p)->GetAllParameters(a)
#define IDirectSound3DBuffer_GetConeAngles(p,a,b)           (p)->GetConeAngles(a,b)
#define IDirectSound3DBuffer_GetConeOrientation(p,a)        (p)->GetConeOrientation(a)
#define IDirectSound3DBuffer_GetConeOutsideVolume(p,a)      (p)->GetConeOutsideVolume(a)
#define IDirectSound3DBuffer_GetPosition(p,a)               (p)->GetPosition(a)
#define IDirectSound3DBuffer_GetMinDistance(p,a)            (p)->GetMinDistance(a)
#define IDirectSound3DBuffer_GetMaxDistance(p,a)            (p)->GetMaxDistance(a)
#define IDirectSound3DBuffer_GetMode(p,a)                   (p)->GetMode(a)
#define IDirectSound3DBuffer_GetVelocity(p,a)               (p)->GetVelocity(a)
#define IDirectSound3DBuffer_SetAllParameters(p,a,b)        (p)->SetAllParameters(a,b)
#define IDirectSound3DBuffer_SetConeAngles(p,a,b,c)         (p)->SetConeAngles(a,b,c)
#define IDirectSound3DBuffer_SetConeOrientation(p,a,b,c,d)  (p)->SetConeOrientation(a,b,c,d)
#define IDirectSound3DBuffer_SetConeOutsideVolume(p,a,b)    (p)->SetConeOutsideVolume(a,b)
#define IDirectSound3DBuffer_SetPosition(p,a,b,c,d)         (p)->SetPosition(a,b,c,d)
#define IDirectSound3DBuffer_SetMinDistance(p,a,b)          (p)->SetMinDistance(a,b)
#define IDirectSound3DBuffer_SetMaxDistance(p,a,b)          (p)->SetMaxDistance(a,b)
#define IDirectSound3DBuffer_SetMode(p,a,b)                 (p)->SetMode(a,b)
#define IDirectSound3DBuffer_SetVelocity(p,a,b,c,d)         (p)->SetVelocity(a,b,c,d)
#endif // !defined(__cplusplus) || defined(CINTERFACE)

//
// IDirectSoundCapture
//

DEFINE_GUID(IID_IDirectSoundCapture, 0xb0210781, 0x89cd, 0x11d0, 0xaf, 0x8, 0x0, 0xa0, 0xc9, 0x25, 0xcd, 0x16);

#undef INTERFACE
#define INTERFACE IDirectSoundCapture

DECLARE_INTERFACE_(IDirectSoundCapture, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // IDirectSoundCapture methods
    STDMETHOD(CreateCaptureBuffer)  (THIS_ LPCDSCBUFFERDESC pcDSCBufferDesc, LPDIRECTSOUNDCAPTUREBUFFER *ppDSCBuffer, LPUNKNOWN pUnkOuter) PURE;
    STDMETHOD(GetCaps)              (THIS_ LPDSCCAPS pDSCCaps) PURE;
    STDMETHOD(Initialize)           (THIS_ LPCGUID pcGuidDevice) PURE;
};

#define IDirectSoundCapture_QueryInterface(p,a,b)           IUnknown_QueryInterface(p,a,b)
#define IDirectSoundCapture_AddRef(p)                       IUnknown_AddRef(p)
#define IDirectSoundCapture_Release(p)                      IUnknown_Release(p)

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundCapture_CreateCaptureBuffer(p,a,b,c)    (p)->lpVtbl->CreateCaptureBuffer(p,a,b,c)
#define IDirectSoundCapture_GetCaps(p,a)                    (p)->lpVtbl->GetCaps(p,a)
#define IDirectSoundCapture_Initialize(p,a)                 (p)->lpVtbl->Initialize(p,a)
#else // !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundCapture_CreateCaptureBuffer(p,a,b,c)    (p)->CreateCaptureBuffer(a,b,c)
#define IDirectSoundCapture_GetCaps(p,a)                    (p)->GetCaps(a)
#define IDirectSoundCapture_Initialize(p,a)                 (p)->Initialize(a)
#endif // !defined(__cplusplus) || defined(CINTERFACE)

//
// IDirectSoundCaptureBuffer
//

DEFINE_GUID(IID_IDirectSoundCaptureBuffer, 0xb0210782, 0x89cd, 0x11d0, 0xaf, 0x8, 0x0, 0xa0, 0xc9, 0x25, 0xcd, 0x16);

#undef INTERFACE
#define INTERFACE IDirectSoundCaptureBuffer

DECLARE_INTERFACE_(IDirectSoundCaptureBuffer, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // IDirectSoundCaptureBuffer methods
    STDMETHOD(GetCaps)              (THIS_ LPDSCBCAPS pDSCBCaps) PURE;
    STDMETHOD(GetCurrentPosition)   (THIS_ LPDWORD pdwCapturePosition, LPDWORD pdwReadPosition) PURE;
    STDMETHOD(GetFormat)            (THIS_ LPWAVEFORMATEX pwfxFormat, DWORD dwSizeAllocated, LPDWORD pdwSizeWritten) PURE;
    STDMETHOD(GetStatus)            (THIS_ LPDWORD pdwStatus) PURE;
    STDMETHOD(Initialize)           (THIS_ LPDIRECTSOUNDCAPTURE pDirectSoundCapture, LPCDSCBUFFERDESC pcDSCBufferDesc) PURE;
    STDMETHOD(Lock)                 (THIS_ DWORD dwOffset, DWORD dwBytes, LPVOID *ppvAudioPtr1, LPDWORD pdwAudioBytes1,
                                           LPVOID *ppvAudioPtr2, LPDWORD pdwAudioBytes2, DWORD dwFlags) PURE;
    STDMETHOD(Start)                (THIS_ DWORD dwFlags) PURE;
    STDMETHOD(Stop)                 (THIS) PURE;
    STDMETHOD(Unlock)               (THIS_ LPVOID pvAudioPtr1, DWORD dwAudioBytes1, LPVOID pvAudioPtr2, DWORD dwAudioBytes2) PURE;
};

#define IDirectSoundCaptureBuffer_QueryInterface(p,a,b)         IUnknown_QueryInterface(p,a,b)
#define IDirectSoundCaptureBuffer_AddRef(p)                     IUnknown_AddRef(p)
#define IDirectSoundCaptureBuffer_Release(p)                    IUnknown_Release(p)

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundCaptureBuffer_GetCaps(p,a)                  (p)->lpVtbl->GetCaps(p,a)
#define IDirectSoundCaptureBuffer_GetCurrentPosition(p,a,b)     (p)->lpVtbl->GetCurrentPosition(p,a,b)
#define IDirectSoundCaptureBuffer_GetFormat(p,a,b,c)            (p)->lpVtbl->GetFormat(p,a,b,c)
#define IDirectSoundCaptureBuffer_GetStatus(p,a)                (p)->lpVtbl->GetStatus(p,a)
#define IDirectSoundCaptureBuffer_Initialize(p,a,b)             (p)->lpVtbl->Initialize(p,a,b)
#define IDirectSoundCaptureBuffer_Lock(p,a,b,c,d,e,f,g)         (p)->lpVtbl->Lock(p,a,b,c,d,e,f,g)
#define IDirectSoundCaptureBuffer_Start(p,a)                    (p)->lpVtbl->Start(p,a)
#define IDirectSoundCaptureBuffer_Stop(p)                       (p)->lpVtbl->Stop(p)
#define IDirectSoundCaptureBuffer_Unlock(p,a,b,c,d)             (p)->lpVtbl->Unlock(p,a,b,c,d)
#else // !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundCaptureBuffer_GetCaps(p,a)                  (p)->GetCaps(a)
#define IDirectSoundCaptureBuffer_GetCurrentPosition(p,a,b)     (p)->GetCurrentPosition(a,b)
#define IDirectSoundCaptureBuffer_GetFormat(p,a,b,c)            (p)->GetFormat(a,b,c)
#define IDirectSoundCaptureBuffer_GetStatus(p,a)                (p)->GetStatus(a)
#define IDirectSoundCaptureBuffer_Initialize(p,a,b)             (p)->Initialize(a,b)
#define IDirectSoundCaptureBuffer_Lock(p,a,b,c,d,e,f,g)         (p)->Lock(a,b,c,d,e,f,g)
#define IDirectSoundCaptureBuffer_Start(p,a)                    (p)->Start(a)
#define IDirectSoundCaptureBuffer_Stop(p)                       (p)->Stop()
#define IDirectSoundCaptureBuffer_Unlock(p,a,b,c,d)             (p)->Unlock(a,b,c,d)
#endif // !defined(__cplusplus) || defined(CINTERFACE)


#if DIRECTSOUND_VERSION >= 0x0800

//
// IDirectSoundCaptureBuffer8
//

DEFINE_GUID(IID_IDirectSoundCaptureBuffer8, 0x990df4, 0xdbb, 0x4872, 0x83, 0x3e, 0x6d, 0x30, 0x3e, 0x80, 0xae, 0xb6);

#undef INTERFACE
#define INTERFACE IDirectSoundCaptureBuffer8

DECLARE_INTERFACE_(IDirectSoundCaptureBuffer8, IDirectSoundCaptureBuffer)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // IDirectSoundCaptureBuffer methods
    STDMETHOD(GetCaps)              (THIS_ LPDSCBCAPS pDSCBCaps) PURE;
    STDMETHOD(GetCurrentPosition)   (THIS_ LPDWORD pdwCapturePosition, LPDWORD pdwReadPosition) PURE;
    STDMETHOD(GetFormat)            (THIS_ LPWAVEFORMATEX pwfxFormat, DWORD dwSizeAllocated, LPDWORD pdwSizeWritten) PURE;
    STDMETHOD(GetStatus)            (THIS_ LPDWORD pdwStatus) PURE;
    STDMETHOD(Initialize)           (THIS_ LPDIRECTSOUNDCAPTURE pDirectSoundCapture, LPCDSCBUFFERDESC pcDSCBufferDesc) PURE;
    STDMETHOD(Lock)                 (THIS_ DWORD dwOffset, DWORD dwBytes, LPVOID *ppvAudioPtr1, LPDWORD pdwAudioBytes1,
                                           LPVOID *ppvAudioPtr2, LPDWORD pdwAudioBytes2, DWORD dwFlags) PURE;
    STDMETHOD(Start)                (THIS_ DWORD dwFlags) PURE;
    STDMETHOD(Stop)                 (THIS) PURE;
    STDMETHOD(Unlock)               (THIS_ LPVOID pvAudioPtr1, DWORD dwAudioBytes1, LPVOID pvAudioPtr2, DWORD dwAudioBytes2) PURE;

    // IDirectSoundCaptureBuffer8 methods
    STDMETHOD(GetObjectInPath)      (THIS_ REFGUID rguidObject, DWORD dwIndex, REFGUID rguidInterface, LPVOID *ppObject) PURE;
    STDMETHOD(GetFXStatus)          (DWORD dwFXCount, LPDWORD pdwFXStatus) PURE;
};

#define IDirectSoundCaptureBuffer8_QueryInterface(p,a,b)            IUnknown_QueryInterface(p,a,b)
#define IDirectSoundCaptureBuffer8_AddRef(p)                        IUnknown_AddRef(p)
#define IDirectSoundCaptureBuffer8_Release(p)                       IUnknown_Release(p)

#define IDirectSoundCaptureBuffer8_GetCaps(p,a)                     IDirectSoundCaptureBuffer_GetCaps(p,a)
#define IDirectSoundCaptureBuffer8_GetCurrentPosition(p,a,b)        IDirectSoundCaptureBuffer_GetCurrentPosition(p,a,b)
#define IDirectSoundCaptureBuffer8_GetFormat(p,a,b,c)               IDirectSoundCaptureBuffer_GetFormat(p,a,b,c)
#define IDirectSoundCaptureBuffer8_GetStatus(p,a)                   IDirectSoundCaptureBuffer_GetStatus(p,a)
#define IDirectSoundCaptureBuffer8_Initialize(p,a,b)                IDirectSoundCaptureBuffer_Initialize(p,a,b)
#define IDirectSoundCaptureBuffer8_Lock(p,a,b,c,d,e,f,g)            IDirectSoundCaptureBuffer_Lock(p,a,b,c,d,e,f,g)
#define IDirectSoundCaptureBuffer8_Start(p,a)                       IDirectSoundCaptureBuffer_Start(p,a)
#define IDirectSoundCaptureBuffer8_Stop(p)                          IDirectSoundCaptureBuffer_Stop(p))
#define IDirectSoundCaptureBuffer8_Unlock(p,a,b,c,d)                IDirectSoundCaptureBuffer_Unlock(p,a,b,c,d)

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundCaptureBuffer8_GetObjectInPath(p,a,b,c,d)       (p)->lpVtbl->GetObjectInPath(p,a,b,c,d)
#define IDirectSoundCaptureBuffer8_GetFXStatus(p,a,b)               (p)->lpVtbl->GetFXStatus(p,a,b)
#else // !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundCaptureBuffer8_GetObjectInPath(p,a,b,c,d)       (p)->GetObjectInPath(a,b,c,d)
#define IDirectSoundCaptureBuffer8_GetFXStatus(p,a,b)               (p)->GetFXStatus(a,b)
#endif // !defined(__cplusplus) || defined(CINTERFACE)

#endif // DIRECTSOUND_VERSION >= 0x0800

//
// IDirectSoundNotify
//

DEFINE_GUID(IID_IDirectSoundNotify, 0xb0210783, 0x89cd, 0x11d0, 0xaf, 0x8, 0x0, 0xa0, 0xc9, 0x25, 0xcd, 0x16);

#undef INTERFACE
#define INTERFACE IDirectSoundNotify

DECLARE_INTERFACE_(IDirectSoundNotify, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)           (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)            (THIS) PURE;
    STDMETHOD_(ULONG,Release)           (THIS) PURE;

    // IDirectSoundNotify methods
    STDMETHOD(SetNotificationPositions) (THIS_ DWORD dwPositionNotifies, LPCDSBPOSITIONNOTIFY pcPositionNotifies) PURE;
};

#define IDirectSoundNotify_QueryInterface(p,a,b)            IUnknown_QueryInterface(p,a,b)
#define IDirectSoundNotify_AddRef(p)                        IUnknown_AddRef(p)
#define IDirectSoundNotify_Release(p)                       IUnknown_Release(p)

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundNotify_SetNotificationPositions(p,a,b)  (p)->lpVtbl->SetNotificationPositions(p,a,b)
#else // !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundNotify_SetNotificationPositions(p,a,b)  (p)->SetNotificationPositions(a,b)
#endif // !defined(__cplusplus) || defined(CINTERFACE)

//
// IKsPropertySet
//

#ifndef _IKsPropertySet_
#define _IKsPropertySet_

#ifdef __cplusplus
// 'struct' not 'class' per the way DECLARE_INTERFACE_ is defined
struct IKsPropertySet;
#endif // __cplusplus

typedef struct IKsPropertySet *LPKSPROPERTYSET;

#define KSPROPERTY_SUPPORT_GET  0x00000001
#define KSPROPERTY_SUPPORT_SET  0x00000002

DEFINE_GUID(IID_IKsPropertySet, 0x31efac30, 0x515c, 0x11d0, 0xa9, 0xaa, 0x00, 0xaa, 0x00, 0x61, 0xbe, 0x93);

#undef INTERFACE
#define INTERFACE IKsPropertySet

DECLARE_INTERFACE_(IKsPropertySet, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)    (THIS) PURE;
    STDMETHOD_(ULONG,Release)   (THIS) PURE;

    // IKsPropertySet methods
    STDMETHOD(Get)              (THIS_ REFGUID rguidPropSet, ULONG ulId, LPVOID pInstanceData, ULONG ulInstanceLength,
                                       LPVOID pPropertyData, ULONG ulDataLength, PULONG pulBytesReturned) PURE;
    STDMETHOD(Set)              (THIS_ REFGUID rguidPropSet, ULONG ulId, LPVOID pInstanceData, ULONG ulInstanceLength,
                                       LPVOID pPropertyData, ULONG ulDataLength) PURE;
    STDMETHOD(QuerySupport)     (THIS_ REFGUID rguidPropSet, ULONG ulId, PULONG pulTypeSupport) PURE;
};

#define IKsPropertySet_QueryInterface(p,a,b)       IUnknown_QueryInterface(p,a,b)
#define IKsPropertySet_AddRef(p)                   IUnknown_AddRef(p)
#define IKsPropertySet_Release(p)                  IUnknown_Release(p)

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IKsPropertySet_Get(p,a,b,c,d,e,f,g)        (p)->lpVtbl->Get(p,a,b,c,d,e,f,g)
#define IKsPropertySet_Set(p,a,b,c,d,e,f)          (p)->lpVtbl->Set(p,a,b,c,d,e,f)
#define IKsPropertySet_QuerySupport(p,a,b,c)       (p)->lpVtbl->QuerySupport(p,a,b,c)
#else // !defined(__cplusplus) || defined(CINTERFACE)
#define IKsPropertySet_Get(p,a,b,c,d,e,f,g)        (p)->Get(a,b,c,d,e,f,g)
#define IKsPropertySet_Set(p,a,b,c,d,e,f)          (p)->Set(a,b,c,d,e,f)
#define IKsPropertySet_QuerySupport(p,a,b,c)       (p)->QuerySupport(a,b,c)
#endif // !defined(__cplusplus) || defined(CINTERFACE)

#endif // _IKsPropertySet_

#if DIRECTSOUND_VERSION >= 0x0800

//
// IDirectSoundFXGargle
//

DEFINE_GUID(IID_IDirectSoundFXGargle, 0xd616f352, 0xd622, 0x11ce, 0xaa, 0xc5, 0x00, 0x20, 0xaf, 0x0b, 0x99, 0xa3);

typedef struct _DSFXGargle
{
    DWORD       dwRateHz;               // Rate of modulation in hz
    DWORD       dwWaveShape;            // DSFXGARGLE_WAVE_xxx
} DSFXGargle, *LPDSFXGargle;

#define DSFXGARGLE_WAVE_TRIANGLE        0
#define DSFXGARGLE_WAVE_SQUARE          1

typedef const DSFXGargle *LPCDSFXGargle;

#define DSFXGARGLE_RATEHZ_MIN           1
#define DSFXGARGLE_RATEHZ_MAX           1000

#undef INTERFACE
#define INTERFACE IDirectSoundFXGargle

DECLARE_INTERFACE_(IDirectSoundFXGargle, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // IDirectSoundFXGargle methods
    STDMETHOD(SetAllParameters)     (THIS_ LPCDSFXGargle pcDsFxGargle) PURE;
    STDMETHOD(GetAllParameters)     (THIS_ LPDSFXGargle pDsFxGargle) PURE;
};

#define IDirectSoundFXGargle_QueryInterface(p,a,b)          IUnknown_QueryInterface(p,a,b)
#define IDirectSoundFXGargle_AddRef(p)                      IUnknown_AddRef(p)
#define IDirectSoundFXGargle_Release(p)                     IUnknown_Release(p)

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundFXGargle_SetAllParameters(p,a)          (p)->lpVtbl->SetAllParameters(p,a)
#define IDirectSoundFXGargle_GetAllParameters(p,a)          (p)->lpVtbl->GetAllParameters(p,a)
#else // !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundFXGargle_SetAllParameters(p,a)          (p)->SetAllParameters(a)
#define IDirectSoundFXGargle_GetAllParameters(p,a)          (p)->GetAllParameters(a)
#endif // !defined(__cplusplus) || defined(CINTERFACE)

//
// IDirectSoundFXChorus
//

DEFINE_GUID(IID_IDirectSoundFXChorus, 0x880842e3, 0x145f, 0x43e6, 0xa9, 0x34, 0xa7, 0x18, 0x06, 0xe5, 0x05, 0x47);

typedef struct _DSFXChorus
{
    FLOAT       fWetDryMix;
    FLOAT       fDepth;
    FLOAT       fFeedback;
    FLOAT       fFrequency;
    LONG        lWaveform;          // LFO shape; DSFXCHORUS_WAVE_xxx
    FLOAT       fDelay;
    LONG        lPhase;
} DSFXChorus, *LPDSFXChorus;

typedef const DSFXChorus *LPCDSFXChorus;

#define DSFXCHORUS_WAVE_TRIANGLE        0
#define DSFXCHORUS_WAVE_SIN             1

#define DSFXCHORUS_WETDRYMIX_MIN        0.0f
#define DSFXCHORUS_WETDRYMIX_MAX        100.0f
#define DSFXCHORUS_DEPTH_MIN            0.0f
#define DSFXCHORUS_DEPTH_MAX            100.0f
#define DSFXCHORUS_FEEDBACK_MIN         -99.0f
#define DSFXCHORUS_FEEDBACK_MAX         99.0f
#define DSFXCHORUS_FREQUENCY_MIN        0.0f
#define DSFXCHORUS_FREQUENCY_MAX        10.0f
#define DSFXCHORUS_DELAY_MIN            0.0f
#define DSFXCHORUS_DELAY_MAX            20.0f
#define DSFXCHORUS_PHASE_MIN            0
#define DSFXCHORUS_PHASE_MAX            4

#define DSFXCHORUS_PHASE_NEG_180        0
#define DSFXCHORUS_PHASE_NEG_90         1
#define DSFXCHORUS_PHASE_ZERO           2
#define DSFXCHORUS_PHASE_90             3
#define DSFXCHORUS_PHASE_180            4

#undef INTERFACE
#define INTERFACE IDirectSoundFXChorus

DECLARE_INTERFACE_(IDirectSoundFXChorus, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // IDirectSoundFXChorus methods
    STDMETHOD(SetAllParameters)     (THIS_ LPCDSFXChorus pcDsFxChorus) PURE;
    STDMETHOD(GetAllParameters)     (THIS_ LPDSFXChorus pDsFxChorus) PURE;
};

#define IDirectSoundFXChorus_QueryInterface(p,a,b)          IUnknown_QueryInterface(p,a,b)
#define IDirectSoundFXChorus_AddRef(p)                      IUnknown_AddRef(p)
#define IDirectSoundFXChorus_Release(p)                     IUnknown_Release(p)

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundFXChorus_SetAllParameters(p,a)          (p)->lpVtbl->SetAllParameters(p,a)
#define IDirectSoundFXChorus_GetAllParameters(p,a)          (p)->lpVtbl->GetAllParameters(p,a)
#else // !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundFXChorus_SetAllParameters(p,a)          (p)->SetAllParameters(a)
#define IDirectSoundFXChorus_GetAllParameters(p,a)          (p)->GetAllParameters(a)
#endif // !defined(__cplusplus) || defined(CINTERFACE)

//
// IDirectSoundFXFlanger
//

DEFINE_GUID(IID_IDirectSoundFXFlanger, 0x903e9878, 0x2c92, 0x4072, 0x9b, 0x2c, 0xea, 0x68, 0xf5, 0x39, 0x67, 0x83);

typedef struct _DSFXFlanger
{
    FLOAT       fWetDryMix;
    FLOAT       fDepth;
    FLOAT       fFeedback;
    FLOAT       fFrequency;
    LONG        lWaveform;
    FLOAT       fDelay;
    LONG        lPhase;
} DSFXFlanger, *LPDSFXFlanger;

typedef const DSFXFlanger *LPCDSFXFlanger;

#define DSFXFLANGER_WAVE_TRIANGLE       0
#define DSFXFLANGER_WAVE_SIN            1

#define DSFXFLANGER_WETDRYMIX_MIN       0.0f
#define DSFXFLANGER_WETDRYMIX_MAX       100.0f
#define DSFXFLANGER_FREQUENCY_MIN       0.0f
#define DSFXFLANGER_FREQUENCY_MAX       10.0f
#define DSFXFLANGER_DEPTH_MIN           0.0f
#define DSFXFLANGER_DEPTH_MAX           100.0f
#define DSFXFLANGER_PHASE_MIN           0
#define DSFXFLANGER_PHASE_MAX           4
#define DSFXFLANGER_FEEDBACK_MIN        -99.0f
#define DSFXFLANGER_FEEDBACK_MAX        99.0f
#define DSFXFLANGER_DELAY_MIN           0.0f
#define DSFXFLANGER_DELAY_MAX           4.0f

#define DSFXFLANGER_PHASE_NEG_180       0
#define DSFXFLANGER_PHASE_NEG_90        1
#define DSFXFLANGER_PHASE_ZERO          2
#define DSFXFLANGER_PHASE_90            3
#define DSFXFLANGER_PHASE_180           4

#undef INTERFACE
#define INTERFACE IDirectSoundFXFlanger

DECLARE_INTERFACE_(IDirectSoundFXFlanger, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // IDirectSoundFXFlanger methods
    STDMETHOD(SetAllParameters)     (THIS_ LPCDSFXFlanger pcDsFxFlanger) PURE;
    STDMETHOD(GetAllParameters)     (THIS_ LPDSFXFlanger pDsFxFlanger) PURE;
};

#define IDirectSoundFXFlanger_QueryInterface(p,a,b)         IUnknown_QueryInterface(p,a,b)
#define IDirectSoundFXFlanger_AddRef(p)                     IUnknown_AddRef(p)
#define IDirectSoundFXFlanger_Release(p)                    IUnknown_Release(p)

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundFXFlanger_SetAllParameters(p,a)         (p)->lpVtbl->SetAllParameters(p,a)
#define IDirectSoundFXFlanger_GetAllParameters(p,a)         (p)->lpVtbl->GetAllParameters(p,a)
#else // !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundFXFlanger_SetAllParameters(p,a)         (p)->SetAllParameters(a)
#define IDirectSoundFXFlanger_GetAllParameters(p,a)         (p)->GetAllParameters(a)
#endif // !defined(__cplusplus) || defined(CINTERFACE)

//
// IDirectSoundFXEcho
//

DEFINE_GUID(IID_IDirectSoundFXEcho, 0x8bd28edf, 0x50db, 0x4e92, 0xa2, 0xbd, 0x44, 0x54, 0x88, 0xd1, 0xed, 0x42);

typedef struct _DSFXEcho
{
    FLOAT   fWetDryMix;
    FLOAT   fFeedback;
    FLOAT   fLeftDelay;
    FLOAT   fRightDelay;
    LONG    lPanDelay;
} DSFXEcho, *LPDSFXEcho;

typedef const DSFXEcho *LPCDSFXEcho;

#define DSFXECHO_WETDRYMIX_MIN      0.0f
#define DSFXECHO_WETDRYMIX_MAX      100.0f
#define DSFXECHO_FEEDBACK_MIN       0.0f
#define DSFXECHO_FEEDBACK_MAX       100.0f
#define DSFXECHO_LEFTDELAY_MIN      1.0f
#define DSFXECHO_LEFTDELAY_MAX      2000.0f
#define DSFXECHO_RIGHTDELAY_MIN     1.0f
#define DSFXECHO_RIGHTDELAY_MAX     2000.0f
#define DSFXECHO_PANDELAY_MIN       0
#define DSFXECHO_PANDELAY_MAX       1

#undef INTERFACE
#define INTERFACE IDirectSoundFXEcho

DECLARE_INTERFACE_(IDirectSoundFXEcho, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // IDirectSoundFXEcho methods
    STDMETHOD(SetAllParameters)     (THIS_ LPCDSFXEcho pcDsFxEcho) PURE;
    STDMETHOD(GetAllParameters)     (THIS_ LPDSFXEcho pDsFxEcho) PURE;
};

#define IDirectSoundFXEcho_QueryInterface(p,a,b)            IUnknown_QueryInterface(p,a,b)
#define IDirectSoundFXEcho_AddRef(p)                        IUnknown_AddRef(p)
#define IDirectSoundFXEcho_Release(p)                       IUnknown_Release(p)

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundFXEcho_SetAllParameters(p,a)            (p)->lpVtbl->SetAllParameters(p,a)
#define IDirectSoundFXEcho_GetAllParameters(p,a)            (p)->lpVtbl->GetAllParameters(p,a)
#else // !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundFXEcho_SetAllParameters(p,a)            (p)->SetAllParameters(a)
#define IDirectSoundFXEcho_GetAllParameters(p,a)            (p)->GetAllParameters(a)
#endif // !defined(__cplusplus) || defined(CINTERFACE)

//
// IDirectSoundFXDistortion
//

DEFINE_GUID(IID_IDirectSoundFXDistortion, 0x8ecf4326, 0x455f, 0x4d8b, 0xbd, 0xa9, 0x8d, 0x5d, 0x3e, 0x9e, 0x3e, 0x0b);

typedef struct _DSFXDistortion
{
    FLOAT   fGain;
    FLOAT   fEdge;
    FLOAT   fPostEQCenterFrequency;
    FLOAT   fPostEQBandwidth;
    FLOAT   fPreLowpassCutoff;
} DSFXDistortion, *LPDSFXDistortion;

typedef const DSFXDistortion *LPCDSFXDistortion;

#define DSFXDISTORTION_GAIN_MIN                     -60.0f
#define DSFXDISTORTION_GAIN_MAX                     0.0f
#define DSFXDISTORTION_EDGE_MIN                     0.0f
#define DSFXDISTORTION_EDGE_MAX                     100.0f
#define DSFXDISTORTION_POSTEQCENTERFREQUENCY_MIN    100.0f
#define DSFXDISTORTION_POSTEQCENTERFREQUENCY_MAX    8000.0f
#define DSFXDISTORTION_POSTEQBANDWIDTH_MIN          100.0f
#define DSFXDISTORTION_POSTEQBANDWIDTH_MAX          8000.0f
#define DSFXDISTORTION_PRELOWPASSCUTOFF_MIN         100.0f
#define DSFXDISTORTION_PRELOWPASSCUTOFF_MAX         8000.0f

#undef INTERFACE
#define INTERFACE IDirectSoundFXDistortion

DECLARE_INTERFACE_(IDirectSoundFXDistortion, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // IDirectSoundFXDistortion methods
    STDMETHOD(SetAllParameters)     (THIS_ LPCDSFXDistortion pcDsFxDistortion) PURE;
    STDMETHOD(GetAllParameters)     (THIS_ LPDSFXDistortion pDsFxDistortion) PURE;
};

#define IDirectSoundFXDistortion_QueryInterface(p,a,b)      IUnknown_QueryInterface(p,a,b)
#define IDirectSoundFXDistortion_AddRef(p)                  IUnknown_AddRef(p)
#define IDirectSoundFXDistortion_Release(p)                 IUnknown_Release(p)

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundFXDistortion_SetAllParameters(p,a)      (p)->lpVtbl->SetAllParameters(p,a)
#define IDirectSoundFXDistortion_GetAllParameters(p,a)      (p)->lpVtbl->GetAllParameters(p,a)
#else // !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundFXDistortion_SetAllParameters(p,a)      (p)->SetAllParameters(a)
#define IDirectSoundFXDistortion_GetAllParameters(p,a)      (p)->GetAllParameters(a)
#endif // !defined(__cplusplus) || defined(CINTERFACE)

//
// IDirectSoundFXCompressor
//

DEFINE_GUID(IID_IDirectSoundFXCompressor, 0x4bbd1154, 0x62f6, 0x4e2c, 0xa1, 0x5c, 0xd3, 0xb6, 0xc4, 0x17, 0xf7, 0xa0);

typedef struct _DSFXCompressor
{
    FLOAT   fGain;
    FLOAT   fAttack;
    FLOAT   fRelease;
    FLOAT   fThreshold;
    FLOAT   fRatio;
    FLOAT   fPredelay;
} DSFXCompressor, *LPDSFXCompressor;

typedef const DSFXCompressor *LPCDSFXCompressor;

#define DSFXCOMPRESSOR_GAIN_MIN             -60.0f
#define DSFXCOMPRESSOR_GAIN_MAX             60.0f
#define DSFXCOMPRESSOR_ATTACK_MIN           0.01f
#define DSFXCOMPRESSOR_ATTACK_MAX           500.0f
#define DSFXCOMPRESSOR_RELEASE_MIN          50.0f
#define DSFXCOMPRESSOR_RELEASE_MAX          3000.0f
#define DSFXCOMPRESSOR_THRESHOLD_MIN        -60.0f
#define DSFXCOMPRESSOR_THRESHOLD_MAX        0.0f
#define DSFXCOMPRESSOR_RATIO_MIN            1.0f
#define DSFXCOMPRESSOR_RATIO_MAX            100.0f
#define DSFXCOMPRESSOR_PREDELAY_MIN         0.0f
#define DSFXCOMPRESSOR_PREDELAY_MAX         4.0f

#undef INTERFACE
#define INTERFACE IDirectSoundFXCompressor

DECLARE_INTERFACE_(IDirectSoundFXCompressor, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // IDirectSoundFXCompressor methods
    STDMETHOD(SetAllParameters)     (THIS_ LPCDSFXCompressor pcDsFxCompressor) PURE;
    STDMETHOD(GetAllParameters)     (THIS_ LPDSFXCompressor pDsFxCompressor) PURE;
};

#define IDirectSoundFXCompressor_QueryInterface(p,a,b)      IUnknown_QueryInterface(p,a,b)
#define IDirectSoundFXCompressor_AddRef(p)                  IUnknown_AddRef(p)
#define IDirectSoundFXCompressor_Release(p)                 IUnknown_Release(p)

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundFXCompressor_SetAllParameters(p,a)      (p)->lpVtbl->SetAllParameters(p,a)
#define IDirectSoundFXCompressor_GetAllParameters(p,a)      (p)->lpVtbl->GetAllParameters(p,a)
#else // !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundFXCompressor_SetAllParameters(p,a)      (p)->SetAllParameters(a)
#define IDirectSoundFXCompressor_GetAllParameters(p,a)      (p)->GetAllParameters(a)
#endif // !defined(__cplusplus) || defined(CINTERFACE)

//
// IDirectSoundFXParamEq
//

DEFINE_GUID(IID_IDirectSoundFXParamEq, 0xc03ca9fe, 0xfe90, 0x4204, 0x80, 0x78, 0x82, 0x33, 0x4c, 0xd1, 0x77, 0xda);

typedef struct _DSFXParamEq
{
    FLOAT   fCenter;
    FLOAT   fBandwidth;
    FLOAT   fGain;
} DSFXParamEq, *LPDSFXParamEq;

typedef const DSFXParamEq *LPCDSFXParamEq;

#define DSFXPARAMEQ_CENTER_MIN      80.0f
#define DSFXPARAMEQ_CENTER_MAX      16000.0f
#define DSFXPARAMEQ_BANDWIDTH_MIN   1.0f
#define DSFXPARAMEQ_BANDWIDTH_MAX   36.0f
#define DSFXPARAMEQ_GAIN_MIN        -15.0f
#define DSFXPARAMEQ_GAIN_MAX        15.0f

#undef INTERFACE
#define INTERFACE IDirectSoundFXParamEq

DECLARE_INTERFACE_(IDirectSoundFXParamEq, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // IDirectSoundFXParamEq methods
    STDMETHOD(SetAllParameters)     (THIS_ LPCDSFXParamEq pcDsFxParamEq) PURE;
    STDMETHOD(GetAllParameters)     (THIS_ LPDSFXParamEq pDsFxParamEq) PURE;
};

#define IDirectSoundFXParamEq_QueryInterface(p,a,b)      IUnknown_QueryInterface(p,a,b)
#define IDirectSoundFXParamEq_AddRef(p)                  IUnknown_AddRef(p)
#define IDirectSoundFXParamEq_Release(p)                 IUnknown_Release(p)

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundFXParamEq_SetAllParameters(p,a)      (p)->lpVtbl->SetAllParameters(p,a)
#define IDirectSoundFXParamEq_GetAllParameters(p,a)      (p)->lpVtbl->GetAllParameters(p,a)
#else // !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundFXParamEq_SetAllParameters(p,a)      (p)->SetAllParameters(a)
#define IDirectSoundFXParamEq_GetAllParameters(p,a)      (p)->GetAllParameters(a)
#endif // !defined(__cplusplus) || defined(CINTERFACE)


//
// IDirectSoundFXI3DL2Reverb
//

DEFINE_GUID(IID_IDirectSoundFXI3DL2Reverb, 0x4b166a6a, 0x0d66, 0x43f3, 0x80, 0xe3, 0xee, 0x62, 0x80, 0xde, 0xe1, 0xa4);

typedef struct _DSFXI3DL2Reverb
{
    LONG    lRoom;                  // [-10000, 0]      default: -1000 mB
    LONG    lRoomHF;                // [-10000, 0]      default: 0 mB
    FLOAT   flRoomRolloffFactor;    // [0.0, 10.0]      default: 0.0
    FLOAT   flDecayTime;            // [0.1, 20.0]      default: 1.49s
    FLOAT   flDecayHFRatio;         // [0.1, 2.0]       default: 0.83
    LONG    lReflections;           // [-10000, 1000]   default: -2602 mB
    FLOAT   flReflectionsDelay;     // [0.0, 0.3]       default: 0.007 s
    LONG    lReverb;                // [-10000, 2000]   default: 200 mB
    FLOAT   flReverbDelay;          // [0.0, 0.1]       default: 0.011 s
    FLOAT   flDiffusion;            // [0.0, 100.0]     default: 100.0 %
    FLOAT   flDensity;              // [0.0, 100.0]     default: 100.0 %
    FLOAT   flHFReference;          // [20.0, 20000.0]  default: 5000.0 Hz
} DSFXI3DL2Reverb, *LPDSFXI3DL2Reverb;

typedef const DSFXI3DL2Reverb *LPCDSFXI3DL2Reverb;

#define DSFX_I3DL2REVERB_ROOM_MIN                   (-10000)
#define DSFX_I3DL2REVERB_ROOM_MAX                   0
#define DSFX_I3DL2REVERB_ROOM_DEFAULT               (-1000)
                                                    
#define DSFX_I3DL2REVERB_ROOMHF_MIN                 (-10000)
#define DSFX_I3DL2REVERB_ROOMHF_MAX                 0
#define DSFX_I3DL2REVERB_ROOMHF_DEFAULT             (-100)
                                                    
#define DSFX_I3DL2REVERB_ROOMROLLOFFFACTOR_MIN      0.0f
#define DSFX_I3DL2REVERB_ROOMROLLOFFFACTOR_MAX      10.0f
#define DSFX_I3DL2REVERB_ROOMROLLOFFFACTOR_DEFAULT  0.0f

#define DSFX_I3DL2REVERB_DECAYTIME_MIN              0.1f
#define DSFX_I3DL2REVERB_DECAYTIME_MAX              20.0f
#define DSFX_I3DL2REVERB_DECAYTIME_DEFAULT          1.49f
                                                    
#define DSFX_I3DL2REVERB_DECAYHFRATIO_MIN           0.1f
#define DSFX_I3DL2REVERB_DECAYHFRATIO_MAX           2.0f
#define DSFX_I3DL2REVERB_DECAYHFRATIO_DEFAULT       0.83f
                                                    
#define DSFX_I3DL2REVERB_REFLECTIONS_MIN            (-10000)
#define DSFX_I3DL2REVERB_REFLECTIONS_MAX            1000
#define DSFX_I3DL2REVERB_REFLECTIONS_DEFAULT        (-2602)
                                                    
#define DSFX_I3DL2REVERB_REFLECTIONSDELAY_MIN       0.0f
#define DSFX_I3DL2REVERB_REFLECTIONSDELAY_MAX       0.3f
#define DSFX_I3DL2REVERB_REFLECTIONSDELAY_DEFAULT   0.007f

#define DSFX_I3DL2REVERB_REVERB_MIN                 (-10000)
#define DSFX_I3DL2REVERB_REVERB_MAX                 2000
#define DSFX_I3DL2REVERB_REVERB_DEFAULT             (200)
                                                    
#define DSFX_I3DL2REVERB_REVERBDELAY_MIN            0.0f
#define DSFX_I3DL2REVERB_REVERBDELAY_MAX            0.1f
#define DSFX_I3DL2REVERB_REVERBDELAY_DEFAULT        0.011f
                                                    
#define DSFX_I3DL2REVERB_DIFFUSION_MIN              0.0f
#define DSFX_I3DL2REVERB_DIFFUSION_MAX              100.0f
#define DSFX_I3DL2REVERB_DIFFUSION_DEFAULT          100.0f
                                                    
#define DSFX_I3DL2REVERB_DENSITY_MIN                0.0f
#define DSFX_I3DL2REVERB_DENSITY_MAX                100.0f
#define DSFX_I3DL2REVERB_DENSITY_DEFAULT            100.0f
                                                    
#define DSFX_I3DL2REVERB_HFREFERENCE_MIN            20.0f
#define DSFX_I3DL2REVERB_HFREFERENCE_MAX            20000.0f
#define DSFX_I3DL2REVERB_HFREFERENCE_DEFAULT        5000.0f
                                                    
#define DSFX_I3DL2REVERB_QUALITY_MIN                0
#define DSFX_I3DL2REVERB_QUALITY_MAX                3
#define DSFX_I3DL2REVERB_QUALITY_DEFAULT            2

#undef INTERFACE
#define INTERFACE IDirectSoundFXI3DL2Reverb

DECLARE_INTERFACE_(IDirectSoundFXI3DL2Reverb, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // IDirectSoundFXI3DL2Reverb methods
    STDMETHOD(SetAllParameters)     (THIS_ LPCDSFXI3DL2Reverb pcDsFxI3DL2Reverb) PURE;
    STDMETHOD(GetAllParameters)     (THIS_ LPDSFXI3DL2Reverb pDsFxI3DL2Reverb) PURE;
    STDMETHOD(SetPreset)            (THIS_ DWORD dwPreset) PURE;
    STDMETHOD(GetPreset)            (THIS_ LPDWORD pdwPreset) PURE;
    STDMETHOD(SetQuality)           (THIS_ LONG lQuality) PURE;
    STDMETHOD(GetQuality)           (THIS_ LONG *plQuality) PURE;
};

#define IDirectSoundFXI3DL2Reverb_QueryInterface(p,a,b)     IUnknown_QueryInterface(p,a,b)
#define IDirectSoundFXI3DL2Reverb_AddRef(p)                 IUnknown_AddRef(p)
#define IDirectSoundFXI3DL2Reverb_Release(p)                IUnknown_Release(p)

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundFXI3DL2Reverb_SetAllParameters(p,a)     (p)->lpVtbl->SetAllParameters(p,a)
#define IDirectSoundFXI3DL2Reverb_GetAllParameters(p,a)     (p)->lpVtbl->GetAllParameters(p,a)
#define IDirectSoundFXI3DL2Reverb_SetPreset(p,a)            (p)->lpVtbl->SetPreset(p,a)
#define IDirectSoundFXI3DL2Reverb_GetPreset(p,a)            (p)->lpVtbl->GetPreset(p,a)
#else // !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundFXI3DL2Reverb_SetAllParameters(p,a)     (p)->SetAllParameters(a)
#define IDirectSoundFXI3DL2Reverb_GetAllParameters(p,a)     (p)->GetAllParameters(a)
#define IDirectSoundFXI3DL2Reverb_SetPreset(p,a)            (p)->SetPreset(a)
#define IDirectSoundFXI3DL2Reverb_GetPreset(p,a)            (p)->GetPreset(a)
#endif // !defined(__cplusplus) || defined(CINTERFACE)


//
// IDirectSoundFXWavesReverb
//

DEFINE_GUID(IID_IDirectSoundFXWavesReverb,0x46858c3a,0x0dc6,0x45e3,0xb7,0x60,0xd4,0xee,0xf1,0x6c,0xb3,0x25);

typedef struct _DSFXWavesReverb
{
    FLOAT   fInGain;                // [-96.0,0.0]            default: 0.0 dB
    FLOAT   fReverbMix;             // [-96.0,0.0]            default: 0.0 db
    FLOAT   fReverbTime;            // [0.001,3000.0]         default: 1000.0 ms
    FLOAT   fHighFreqRTRatio;       // [0.001,0.999]          default: 0.001
} DSFXWavesReverb, *LPDSFXWavesReverb;

typedef const DSFXWavesReverb *LPCDSFXWavesReverb;

#define DSFX_WAVESREVERB_INGAIN_MIN                 -96.0f
#define DSFX_WAVESREVERB_INGAIN_MAX                 0.0f
#define DSFX_WAVESREVERB_INGAIN_DEFAULT             0.0f
#define DSFX_WAVESREVERB_REVERBMIX_MIN              -96.0f
#define DSFX_WAVESREVERB_REVERBMIX_MAX              0.0f
#define DSFX_WAVESREVERB_REVERBMIX_DEFAULT          0.0f
#define DSFX_WAVESREVERB_REVERBTIME_MIN             0.001f
#define DSFX_WAVESREVERB_REVERBTIME_MAX             3000.0f
#define DSFX_WAVESREVERB_REVERBTIME_DEFAULT         1000.0f
#define DSFX_WAVESREVERB_HIGHFREQRTRATIO_MIN        0.001f
#define DSFX_WAVESREVERB_HIGHFREQRTRATIO_MAX        0.999f
#define DSFX_WAVESREVERB_HIGHFREQRTRATIO_DEFAULT    0.001f

#undef INTERFACE
#define INTERFACE IDirectSoundFXWavesReverb

DECLARE_INTERFACE_(IDirectSoundFXWavesReverb, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // IDirectSoundFXWavesReverb methods
    STDMETHOD(SetAllParameters)     (THIS_ LPCDSFXWavesReverb pcDsFxWavesReverb) PURE;
    STDMETHOD(GetAllParameters)     (THIS_ LPDSFXWavesReverb pDsFxWavesReverb) PURE;
};

#define IDirectSoundFXWavesReverb_QueryInterface(p,a,b)     IUnknown_QueryInterface(p,a,b)
#define IDirectSoundFXWavesReverb_AddRef(p)                 IUnknown_AddRef(p)
#define IDirectSoundFXWavesReverb_Release(p)                IUnknown_Release(p)

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundFXWavesReverb_SetAllParameters(p,a)     (p)->lpVtbl->SetAllParameters(p,a)
#define IDirectSoundFXWavesReverb_GetAllParameters(p,a)     (p)->lpVtbl->GetAllParameters(p,a)
#else // !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundFXWavesReverb_SetAllParameters(p,a)     (p)->SetAllParameters(a)
#define IDirectSoundFXWavesReverb_GetAllParameters(p,a)     (p)->GetAllParameters(a)
#endif // !defined(__cplusplus) || defined(CINTERFACE)

//
// IDirectSoundCaptureFXAec
//

DEFINE_GUID(IID_IDirectSoundCaptureFXAec, 0xad74143d, 0x903d, 0x4ab7, 0x80, 0x66, 0x28, 0xd3, 0x63, 0x03, 0x6d, 0x65);

typedef struct _DSCFXAec
{
    BOOL    fEnable;
    BOOL    fNoiseFill;
    DWORD   dwMode;
} DSCFXAec, *LPDSCFXAec;

typedef const DSCFXAec *LPCDSCFXAec;

// These match the AEC_MODE_* constants in the DDK's ksmedia.h file
#define DSCFX_AEC_MODE_PASS_THROUGH                     0x0
#define DSCFX_AEC_MODE_HALF_DUPLEX                      0x1
#define DSCFX_AEC_MODE_FULL_DUPLEX                      0x2

// These match the AEC_STATUS_* constants in ksmedia.h
#define DSCFX_AEC_STATUS_HISTORY_UNINITIALIZED          0x0
#define DSCFX_AEC_STATUS_HISTORY_CONTINUOUSLY_CONVERGED 0x1
#define DSCFX_AEC_STATUS_HISTORY_PREVIOUSLY_DIVERGED    0x2
#define DSCFX_AEC_STATUS_CURRENTLY_CONVERGED            0x8

#undef INTERFACE
#define INTERFACE IDirectSoundCaptureFXAec

DECLARE_INTERFACE_(IDirectSoundCaptureFXAec, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // IDirectSoundCaptureFXAec methods
    STDMETHOD(SetAllParameters)     (THIS_ LPCDSCFXAec pDscFxAec) PURE;
    STDMETHOD(GetAllParameters)     (THIS_ LPDSCFXAec pDscFxAec) PURE;
    STDMETHOD(GetStatus)            (THIS_ PDWORD pdwStatus) PURE;
    STDMETHOD(Reset)                (THIS) PURE;
};

#define IDirectSoundCaptureFXAec_QueryInterface(p,a,b)     IUnknown_QueryInterface(p,a,b)
#define IDirectSoundCaptureFXAec_AddRef(p)                 IUnknown_AddRef(p)
#define IDirectSoundCaptureFXAec_Release(p)                IUnknown_Release(p)

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundCaptureFXAec_SetAllParameters(p,a)     (p)->lpVtbl->SetAllParameters(p,a)
#define IDirectSoundCaptureFXAec_GetAllParameters(p,a)     (p)->lpVtbl->GetAllParameters(p,a)
#else // !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundCaptureFXAec_SetAllParameters(p,a)     (p)->SetAllParameters(a)
#define IDirectSoundCaptureFXAec_GetAllParameters(p,a)     (p)->GetAllParameters(a)
#endif // !defined(__cplusplus) || defined(CINTERFACE)


//
// IDirectSoundCaptureFXNoiseSuppress
//

DEFINE_GUID(IID_IDirectSoundCaptureFXNoiseSuppress, 0xed311e41, 0xfbae, 0x4175, 0x96, 0x25, 0xcd, 0x8, 0x54, 0xf6, 0x93, 0xca);

typedef struct _DSCFXNoiseSuppress
{
    BOOL    fEnable;
} DSCFXNoiseSuppress, *LPDSCFXNoiseSuppress;

typedef const DSCFXNoiseSuppress *LPCDSCFXNoiseSuppress;

#undef INTERFACE
#define INTERFACE IDirectSoundCaptureFXNoiseSuppress

DECLARE_INTERFACE_(IDirectSoundCaptureFXNoiseSuppress, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // IDirectSoundCaptureFXNoiseSuppress methods
    STDMETHOD(SetAllParameters)     (THIS_ LPCDSCFXNoiseSuppress pcDscFxNoiseSuppress) PURE;
    STDMETHOD(GetAllParameters)     (THIS_ LPDSCFXNoiseSuppress pDscFxNoiseSuppress) PURE;
    STDMETHOD(Reset)                (THIS) PURE;
};

#define IDirectSoundCaptureFXNoiseSuppress_QueryInterface(p,a,b)     IUnknown_QueryInterface(p,a,b)
#define IDirectSoundCaptureFXNoiseSuppress_AddRef(p)                 IUnknown_AddRef(p)
#define IDirectSoundCaptureFXNoiseSuppress_Release(p)                IUnknown_Release(p)

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundCaptureFXNoiseSuppress_SetAllParameters(p,a)     (p)->lpVtbl->SetAllParameters(p,a)
#define IDirectSoundCaptureFXNoiseSuppress_GetAllParameters(p,a)     (p)->lpVtbl->GetAllParameters(p,a)
#else // !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundCaptureFXNoiseSuppress_SetAllParameters(p,a)     (p)->SetAllParameters(a)
#define IDirectSoundCaptureFXNoiseSuppress_GetAllParameters(p,a)     (p)->GetAllParameters(a)
#endif // !defined(__cplusplus) || defined(CINTERFACE)


//
// IDirectSoundFullDuplex
//

#ifndef _IDirectSoundFullDuplex_
#define _IDirectSoundFullDuplex_

#ifdef __cplusplus
// 'struct' not 'class' per the way DECLARE_INTERFACE_ is defined
struct IDirectSoundFullDuplex;
#endif // __cplusplus

typedef struct IDirectSoundFullDuplex *LPDIRECTSOUNDFULLDUPLEX;

DEFINE_GUID(IID_IDirectSoundFullDuplex, 0xedcb4c7a, 0xdaab, 0x4216, 0xa4, 0x2e, 0x6c, 0x50, 0x59, 0x6d, 0xdc, 0x1d);

#undef INTERFACE
#define INTERFACE IDirectSoundFullDuplex

DECLARE_INTERFACE_(IDirectSoundFullDuplex, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)    (THIS) PURE;
    STDMETHOD_(ULONG,Release)   (THIS) PURE;

    // IDirectSoundFullDuplex methods 
    STDMETHOD(Initialize)     (THIS_ LPCGUID pCaptureGuid, LPCGUID pRenderGuid, LPCDSCBUFFERDESC lpDscBufferDesc, LPCDSBUFFERDESC lpDsBufferDesc, HWND hWnd, DWORD dwLevel, LPLPDIRECTSOUNDCAPTUREBUFFER8 lplpDirectSoundCaptureBuffer8, LPLPDIRECTSOUNDBUFFER8 lplpDirectSoundBuffer8) PURE;
};

#define IDirectSoundFullDuplex_QueryInterface(p,a,b)    IUnknown_QueryInterface(p,a,b)
#define IDirectSoundFullDuplex_AddRef(p)                IUnknown_AddRef(p)
#define IDirectSoundFullDuplex_Release(p)               IUnknown_Release(p)

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundFullDuplex_Initialize(p,a,b,c,d,e,f,g,h)     (p)->lpVtbl->Initialize(p,a,b,c,d,e,f,g,h)
#else // !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectSoundFullDuplex_Initialize(p,a,b,c,d,e,f,g,h)     (p)->Initialize(a,b,c,d,e,f,g,h)
#endif // !defined(__cplusplus) || defined(CINTERFACE)

#endif // _IDirectSoundFullDuplex_

#endif // DIRECTSOUND_VERSION >= 0x0800

//
// Return Codes
//

// The function completed successfully
#define DS_OK                           S_OK

// The call succeeded, but we had to substitute the 3D algorithm
#define DS_NO_VIRTUALIZATION            MAKE_HRESULT(0, _FACDS, 10)

// The call succeeded, but not all of the optional effects were obtained.
#define DS_INCOMPLETE                   MAKE_HRESULT(0, _FACDS, 20)

// The call failed because resources (such as a priority level)
// were already being used by another caller
#define DSERR_ALLOCATED                 MAKE_DSHRESULT(10)

// The control (vol, pan, etc.) requested by the caller is not available
#define DSERR_CONTROLUNAVAIL            MAKE_DSHRESULT(30)

// An invalid parameter was passed to the returning function
#define DSERR_INVALIDPARAM              E_INVALIDARG

// This call is not valid for the current state of this object
#define DSERR_INVALIDCALL               MAKE_DSHRESULT(50)

// An undetermined error occurred inside the DirectSound subsystem
#define DSERR_GENERIC                   E_FAIL

// The caller does not have the priority level required for the function to
// succeed
#define DSERR_PRIOLEVELNEEDED           MAKE_DSHRESULT(70)

// Not enough free memory is available to complete the operation
#define DSERR_OUTOFMEMORY               E_OUTOFMEMORY

// The specified WAVE format is not supported
#define DSERR_BADFORMAT                 MAKE_DSHRESULT(100)

// The function called is not supported at this time
#define DSERR_UNSUPPORTED               E_NOTIMPL

// No sound driver is available for use
#define DSERR_NODRIVER                  MAKE_DSHRESULT(120)

// This object is already initialized
#define DSERR_ALREADYINITIALIZED        MAKE_DSHRESULT(130)

// This object does not support aggregation
#define DSERR_NOAGGREGATION             CLASS_E_NOAGGREGATION

// The buffer memory has been lost, and must be restored
#define DSERR_BUFFERLOST                MAKE_DSHRESULT(150)

// Another app has a higher priority level, preventing this call from
// succeeding
#define DSERR_OTHERAPPHASPRIO           MAKE_DSHRESULT(160)

// This object has not been initialized
#define DSERR_UNINITIALIZED             MAKE_DSHRESULT(170)

// The requested COM interface is not available
#define DSERR_NOINTERFACE               E_NOINTERFACE

// Access is denied
#define DSERR_ACCESSDENIED              E_ACCESSDENIED

// Tried to create a DSBCAPS_CTRLFX buffer shorter than DSBSIZE_FX_MIN milliseconds
#define DSERR_BUFFERTOOSMALL            MAKE_DSHRESULT(180)

// Attempt to use DirectSound 8 functionality on an older DirectSound object
#define DSERR_DS8_REQUIRED              MAKE_DSHRESULT(190)

// A circular loop of send effects was detected
#define DSERR_SENDLOOP                  MAKE_DSHRESULT(200)

// The GUID specified in an audiopath file does not match a valid MIXIN buffer
#define DSERR_BADSENDBUFFERGUID         MAKE_DSHRESULT(210)

// The object requested was not found (numerically equal to DMUS_E_NOT_FOUND)
#define DSERR_OBJECTNOTFOUND            MAKE_DSHRESULT(4449)

// The effects requested could not be found on the system, or they were found
// but in the wrong order, or in the wrong hardware/software locations.
#define DSERR_FXUNAVAILABLE             MAKE_DSHRESULT(220)

//
// Flags
//

#define DSCAPS_PRIMARYMONO          0x00000001
#define DSCAPS_PRIMARYSTEREO        0x00000002
#define DSCAPS_PRIMARY8BIT          0x00000004
#define DSCAPS_PRIMARY16BIT         0x00000008
#define DSCAPS_CONTINUOUSRATE       0x00000010
#define DSCAPS_EMULDRIVER           0x00000020
#define DSCAPS_CERTIFIED            0x00000040
#define DSCAPS_SECONDARYMONO        0x00000100
#define DSCAPS_SECONDARYSTEREO      0x00000200
#define DSCAPS_SECONDARY8BIT        0x00000400
#define DSCAPS_SECONDARY16BIT       0x00000800

#define DSSCL_NORMAL                0x00000001
#define DSSCL_PRIORITY              0x00000002
#define DSSCL_EXCLUSIVE             0x00000003
#define DSSCL_WRITEPRIMARY          0x00000004

#define DSSPEAKER_DIRECTOUT         0x00000000
#define DSSPEAKER_HEADPHONE         0x00000001
#define DSSPEAKER_MONO              0x00000002
#define DSSPEAKER_QUAD              0x00000003
#define DSSPEAKER_STEREO            0x00000004
#define DSSPEAKER_SURROUND          0x00000005
#define DSSPEAKER_5POINT1           0x00000006
#define DSSPEAKER_7POINT1           0x00000007

#define DSSPEAKER_GEOMETRY_MIN      0x00000005  //   5 degrees
#define DSSPEAKER_GEOMETRY_NARROW   0x0000000A  //  10 degrees
#define DSSPEAKER_GEOMETRY_WIDE     0x00000014  //  20 degrees
#define DSSPEAKER_GEOMETRY_MAX      0x000000B4  // 180 degrees

#define DSSPEAKER_COMBINED(c, g)    ((DWORD)(((BYTE)(c)) | ((DWORD)((BYTE)(g))) << 16))
#define DSSPEAKER_CONFIG(a)         ((BYTE)(a))
#define DSSPEAKER_GEOMETRY(a)       ((BYTE)(((DWORD)(a) >> 16) & 0x00FF))

#define DSBCAPS_PRIMARYBUFFER       0x00000001
#define DSBCAPS_STATIC              0x00000002
#define DSBCAPS_LOCHARDWARE         0x00000004
#define DSBCAPS_LOCSOFTWARE         0x00000008
#define DSBCAPS_CTRL3D              0x00000010
#define DSBCAPS_CTRLFREQUENCY       0x00000020
#define DSBCAPS_CTRLPAN             0x00000040
#define DSBCAPS_CTRLVOLUME          0x00000080
#define DSBCAPS_CTRLPOSITIONNOTIFY  0x00000100
#define DSBCAPS_CTRLFX              0x00000200
#define DSBCAPS_STICKYFOCUS         0x00004000
#define DSBCAPS_GLOBALFOCUS         0x00008000
#define DSBCAPS_GETCURRENTPOSITION2 0x00010000
#define DSBCAPS_MUTE3DATMAXDISTANCE 0x00020000
#define DSBCAPS_LOCDEFER            0x00040000

#define DSBPLAY_LOOPING             0x00000001
#define DSBPLAY_LOCHARDWARE         0x00000002
#define DSBPLAY_LOCSOFTWARE         0x00000004
#define DSBPLAY_TERMINATEBY_TIME    0x00000008
#define DSBPLAY_TERMINATEBY_DISTANCE    0x000000010
#define DSBPLAY_TERMINATEBY_PRIORITY    0x000000020

#define DSBSTATUS_PLAYING           0x00000001
#define DSBSTATUS_BUFFERLOST        0x00000002
#define DSBSTATUS_LOOPING           0x00000004
#define DSBSTATUS_LOCHARDWARE       0x00000008
#define DSBSTATUS_LOCSOFTWARE       0x00000010
#define DSBSTATUS_TERMINATED        0x00000020

#define DSBLOCK_FROMWRITECURSOR     0x00000001
#define DSBLOCK_ENTIREBUFFER        0x00000002

#define DSBFREQUENCY_MIN            100
#define DSBFREQUENCY_MAX            100000
#define DSBFREQUENCY_ORIGINAL       0

#define DSBPAN_LEFT                 -10000
#define DSBPAN_CENTER               0
#define DSBPAN_RIGHT                10000

#define DSBVOLUME_MIN               -10000
#define DSBVOLUME_MAX               0

#define DSBSIZE_MIN                 4
#define DSBSIZE_MAX                 0x0FFFFFFF
#define DSBSIZE_FX_MIN              150  // NOTE: Milliseconds, not bytes

#define DS3DMODE_NORMAL             0x00000000
#define DS3DMODE_HEADRELATIVE       0x00000001
#define DS3DMODE_DISABLE            0x00000002

#define DS3D_IMMEDIATE              0x00000000
#define DS3D_DEFERRED               0x00000001

#define DS3D_MINDISTANCEFACTOR      FLT_MIN
#define DS3D_MAXDISTANCEFACTOR      FLT_MAX
#define DS3D_DEFAULTDISTANCEFACTOR  1.0f

#define DS3D_MINROLLOFFFACTOR       0.0f
#define DS3D_MAXROLLOFFFACTOR       10.0f
#define DS3D_DEFAULTROLLOFFFACTOR   1.0f

#define DS3D_MINDOPPLERFACTOR       0.0f
#define DS3D_MAXDOPPLERFACTOR       10.0f
#define DS3D_DEFAULTDOPPLERFACTOR   1.0f

#define DS3D_DEFAULTMINDISTANCE     1.0f
#define DS3D_DEFAULTMAXDISTANCE     1000000000.0f

#define DS3D_MINCONEANGLE           0
#define DS3D_MAXCONEANGLE           360
#define DS3D_DEFAULTCONEANGLE       360

#define DS3D_DEFAULTCONEOUTSIDEVOLUME DSBVOLUME_MAX

// IDirectSoundCapture attributes

#define DSCCAPS_EMULDRIVER          DSCAPS_EMULDRIVER
#define DSCCAPS_CERTIFIED           DSCAPS_CERTIFIED

// IDirectSoundCaptureBuffer attributes

#define DSCBCAPS_WAVEMAPPED         0x80000000

#if DIRECTSOUND_VERSION >= 0x0800
#define DSCBCAPS_CTRLFX             0x00000200
#endif


#define DSCBLOCK_ENTIREBUFFER       0x00000001

#define DSCBSTATUS_CAPTURING        0x00000001
#define DSCBSTATUS_LOOPING          0x00000002

#define DSCBSTART_LOOPING           0x00000001

#define DSBPN_OFFSETSTOP            0xFFFFFFFF

#define DS_CERTIFIED                0x00000000
#define DS_UNCERTIFIED              0x00000001


//
// I3DL2 Material Presets
//

enum
{
    DSFX_I3DL2_MATERIAL_PRESET_SINGLEWINDOW,
    DSFX_I3DL2_MATERIAL_PRESET_DOUBLEWINDOW,
    DSFX_I3DL2_MATERIAL_PRESET_THINDOOR,
    DSFX_I3DL2_MATERIAL_PRESET_THICKDOOR,
    DSFX_I3DL2_MATERIAL_PRESET_WOODWALL,
    DSFX_I3DL2_MATERIAL_PRESET_BRICKWALL,
    DSFX_I3DL2_MATERIAL_PRESET_STONEWALL,
    DSFX_I3DL2_MATERIAL_PRESET_CURTAIN
};

#define I3DL2_MATERIAL_PRESET_SINGLEWINDOW    -2800,0.71f
#define I3DL2_MATERIAL_PRESET_DOUBLEWINDOW    -5000,0.40f
#define I3DL2_MATERIAL_PRESET_THINDOOR        -1800,0.66f
#define I3DL2_MATERIAL_PRESET_THICKDOOR       -4400,0.64f
#define I3DL2_MATERIAL_PRESET_WOODWALL        -4000,0.50f
#define I3DL2_MATERIAL_PRESET_BRICKWALL       -5000,0.60f
#define I3DL2_MATERIAL_PRESET_STONEWALL       -6000,0.68f
#define I3DL2_MATERIAL_PRESET_CURTAIN         -1200,0.15f


enum
{
    DSFX_I3DL2_ENVIRONMENT_PRESET_DEFAULT,
    DSFX_I3DL2_ENVIRONMENT_PRESET_GENERIC,
    DSFX_I3DL2_ENVIRONMENT_PRESET_PADDEDCELL,
    DSFX_I3DL2_ENVIRONMENT_PRESET_ROOM,
    DSFX_I3DL2_ENVIRONMENT_PRESET_BATHROOM,
    DSFX_I3DL2_ENVIRONMENT_PRESET_LIVINGROOM,
    DSFX_I3DL2_ENVIRONMENT_PRESET_STONEROOM,
    DSFX_I3DL2_ENVIRONMENT_PRESET_AUDITORIUM,
    DSFX_I3DL2_ENVIRONMENT_PRESET_CONCERTHALL,
    DSFX_I3DL2_ENVIRONMENT_PRESET_CAVE,
    DSFX_I3DL2_ENVIRONMENT_PRESET_ARENA,
    DSFX_I3DL2_ENVIRONMENT_PRESET_HANGAR,
    DSFX_I3DL2_ENVIRONMENT_PRESET_CARPETEDHALLWAY,
    DSFX_I3DL2_ENVIRONMENT_PRESET_HALLWAY,
    DSFX_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR,
    DSFX_I3DL2_ENVIRONMENT_PRESET_ALLEY,
    DSFX_I3DL2_ENVIRONMENT_PRESET_FOREST,
    DSFX_I3DL2_ENVIRONMENT_PRESET_CITY,
    DSFX_I3DL2_ENVIRONMENT_PRESET_MOUNTAINS,
    DSFX_I3DL2_ENVIRONMENT_PRESET_QUARRY,
    DSFX_I3DL2_ENVIRONMENT_PRESET_PLAIN,
    DSFX_I3DL2_ENVIRONMENT_PRESET_PARKINGLOT,
    DSFX_I3DL2_ENVIRONMENT_PRESET_SEWERPIPE,
    DSFX_I3DL2_ENVIRONMENT_PRESET_UNDERWATER,
    DSFX_I3DL2_ENVIRONMENT_PRESET_SMALLROOM,
    DSFX_I3DL2_ENVIRONMENT_PRESET_MEDIUMROOM,
    DSFX_I3DL2_ENVIRONMENT_PRESET_LARGEROOM,
    DSFX_I3DL2_ENVIRONMENT_PRESET_MEDIUMHALL,
    DSFX_I3DL2_ENVIRONMENT_PRESET_LARGEHALL,
    DSFX_I3DL2_ENVIRONMENT_PRESET_PLATE
};

//
// I3DL2 Reverberation Presets Values
//

#define I3DL2_ENVIRONMENT_PRESET_DEFAULT         -1000, -100, 0.0f, 1.49f, 0.83f, -2602, 0.007f,   200, 0.011f, 100.0f, 100.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_GENERIC         -1000, -100, 0.0f, 1.49f, 0.83f, -2602, 0.007f,   200, 0.011f, 100.0f, 100.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_PADDEDCELL      -1000,-6000, 0.0f, 0.17f, 0.10f, -1204, 0.001f,   207, 0.002f, 100.0f, 100.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_ROOM            -1000, -454, 0.0f, 0.40f, 0.83f, -1646, 0.002f,    53, 0.003f, 100.0f, 100.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_BATHROOM        -1000,-1200, 0.0f, 1.49f, 0.54f,  -370, 0.007f,  1030, 0.011f, 100.0f,  60.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_LIVINGROOM      -1000,-6000, 0.0f, 0.50f, 0.10f, -1376, 0.003f, -1104, 0.004f, 100.0f, 100.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_STONEROOM       -1000, -300, 0.0f, 2.31f, 0.64f,  -711, 0.012f,    83, 0.017f, 100.0f, 100.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_AUDITORIUM      -1000, -476, 0.0f, 4.32f, 0.59f,  -789, 0.020f,  -289, 0.030f, 100.0f, 100.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_CONCERTHALL     -1000, -500, 0.0f, 3.92f, 0.70f, -1230, 0.020f,    -2, 0.029f, 100.0f, 100.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_CAVE            -1000,    0, 0.0f, 2.91f, 1.30f,  -602, 0.015f,  -302, 0.022f, 100.0f, 100.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_ARENA           -1000, -698, 0.0f, 7.24f, 0.33f, -1166, 0.020f,    16, 0.030f, 100.0f, 100.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_HANGAR          -1000,-1000, 0.0f,10.05f, 0.23f,  -602, 0.020f,   198, 0.030f, 100.0f, 100.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_CARPETEDHALLWAY -1000,-4000, 0.0f, 0.30f, 0.10f, -1831, 0.002f, -1630, 0.030f, 100.0f, 100.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_HALLWAY         -1000, -300, 0.0f, 1.49f, 0.59f, -1219, 0.007f,   441, 0.011f, 100.0f, 100.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR   -1000, -237, 0.0f, 2.70f, 0.79f, -1214, 0.013f,   395, 0.020f, 100.0f, 100.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_ALLEY           -1000, -270, 0.0f, 1.49f, 0.86f, -1204, 0.007f,    -4, 0.011f, 100.0f, 100.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_FOREST          -1000,-3300, 0.0f, 1.49f, 0.54f, -2560, 0.162f,  -613, 0.088f,  79.0f, 100.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_CITY            -1000, -800, 0.0f, 1.49f, 0.67f, -2273, 0.007f, -2217, 0.011f,  50.0f, 100.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_MOUNTAINS       -1000,-2500, 0.0f, 1.49f, 0.21f, -2780, 0.300f, -2014, 0.100f,  27.0f, 100.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_QUARRY          -1000,-1000, 0.0f, 1.49f, 0.83f,-10000, 0.061f,   500, 0.025f, 100.0f, 100.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_PLAIN           -1000,-2000, 0.0f, 1.49f, 0.50f, -2466, 0.179f, -2514, 0.100f,  21.0f, 100.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_PARKINGLOT      -1000,    0, 0.0f, 1.65f, 1.50f, -1363, 0.008f, -1153, 0.012f, 100.0f, 100.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_SEWERPIPE       -1000,-1000, 0.0f, 2.81f, 0.14f,   429, 0.014f,   648, 0.021f,  80.0f,  60.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_UNDERWATER      -1000,-4000, 0.0f, 1.49f, 0.10f,  -449, 0.007f,  1700, 0.011f, 100.0f, 100.0f, 5000.0f

//
// Examples simulating 'musical' reverb presets
//
// Name       Decay time   Description
// Small Room    1.1s      A small size room with a length of 5m or so.
// Medium Room   1.3s      A medium size room with a length of 10m or so.
// Large Room    1.5s      A large size room suitable for live performances.
// Medium Hall   1.8s      A medium size concert hall.
// Large Hall    1.8s      A large size concert hall suitable for a full orchestra.
// Plate         1.3s      A plate reverb simulation.
//

#define I3DL2_ENVIRONMENT_PRESET_SMALLROOM       -1000, -600, 0.0f, 1.10f, 0.83f,  -400, 0.005f,   500, 0.010f, 100.0f, 100.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_MEDIUMROOM      -1000, -600, 0.0f, 1.30f, 0.83f, -1000, 0.010f,  -200, 0.020f, 100.0f, 100.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_LARGEROOM       -1000, -600, 0.0f, 1.50f, 0.83f, -1600, 0.020f, -1000, 0.040f, 100.0f, 100.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_MEDIUMHALL      -1000, -600, 0.0f, 1.80f, 0.70f, -1300, 0.015f,  -800, 0.030f, 100.0f, 100.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_LARGEHALL       -1000, -600, 0.0f, 1.80f, 0.70f, -2000, 0.030f, -1400, 0.060f, 100.0f, 100.0f, 5000.0f
#define I3DL2_ENVIRONMENT_PRESET_PLATE           -1000, -200, 0.0f, 1.30f, 0.90f,     0, 0.002f,     0, 0.010f, 100.0f,  75.0f, 5000.0f

//
// DirectSound3D Algorithms
//

// Default DirectSound3D algorithm {00000000-0000-0000-0000-000000000000}
#define DS3DALG_DEFAULT GUID_NULL

// No virtualization (Pan3D) {C241333F-1C1B-11d2-94F5-00C04FC28ACA}
DEFINE_GUID(DS3DALG_NO_VIRTUALIZATION, 0xc241333f, 0x1c1b, 0x11d2, 0x94, 0xf5, 0x0, 0xc0, 0x4f, 0xc2, 0x8a, 0xca);

// High-quality HRTF algorithm {C2413340-1C1B-11d2-94F5-00C04FC28ACA}
DEFINE_GUID(DS3DALG_HRTF_FULL, 0xc2413340, 0x1c1b, 0x11d2, 0x94, 0xf5, 0x0, 0xc0, 0x4f, 0xc2, 0x8a, 0xca);

// Lower-quality HRTF algorithm {C2413342-1C1B-11d2-94F5-00C04FC28ACA}
DEFINE_GUID(DS3DALG_HRTF_LIGHT, 0xc2413342, 0x1c1b, 0x11d2, 0x94, 0xf5, 0x0, 0xc0, 0x4f, 0xc2, 0x8a, 0xca);


#if DIRECTSOUND_VERSION >= 0x0800

//
// DirectSound Internal Effect Algorithms
//


// Gargle {DAFD8210-5711-4B91-9FE3-F75B7AE279BF}
DEFINE_GUID(GUID_DSFX_STANDARD_GARGLE, 0xdafd8210, 0x5711, 0x4b91, 0x9f, 0xe3, 0xf7, 0x5b, 0x7a, 0xe2, 0x79, 0xbf);

// Chorus {EFE6629C-81F7-4281-BD91-C9D604A95AF6}
DEFINE_GUID(GUID_DSFX_STANDARD_CHORUS, 0xefe6629c, 0x81f7, 0x4281, 0xbd, 0x91, 0xc9, 0xd6, 0x04, 0xa9, 0x5a, 0xf6);

// Flanger {EFCA3D92-DFD8-4672-A603-7420894BAD98}
DEFINE_GUID(GUID_DSFX_STANDARD_FLANGER, 0xefca3d92, 0xdfd8, 0x4672, 0xa6, 0x03, 0x74, 0x20, 0x89, 0x4b, 0xad, 0x98);

// Echo/Delay {EF3E932C-D40B-4F51-8CCF-3F98F1B29D5D}
DEFINE_GUID(GUID_DSFX_STANDARD_ECHO, 0xef3e932c, 0xd40b, 0x4f51, 0x8c, 0xcf, 0x3f, 0x98, 0xf1, 0xb2, 0x9d, 0x5d);

// Distortion {EF114C90-CD1D-484E-96E5-09CFAF912A21}
DEFINE_GUID(GUID_DSFX_STANDARD_DISTORTION, 0xef114c90, 0xcd1d, 0x484e, 0x96, 0xe5, 0x09, 0xcf, 0xaf, 0x91, 0x2a, 0x21);

// Compressor/Limiter {EF011F79-4000-406D-87AF-BFFB3FC39D57}
DEFINE_GUID(GUID_DSFX_STANDARD_COMPRESSOR, 0xef011f79, 0x4000, 0x406d, 0x87, 0xaf, 0xbf, 0xfb, 0x3f, 0xc3, 0x9d, 0x57);

// Parametric Equalization {120CED89-3BF4-4173-A132-3CB406CF3231}
DEFINE_GUID(GUID_DSFX_STANDARD_PARAMEQ, 0x120ced89, 0x3bf4, 0x4173, 0xa1, 0x32, 0x3c, 0xb4, 0x06, 0xcf, 0x32, 0x31);


// I3DL2 Environmental Reverberation: Reverb (Listener) Effect {EF985E71-D5C7-42D4-BA4D-2D073E2E96F4}
DEFINE_GUID(GUID_DSFX_STANDARD_I3DL2REVERB, 0xef985e71, 0xd5c7, 0x42d4, 0xba, 0x4d, 0x2d, 0x07, 0x3e, 0x2e, 0x96, 0xf4);

// Waves Reverberation {87FC0268-9A55-4360-95AA-004A1D9DE26C}
DEFINE_GUID(GUID_DSFX_WAVES_REVERB, 0x87fc0268, 0x9a55, 0x4360, 0x95, 0xaa, 0x00, 0x4a, 0x1d, 0x9d, 0xe2, 0x6c);

//
// DirectSound Capture Effect Algorithms
//


// Acoustic Echo Canceller {BF963D80-C559-11D0-8A2B-00A0C9255AC1}
// Matches KSNODETYPE_ACOUSTIC_ECHO_CANCEL in ksmedia.h
DEFINE_GUID(GUID_DSCFX_CLASS_AEC, 0xBF963D80L, 0xC559, 0x11D0, 0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1);

// Microsoft AEC {CDEBB919-379A-488a-8765-F53CFD36DE40}
DEFINE_GUID(GUID_DSCFX_MS_AEC, 0xcdebb919, 0x379a, 0x488a, 0x87, 0x65, 0xf5, 0x3c, 0xfd, 0x36, 0xde, 0x40);

// System AEC {1C22C56D-9879-4f5b-A389-27996DDC2810}
DEFINE_GUID(GUID_DSCFX_SYSTEM_AEC, 0x1c22c56d, 0x9879, 0x4f5b, 0xa3, 0x89, 0x27, 0x99, 0x6d, 0xdc, 0x28, 0x10);

// Noise Supression {E07F903F-62FD-4e60-8CDD-DEA7236665B5}
// Matches KSNODETYPE_NOISE_SUPPRESS in post Windows ME DDK's ksmedia.h
DEFINE_GUID(GUID_DSCFX_CLASS_NS, 0xe07f903f, 0x62fd, 0x4e60, 0x8c, 0xdd, 0xde, 0xa7, 0x23, 0x66, 0x65, 0xb5);

// Microsoft Noise Suppresion {11C5C73B-66E9-4ba1-A0BA-E814C6EED92D}
DEFINE_GUID(GUID_DSCFX_MS_NS, 0x11c5c73b, 0x66e9, 0x4ba1, 0xa0, 0xba, 0xe8, 0x14, 0xc6, 0xee, 0xd9, 0x2d);

// System Noise Suppresion {5AB0882E-7274-4516-877D-4EEE99BA4FD0}
DEFINE_GUID(GUID_DSCFX_SYSTEM_NS, 0x5ab0882e, 0x7274, 0x4516, 0x87, 0x7d, 0x4e, 0xee, 0x99, 0xba, 0x4f, 0xd0);

#endif // DIRECTSOUND_VERSION >= 0x0800

#endif // __DSOUND_INCLUDED__



#ifdef __cplusplus
};
#endif // __cplusplus


