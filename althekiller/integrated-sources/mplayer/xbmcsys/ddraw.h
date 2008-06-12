/*==========================================================================;
 *
 *  Copyright (C) Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddraw.h
 *  Content:    DirectDraw include file
 *
 ***************************************************************************/

#ifndef __DDRAW_INCLUDED__
#define __DDRAW_INCLUDED__

//Disable the nameless union warning when building internally
#undef ENABLE_NAMELESS_UNION_PRAGMA
#ifdef DIRECTX_REDIST
#define ENABLE_NAMELESS_UNION_PRAGMA
#endif

#ifdef ENABLE_NAMELESS_UNION_PRAGMA
#pragma warning(disable:4201)
#endif

/*
 * If you wish an application built against the newest version of DirectDraw
 * to run against an older DirectDraw run time then define DIRECTDRAW_VERSION
 * to be the earlies version of DirectDraw you wish to run against. For,
 * example if you wish an application to run against a DX 3 runtime define
 * DIRECTDRAW_VERSION to be 0x0300.
 */
#ifndef   DIRECTDRAW_VERSION
#define   DIRECTDRAW_VERSION 0x0700
#endif /* DIRECTDRAW_VERSION */

#if defined( _WIN32 )  && !defined( _NO_COM )
#define COM_NO_WINDOWS_H
#include <objbase.h>
#else
#define IUnknown            void
#if !defined( NT_BUILD_ENVIRONMENT ) && !defined(WINNT)
        #define CO_E_NOTINITIALIZED 0x800401F0L
#endif
#endif

#define _FACDD  0x876
#define MAKE_DDHRESULT( code )  MAKE_HRESULT( 1, _FACDD, code )

#ifdef __cplusplus
extern "C" {
#endif

//
// For compilers that don't support nameless unions, do a
//
// #define NONAMELESSUNION
//
// before #include <ddraw.h>
//
#ifndef DUMMYUNIONNAMEN
#if defined(__cplusplus) || !defined(NONAMELESSUNION)
#define DUMMYUNIONNAMEN(n)
#else
#define DUMMYUNIONNAMEN(n)      u##n
#endif
#endif

#ifndef MAKEFOURCC
    #define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |   \
                ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#endif //defined(MAKEFOURCC)

/*
 * FOURCC codes for DX compressed-texture pixel formats
 */
#define FOURCC_DXT1  (MAKEFOURCC('D','X','T','1'))
#define FOURCC_DXT2  (MAKEFOURCC('D','X','T','2'))
#define FOURCC_DXT3  (MAKEFOURCC('D','X','T','3'))
#define FOURCC_DXT4  (MAKEFOURCC('D','X','T','4'))
#define FOURCC_DXT5  (MAKEFOURCC('D','X','T','5'))

/*
 * GUIDS used by DirectDraw objects
 */
#if defined( _WIN32 ) && !defined( _NO_COM )

DEFINE_GUID( CLSID_DirectDraw,                  0xD7B70EE0,0x4340,0x11CF,0xB0,0x63,0x00,0x20,0xAF,0xC2,0xCD,0x35 );
DEFINE_GUID( CLSID_DirectDraw7,                 0x3c305196,0x50db,0x11d3,0x9c,0xfe,0x00,0xc0,0x4f,0xd9,0x30,0xc5 );
DEFINE_GUID( CLSID_DirectDrawClipper,           0x593817A0,0x7DB3,0x11CF,0xA2,0xDE,0x00,0xAA,0x00,0xb9,0x33,0x56 );
DEFINE_GUID( IID_IDirectDraw,                   0x6C14DB80,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
DEFINE_GUID( IID_IDirectDraw2,                  0xB3A6F3E0,0x2B43,0x11CF,0xA2,0xDE,0x00,0xAA,0x00,0xB9,0x33,0x56 );
DEFINE_GUID( IID_IDirectDraw4,                  0x9c59509a,0x39bd,0x11d1,0x8c,0x4a,0x00,0xc0,0x4f,0xd9,0x30,0xc5 );
DEFINE_GUID( IID_IDirectDraw7,                  0x15e65ec0,0x3b9c,0x11d2,0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b );
DEFINE_GUID( IID_IDirectDrawSurface,            0x6C14DB81,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
DEFINE_GUID( IID_IDirectDrawSurface2,           0x57805885,0x6eec,0x11cf,0x94,0x41,0xa8,0x23,0x03,0xc1,0x0e,0x27 );
DEFINE_GUID( IID_IDirectDrawSurface3,           0xDA044E00,0x69B2,0x11D0,0xA1,0xD5,0x00,0xAA,0x00,0xB8,0xDF,0xBB );
DEFINE_GUID( IID_IDirectDrawSurface4,           0x0B2B8630,0xAD35,0x11D0,0x8E,0xA6,0x00,0x60,0x97,0x97,0xEA,0x5B );
DEFINE_GUID( IID_IDirectDrawSurface7,           0x06675a80,0x3b9b,0x11d2,0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b );
DEFINE_GUID( IID_IDirectDrawPalette,            0x6C14DB84,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
DEFINE_GUID( IID_IDirectDrawClipper,            0x6C14DB85,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
DEFINE_GUID( IID_IDirectDrawColorControl,       0x4B9F0EE0,0x0D7E,0x11D0,0x9B,0x06,0x00,0xA0,0xC9,0x03,0xA3,0xB8 );
DEFINE_GUID( IID_IDirectDrawGammaControl,       0x69C11C3E,0xB46B,0x11D1,0xAD,0x7A,0x00,0xC0,0x4F,0xC2,0x9B,0x4E );

#endif

/*============================================================================
 *
 * DirectDraw Structures
 *
 * Various structures used to invoke DirectDraw.
 *
 *==========================================================================*/

struct IDirectDraw;
struct IDirectDrawSurface;
struct IDirectDrawPalette;
struct IDirectDrawClipper;

typedef struct IDirectDraw              FAR *LPDIRECTDRAW;
typedef struct IDirectDraw2             FAR *LPDIRECTDRAW2;
typedef struct IDirectDraw4             FAR *LPDIRECTDRAW4;
typedef struct IDirectDraw7             FAR *LPDIRECTDRAW7;
typedef struct IDirectDrawSurface       FAR *LPDIRECTDRAWSURFACE;
typedef struct IDirectDrawSurface2      FAR *LPDIRECTDRAWSURFACE2;
typedef struct IDirectDrawSurface3      FAR *LPDIRECTDRAWSURFACE3;
typedef struct IDirectDrawSurface4      FAR *LPDIRECTDRAWSURFACE4;
typedef struct IDirectDrawSurface7      FAR *LPDIRECTDRAWSURFACE7;
typedef struct IDirectDrawPalette               FAR *LPDIRECTDRAWPALETTE;
typedef struct IDirectDrawClipper               FAR *LPDIRECTDRAWCLIPPER;
typedef struct IDirectDrawColorControl          FAR *LPDIRECTDRAWCOLORCONTROL;
typedef struct IDirectDrawGammaControl          FAR *LPDIRECTDRAWGAMMACONTROL;

typedef struct _DDFXROP                 FAR *LPDDFXROP;
typedef struct _DDSURFACEDESC           FAR *LPDDSURFACEDESC;
typedef struct _DDSURFACEDESC2          FAR *LPDDSURFACEDESC2;
typedef struct _DDCOLORCONTROL          FAR *LPDDCOLORCONTROL;

/*
 * API's
 */
#if (defined (WIN32) || defined( _WIN32 ) ) && !defined( _NO_COM )
//#if defined( _WIN32 ) && !defined( _NO_ENUM )
    typedef BOOL (FAR PASCAL * LPDDENUMCALLBACKA)(GUID FAR *, LPSTR, LPSTR, LPVOID);
    typedef BOOL (FAR PASCAL * LPDDENUMCALLBACKW)(GUID FAR *, LPWSTR, LPWSTR, LPVOID);
    extern HRESULT WINAPI DirectDrawEnumerateW( LPDDENUMCALLBACKW lpCallback, LPVOID lpContext );
    extern HRESULT WINAPI DirectDrawEnumerateA( LPDDENUMCALLBACKA lpCallback, LPVOID lpContext );
    /*
     * Protect against old SDKs
     */
    #if !defined(HMONITOR_DECLARED) && (WINVER < 0x0500)
        #define HMONITOR_DECLARED
        DECLARE_HANDLE(HMONITOR);
    #endif
    typedef BOOL (FAR PASCAL * LPDDENUMCALLBACKEXA)(GUID FAR *, LPSTR, LPSTR, LPVOID, HMONITOR);
    typedef BOOL (FAR PASCAL * LPDDENUMCALLBACKEXW)(GUID FAR *, LPWSTR, LPWSTR, LPVOID, HMONITOR);
    extern HRESULT WINAPI DirectDrawEnumerateExW( LPDDENUMCALLBACKEXW lpCallback, LPVOID lpContext, DWORD dwFlags);
    extern HRESULT WINAPI DirectDrawEnumerateExA( LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags);
    typedef HRESULT (WINAPI * LPDIRECTDRAWENUMERATEEXA)( LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags);
    typedef HRESULT (WINAPI * LPDIRECTDRAWENUMERATEEXW)( LPDDENUMCALLBACKEXW lpCallback, LPVOID lpContext, DWORD dwFlags);

    #ifdef UNICODE
        typedef LPDDENUMCALLBACKW           LPDDENUMCALLBACK;
        #define DirectDrawEnumerate         DirectDrawEnumerateW
        typedef LPDDENUMCALLBACKEXW         LPDDENUMCALLBACKEX;
        typedef LPDIRECTDRAWENUMERATEEXW        LPDIRECTDRAWENUMERATEEX;
        #define DirectDrawEnumerateEx       DirectDrawEnumerateExW
    #else
        typedef LPDDENUMCALLBACKA           LPDDENUMCALLBACK;
        #define DirectDrawEnumerate         DirectDrawEnumerateA
        typedef LPDDENUMCALLBACKEXA         LPDDENUMCALLBACKEX;
        typedef LPDIRECTDRAWENUMERATEEXA        LPDIRECTDRAWENUMERATEEX;
        #define DirectDrawEnumerateEx       DirectDrawEnumerateExA
    #endif
    extern HRESULT WINAPI DirectDrawCreate( GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter );
    extern HRESULT WINAPI DirectDrawCreateEx( GUID FAR * lpGuid, LPVOID  *lplpDD, REFIID  iid,IUnknown FAR *pUnkOuter );
    extern HRESULT WINAPI DirectDrawCreateClipper( DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter );
#endif
/*
 * Flags for DirectDrawEnumerateEx
 * DirectDrawEnumerateEx supercedes DirectDrawEnumerate. You must use GetProcAddress to
 * obtain a function pointer (of type LPDIRECTDRAWENUMERATEEX) to DirectDrawEnumerateEx.
 * By default, only the primary display device is enumerated.
 * DirectDrawEnumerate is equivalent to DirectDrawEnumerate(,,DDENUM_NONDISPLAYDEVICES)
 */

/*
 * This flag causes enumeration of any GDI display devices which are part of
 * the Windows Desktop
 */
#define DDENUM_ATTACHEDSECONDARYDEVICES     0x00000001L

/*
 * This flag causes enumeration of any GDI display devices which are not
 * part of the Windows Desktop
 */
#define DDENUM_DETACHEDSECONDARYDEVICES     0x00000002L

/*
 * This flag causes enumeration of non-display devices
 */
#define DDENUM_NONDISPLAYDEVICES            0x00000004L


#define REGSTR_KEY_DDHW_DESCRIPTION     "Description"
#define REGSTR_KEY_DDHW_DRIVERNAME      "DriverName"
#define REGSTR_PATH_DDHW                "Hardware\\DirectDrawDrivers"

#define DDCREATE_HARDWAREONLY           0x00000001l
#define DDCREATE_EMULATIONONLY          0x00000002l

#if defined(WINNT) || !defined(WIN32)
typedef long HRESULT;
#endif

//#ifndef WINNT
typedef HRESULT (FAR PASCAL * LPDDENUMMODESCALLBACK)(LPDDSURFACEDESC, LPVOID);
typedef HRESULT (FAR PASCAL * LPDDENUMMODESCALLBACK2)(LPDDSURFACEDESC2, LPVOID);
typedef HRESULT (FAR PASCAL * LPDDENUMSURFACESCALLBACK)(LPDIRECTDRAWSURFACE, LPDDSURFACEDESC, LPVOID);
typedef HRESULT (FAR PASCAL * LPDDENUMSURFACESCALLBACK2)(LPDIRECTDRAWSURFACE4, LPDDSURFACEDESC2, LPVOID);
typedef HRESULT (FAR PASCAL * LPDDENUMSURFACESCALLBACK7)(LPDIRECTDRAWSURFACE7, LPDDSURFACEDESC2, LPVOID);
//#endif

/*
 * Generic pixel format with 8-bit RGB and alpha components
 */
typedef struct _DDARGB
{
    BYTE blue;
    BYTE green;
    BYTE red;
    BYTE alpha;
} DDARGB;

typedef DDARGB FAR *LPDDARGB;

/*
 * This version of the structure remains for backwards source compatibility.
 * The DDARGB structure is the one that should be used for all DirectDraw APIs.
 */
typedef struct _DDRGBA
{
    BYTE red;
    BYTE green;
    BYTE blue;
    BYTE alpha;
} DDRGBA;

typedef DDRGBA FAR *LPDDRGBA;


/*
 * DDCOLORKEY
 */
typedef struct _DDCOLORKEY
{
    DWORD       dwColorSpaceLowValue;   // low boundary of color space that is to
                                        // be treated as Color Key, inclusive
    DWORD       dwColorSpaceHighValue;  // high boundary of color space that is
                                        // to be treated as Color Key, inclusive
} DDCOLORKEY;

typedef DDCOLORKEY FAR* LPDDCOLORKEY;

/*
 * DDBLTFX
 * Used to pass override information to the DIRECTDRAWSURFACE callback Blt.
 */
typedef struct _DDBLTFX
{
    DWORD       dwSize;                         // size of structure
    DWORD       dwDDFX;                         // FX operations
    DWORD       dwROP;                          // Win32 raster operations
    DWORD       dwDDROP;                        // Raster operations new for DirectDraw
    DWORD       dwRotationAngle;                // Rotation angle for blt
    DWORD       dwZBufferOpCode;                // ZBuffer compares
    DWORD       dwZBufferLow;                   // Low limit of Z buffer
    DWORD       dwZBufferHigh;                  // High limit of Z buffer
    DWORD       dwZBufferBaseDest;              // Destination base value
    DWORD       dwZDestConstBitDepth;           // Bit depth used to specify Z constant for destination
    union
    {
        DWORD   dwZDestConst;                   // Constant to use as Z buffer for dest
        LPDIRECTDRAWSURFACE lpDDSZBufferDest;   // Surface to use as Z buffer for dest
    } DUMMYUNIONNAMEN(1);
    DWORD       dwZSrcConstBitDepth;            // Bit depth used to specify Z constant for source
    union
    {
        DWORD   dwZSrcConst;                    // Constant to use as Z buffer for src
        LPDIRECTDRAWSURFACE lpDDSZBufferSrc;    // Surface to use as Z buffer for src
    } DUMMYUNIONNAMEN(2);
    DWORD       dwAlphaEdgeBlendBitDepth;       // Bit depth used to specify constant for alpha edge blend
    DWORD       dwAlphaEdgeBlend;               // Alpha for edge blending
    DWORD       dwReserved;
    DWORD       dwAlphaDestConstBitDepth;       // Bit depth used to specify alpha constant for destination
    union
    {
        DWORD   dwAlphaDestConst;               // Constant to use as Alpha Channel
        LPDIRECTDRAWSURFACE lpDDSAlphaDest;     // Surface to use as Alpha Channel
    } DUMMYUNIONNAMEN(3);
    DWORD       dwAlphaSrcConstBitDepth;        // Bit depth used to specify alpha constant for source
    union
    {
        DWORD   dwAlphaSrcConst;                // Constant to use as Alpha Channel
        LPDIRECTDRAWSURFACE lpDDSAlphaSrc;      // Surface to use as Alpha Channel
    } DUMMYUNIONNAMEN(4);
    union
    {
        DWORD   dwFillColor;                    // color in RGB or Palettized
        DWORD   dwFillDepth;                    // depth value for z-buffer
        DWORD   dwFillPixel;                    // pixel value for RGBA or RGBZ
        LPDIRECTDRAWSURFACE lpDDSPattern;       // Surface to use as pattern
    } DUMMYUNIONNAMEN(5);
    DDCOLORKEY  ddckDestColorkey;               // DestColorkey override
    DDCOLORKEY  ddckSrcColorkey;                // SrcColorkey override
} DDBLTFX;

typedef DDBLTFX FAR* LPDDBLTFX;



/*
 * DDSCAPS
 */
typedef struct _DDSCAPS
{
    DWORD       dwCaps;         // capabilities of surface wanted
} DDSCAPS;

typedef DDSCAPS FAR* LPDDSCAPS;


/*
 * DDOSCAPS
 */
typedef struct _DDOSCAPS
{
    DWORD       dwCaps;         // capabilities of surface wanted
} DDOSCAPS;

typedef DDOSCAPS FAR* LPDDOSCAPS;

/*
 * This structure is used internally by DirectDraw.
 */
typedef struct _DDSCAPSEX
{
    DWORD       dwCaps2;
    DWORD       dwCaps3;
    union
    {
        DWORD       dwCaps4;
        DWORD       dwVolumeDepth;
    } DUMMYUNIONNAMEN(1);
} DDSCAPSEX, FAR * LPDDSCAPSEX;

/*
 * DDSCAPS2
 */
typedef struct _DDSCAPS2
{
    DWORD       dwCaps;         // capabilities of surface wanted
    DWORD       dwCaps2;
    DWORD       dwCaps3;
    union
    {
        DWORD       dwCaps4;
        DWORD       dwVolumeDepth;
    } DUMMYUNIONNAMEN(1);
} DDSCAPS2;

typedef DDSCAPS2 FAR* LPDDSCAPS2;

/*
 * DDCAPS
 */
#define DD_ROP_SPACE            (256/32)        // space required to store ROP array
/*
 * NOTE: Our choosen structure number scheme is to append a single digit to
 * the end of the structure giving the version that structure is associated
 * with.
 */

/*
 * This structure represents the DDCAPS structure released in DirectDraw 1.0.  It is used internally
 * by DirectDraw to interpret caps passed into ddraw by drivers written prior to the release of DirectDraw 2.0.
 * New applications should use the DDCAPS structure defined below.
 */
typedef struct _DDCAPS_DX1
{
    DWORD       dwSize;                 // size of the DDDRIVERCAPS structure
    DWORD       dwCaps;                 // driver specific capabilities
    DWORD       dwCaps2;                // more driver specific capabilites
    DWORD       dwCKeyCaps;             // color key capabilities of the surface
    DWORD       dwFXCaps;               // driver specific stretching and effects capabilites
    DWORD       dwFXAlphaCaps;          // alpha driver specific capabilities
    DWORD       dwPalCaps;              // palette capabilities
    DWORD       dwSVCaps;               // stereo vision capabilities
    DWORD       dwAlphaBltConstBitDepths;       // DDBD_2,4,8
    DWORD       dwAlphaBltPixelBitDepths;       // DDBD_1,2,4,8
    DWORD       dwAlphaBltSurfaceBitDepths;     // DDBD_1,2,4,8
    DWORD       dwAlphaOverlayConstBitDepths;   // DDBD_2,4,8
    DWORD       dwAlphaOverlayPixelBitDepths;   // DDBD_1,2,4,8
    DWORD       dwAlphaOverlaySurfaceBitDepths; // DDBD_1,2,4,8
    DWORD       dwZBufferBitDepths;             // DDBD_8,16,24,32
    DWORD       dwVidMemTotal;          // total amount of video memory
    DWORD       dwVidMemFree;           // amount of free video memory
    DWORD       dwMaxVisibleOverlays;   // maximum number of visible overlays
    DWORD       dwCurrVisibleOverlays;  // current number of visible overlays
    DWORD       dwNumFourCCCodes;       // number of four cc codes
    DWORD       dwAlignBoundarySrc;     // source rectangle alignment
    DWORD       dwAlignSizeSrc;         // source rectangle byte size
    DWORD       dwAlignBoundaryDest;    // dest rectangle alignment
    DWORD       dwAlignSizeDest;        // dest rectangle byte size
    DWORD       dwAlignStrideAlign;     // stride alignment
    DWORD       dwRops[DD_ROP_SPACE];   // ROPS supported
    DDSCAPS     ddsCaps;                // DDSCAPS structure has all the general capabilities
    DWORD       dwMinOverlayStretch;    // minimum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    DWORD       dwMaxOverlayStretch;    // maximum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    DWORD       dwMinLiveVideoStretch;  // OBSOLETE! This field remains for compatability reasons only
    DWORD       dwMaxLiveVideoStretch;  // OBSOLETE! This field remains for compatability reasons only
    DWORD       dwMinHwCodecStretch;    // OBSOLETE! This field remains for compatability reasons only
    DWORD       dwMaxHwCodecStretch;    // OBSOLETE! This field remains for compatability reasons only
    DWORD       dwReserved1;            // reserved
    DWORD       dwReserved2;            // reserved
    DWORD       dwReserved3;            // reserved
} DDCAPS_DX1;

typedef DDCAPS_DX1 FAR* LPDDCAPS_DX1;

/*
 * This structure is the DDCAPS structure as it was in version 2 and 3 of Direct X.
 * It is present for back compatability.
 */
typedef struct _DDCAPS_DX3
{
    DWORD       dwSize;                 // size of the DDDRIVERCAPS structure
    DWORD       dwCaps;                 // driver specific capabilities
    DWORD       dwCaps2;                // more driver specific capabilites
    DWORD       dwCKeyCaps;             // color key capabilities of the surface
    DWORD       dwFXCaps;               // driver specific stretching and effects capabilites
    DWORD       dwFXAlphaCaps;          // alpha driver specific capabilities
    DWORD       dwPalCaps;              // palette capabilities
    DWORD       dwSVCaps;               // stereo vision capabilities
    DWORD       dwAlphaBltConstBitDepths;       // DDBD_2,4,8
    DWORD       dwAlphaBltPixelBitDepths;       // DDBD_1,2,4,8
    DWORD       dwAlphaBltSurfaceBitDepths;     // DDBD_1,2,4,8
    DWORD       dwAlphaOverlayConstBitDepths;   // DDBD_2,4,8
    DWORD       dwAlphaOverlayPixelBitDepths;   // DDBD_1,2,4,8
    DWORD       dwAlphaOverlaySurfaceBitDepths; // DDBD_1,2,4,8
    DWORD       dwZBufferBitDepths;             // DDBD_8,16,24,32
    DWORD       dwVidMemTotal;          // total amount of video memory
    DWORD       dwVidMemFree;           // amount of free video memory
    DWORD       dwMaxVisibleOverlays;   // maximum number of visible overlays
    DWORD       dwCurrVisibleOverlays;  // current number of visible overlays
    DWORD       dwNumFourCCCodes;       // number of four cc codes
    DWORD       dwAlignBoundarySrc;     // source rectangle alignment
    DWORD       dwAlignSizeSrc;         // source rectangle byte size
    DWORD       dwAlignBoundaryDest;    // dest rectangle alignment
    DWORD       dwAlignSizeDest;        // dest rectangle byte size
    DWORD       dwAlignStrideAlign;     // stride alignment
    DWORD       dwRops[DD_ROP_SPACE];   // ROPS supported
    DDSCAPS     ddsCaps;                // DDSCAPS structure has all the general capabilities
    DWORD       dwMinOverlayStretch;    // minimum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    DWORD       dwMaxOverlayStretch;    // maximum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    DWORD       dwMinLiveVideoStretch;  // minimum live video stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    DWORD       dwMaxLiveVideoStretch;  // maximum live video stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    DWORD       dwMinHwCodecStretch;    // minimum hardware codec stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    DWORD       dwMaxHwCodecStretch;    // maximum hardware codec stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    DWORD       dwReserved1;            // reserved
    DWORD       dwReserved2;            // reserved
    DWORD       dwReserved3;            // reserved
    DWORD       dwSVBCaps;              // driver specific capabilities for System->Vmem blts
    DWORD       dwSVBCKeyCaps;          // driver color key capabilities for System->Vmem blts
    DWORD       dwSVBFXCaps;            // driver FX capabilities for System->Vmem blts
    DWORD       dwSVBRops[DD_ROP_SPACE];// ROPS supported for System->Vmem blts
    DWORD       dwVSBCaps;              // driver specific capabilities for Vmem->System blts
    DWORD       dwVSBCKeyCaps;          // driver color key capabilities for Vmem->System blts
    DWORD       dwVSBFXCaps;            // driver FX capabilities for Vmem->System blts
    DWORD       dwVSBRops[DD_ROP_SPACE];// ROPS supported for Vmem->System blts
    DWORD       dwSSBCaps;              // driver specific capabilities for System->System blts
    DWORD       dwSSBCKeyCaps;          // driver color key capabilities for System->System blts
    DWORD       dwSSBFXCaps;            // driver FX capabilities for System->System blts
    DWORD       dwSSBRops[DD_ROP_SPACE];// ROPS supported for System->System blts
    DWORD       dwReserved4;            // reserved
    DWORD       dwReserved5;            // reserved
    DWORD       dwReserved6;            // reserved
} DDCAPS_DX3;
typedef DDCAPS_DX3 FAR* LPDDCAPS_DX3;

/*
 * This structure is the DDCAPS structure as it was in version 5 of Direct X.
 * It is present for back compatability.
 */
typedef struct _DDCAPS_DX5
{
/*  0*/ DWORD   dwSize;                 // size of the DDDRIVERCAPS structure
/*  4*/ DWORD   dwCaps;                 // driver specific capabilities
/*  8*/ DWORD   dwCaps2;                // more driver specific capabilites
/*  c*/ DWORD   dwCKeyCaps;             // color key capabilities of the surface
/* 10*/ DWORD   dwFXCaps;               // driver specific stretching and effects capabilites
/* 14*/ DWORD   dwFXAlphaCaps;          // alpha driver specific capabilities
/* 18*/ DWORD   dwPalCaps;              // palette capabilities
/* 1c*/ DWORD   dwSVCaps;               // stereo vision capabilities
/* 20*/ DWORD   dwAlphaBltConstBitDepths;       // DDBD_2,4,8
/* 24*/ DWORD   dwAlphaBltPixelBitDepths;       // DDBD_1,2,4,8
/* 28*/ DWORD   dwAlphaBltSurfaceBitDepths;     // DDBD_1,2,4,8
/* 2c*/ DWORD   dwAlphaOverlayConstBitDepths;   // DDBD_2,4,8
/* 30*/ DWORD   dwAlphaOverlayPixelBitDepths;   // DDBD_1,2,4,8
/* 34*/ DWORD   dwAlphaOverlaySurfaceBitDepths; // DDBD_1,2,4,8
/* 38*/ DWORD   dwZBufferBitDepths;             // DDBD_8,16,24,32
/* 3c*/ DWORD   dwVidMemTotal;          // total amount of video memory
/* 40*/ DWORD   dwVidMemFree;           // amount of free video memory
/* 44*/ DWORD   dwMaxVisibleOverlays;   // maximum number of visible overlays
/* 48*/ DWORD   dwCurrVisibleOverlays;  // current number of visible overlays
/* 4c*/ DWORD   dwNumFourCCCodes;       // number of four cc codes
/* 50*/ DWORD   dwAlignBoundarySrc;     // source rectangle alignment
/* 54*/ DWORD   dwAlignSizeSrc;         // source rectangle byte size
/* 58*/ DWORD   dwAlignBoundaryDest;    // dest rectangle alignment
/* 5c*/ DWORD   dwAlignSizeDest;        // dest rectangle byte size
/* 60*/ DWORD   dwAlignStrideAlign;     // stride alignment
/* 64*/ DWORD   dwRops[DD_ROP_SPACE];   // ROPS supported
/* 84*/ DDSCAPS ddsCaps;                // DDSCAPS structure has all the general capabilities
/* 88*/ DWORD   dwMinOverlayStretch;    // minimum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
/* 8c*/ DWORD   dwMaxOverlayStretch;    // maximum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
/* 90*/ DWORD   dwMinLiveVideoStretch;  // minimum live video stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
/* 94*/ DWORD   dwMaxLiveVideoStretch;  // maximum live video stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
/* 98*/ DWORD   dwMinHwCodecStretch;    // minimum hardware codec stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
/* 9c*/ DWORD   dwMaxHwCodecStretch;    // maximum hardware codec stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
/* a0*/ DWORD   dwReserved1;            // reserved
/* a4*/ DWORD   dwReserved2;            // reserved
/* a8*/ DWORD   dwReserved3;            // reserved
/* ac*/ DWORD   dwSVBCaps;              // driver specific capabilities for System->Vmem blts
/* b0*/ DWORD   dwSVBCKeyCaps;          // driver color key capabilities for System->Vmem blts
/* b4*/ DWORD   dwSVBFXCaps;            // driver FX capabilities for System->Vmem blts
/* b8*/ DWORD   dwSVBRops[DD_ROP_SPACE];// ROPS supported for System->Vmem blts
/* d8*/ DWORD   dwVSBCaps;              // driver specific capabilities for Vmem->System blts
/* dc*/ DWORD   dwVSBCKeyCaps;          // driver color key capabilities for Vmem->System blts
/* e0*/ DWORD   dwVSBFXCaps;            // driver FX capabilities for Vmem->System blts
/* e4*/ DWORD   dwVSBRops[DD_ROP_SPACE];// ROPS supported for Vmem->System blts
/*104*/ DWORD   dwSSBCaps;              // driver specific capabilities for System->System blts
/*108*/ DWORD   dwSSBCKeyCaps;          // driver color key capabilities for System->System blts
/*10c*/ DWORD   dwSSBFXCaps;            // driver FX capabilities for System->System blts
/*110*/ DWORD   dwSSBRops[DD_ROP_SPACE];// ROPS supported for System->System blts
// Members added for DX5:
/*130*/ DWORD   dwMaxVideoPorts;        // maximum number of usable video ports
/*134*/ DWORD   dwCurrVideoPorts;       // current number of video ports used
/*138*/ DWORD   dwSVBCaps2;             // more driver specific capabilities for System->Vmem blts
/*13c*/ DWORD   dwNLVBCaps;               // driver specific capabilities for non-local->local vidmem blts
/*140*/ DWORD   dwNLVBCaps2;              // more driver specific capabilities non-local->local vidmem blts
/*144*/ DWORD   dwNLVBCKeyCaps;           // driver color key capabilities for non-local->local vidmem blts
/*148*/ DWORD   dwNLVBFXCaps;             // driver FX capabilities for non-local->local blts
/*14c*/ DWORD   dwNLVBRops[DD_ROP_SPACE]; // ROPS supported for non-local->local blts
} DDCAPS_DX5;
typedef DDCAPS_DX5 FAR* LPDDCAPS_DX5;

typedef struct _DDCAPS_DX6
{
/*  0*/ DWORD   dwSize;                 // size of the DDDRIVERCAPS structure
/*  4*/ DWORD   dwCaps;                 // driver specific capabilities
/*  8*/ DWORD   dwCaps2;                // more driver specific capabilites
/*  c*/ DWORD   dwCKeyCaps;             // color key capabilities of the surface
/* 10*/ DWORD   dwFXCaps;               // driver specific stretching and effects capabilites
/* 14*/ DWORD   dwFXAlphaCaps;          // alpha caps
/* 18*/ DWORD   dwPalCaps;              // palette capabilities
/* 1c*/ DWORD   dwSVCaps;               // stereo vision capabilities
/* 20*/ DWORD   dwAlphaBltConstBitDepths;       // DDBD_2,4,8
/* 24*/ DWORD   dwAlphaBltPixelBitDepths;       // DDBD_1,2,4,8
/* 28*/ DWORD   dwAlphaBltSurfaceBitDepths;     // DDBD_1,2,4,8
/* 2c*/ DWORD   dwAlphaOverlayConstBitDepths;   // DDBD_2,4,8
/* 30*/ DWORD   dwAlphaOverlayPixelBitDepths;   // DDBD_1,2,4,8
/* 34*/ DWORD   dwAlphaOverlaySurfaceBitDepths; // DDBD_1,2,4,8
/* 38*/ DWORD   dwZBufferBitDepths;             // DDBD_8,16,24,32
/* 3c*/ DWORD   dwVidMemTotal;          // total amount of video memory
/* 40*/ DWORD   dwVidMemFree;           // amount of free video memory
/* 44*/ DWORD   dwMaxVisibleOverlays;   // maximum number of visible overlays
/* 48*/ DWORD   dwCurrVisibleOverlays;  // current number of visible overlays
/* 4c*/ DWORD   dwNumFourCCCodes;       // number of four cc codes
/* 50*/ DWORD   dwAlignBoundarySrc;     // source rectangle alignment
/* 54*/ DWORD   dwAlignSizeSrc;         // source rectangle byte size
/* 58*/ DWORD   dwAlignBoundaryDest;    // dest rectangle alignment
/* 5c*/ DWORD   dwAlignSizeDest;        // dest rectangle byte size
/* 60*/ DWORD   dwAlignStrideAlign;     // stride alignment
/* 64*/ DWORD   dwRops[DD_ROP_SPACE];   // ROPS supported
/* 84*/ DDSCAPS ddsOldCaps;             // Was DDSCAPS  ddsCaps. ddsCaps is of type DDSCAPS2 for DX6
/* 88*/ DWORD   dwMinOverlayStretch;    // minimum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
/* 8c*/ DWORD   dwMaxOverlayStretch;    // maximum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
/* 90*/ DWORD   dwMinLiveVideoStretch;  // minimum live video stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
/* 94*/ DWORD   dwMaxLiveVideoStretch;  // maximum live video stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
/* 98*/ DWORD   dwMinHwCodecStretch;    // minimum hardware codec stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
/* 9c*/ DWORD   dwMaxHwCodecStretch;    // maximum hardware codec stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
/* a0*/ DWORD   dwReserved1;            // reserved
/* a4*/ DWORD   dwReserved2;            // reserved
/* a8*/ DWORD   dwReserved3;            // reserved
/* ac*/ DWORD   dwSVBCaps;              // driver specific capabilities for System->Vmem blts
/* b0*/ DWORD   dwSVBCKeyCaps;          // driver color key capabilities for System->Vmem blts
/* b4*/ DWORD   dwSVBFXCaps;            // driver FX capabilities for System->Vmem blts
/* b8*/ DWORD   dwSVBRops[DD_ROP_SPACE];// ROPS supported for System->Vmem blts
/* d8*/ DWORD   dwVSBCaps;              // driver specific capabilities for Vmem->System blts
/* dc*/ DWORD   dwVSBCKeyCaps;          // driver color key capabilities for Vmem->System blts
/* e0*/ DWORD   dwVSBFXCaps;            // driver FX capabilities for Vmem->System blts
/* e4*/ DWORD   dwVSBRops[DD_ROP_SPACE];// ROPS supported for Vmem->System blts
/*104*/ DWORD   dwSSBCaps;              // driver specific capabilities for System->System blts
/*108*/ DWORD   dwSSBCKeyCaps;          // driver color key capabilities for System->System blts
/*10c*/ DWORD   dwSSBFXCaps;            // driver FX capabilities for System->System blts
/*110*/ DWORD   dwSSBRops[DD_ROP_SPACE];// ROPS supported for System->System blts
/*130*/ DWORD   dwMaxVideoPorts;        // maximum number of usable video ports
/*134*/ DWORD   dwCurrVideoPorts;       // current number of video ports used
/*138*/ DWORD   dwSVBCaps2;             // more driver specific capabilities for System->Vmem blts
/*13c*/ DWORD   dwNLVBCaps;               // driver specific capabilities for non-local->local vidmem blts
/*140*/ DWORD   dwNLVBCaps2;              // more driver specific capabilities non-local->local vidmem blts
/*144*/ DWORD   dwNLVBCKeyCaps;           // driver color key capabilities for non-local->local vidmem blts
/*148*/ DWORD   dwNLVBFXCaps;             // driver FX capabilities for non-local->local blts
/*14c*/ DWORD   dwNLVBRops[DD_ROP_SPACE]; // ROPS supported for non-local->local blts
// Members added for DX6 release
/*16c*/ DDSCAPS2 ddsCaps;               // Surface Caps
} DDCAPS_DX6;
typedef DDCAPS_DX6 FAR* LPDDCAPS_DX6;

typedef struct _DDCAPS_DX7
{
/*  0*/ DWORD   dwSize;                 // size of the DDDRIVERCAPS structure
/*  4*/ DWORD   dwCaps;                 // driver specific capabilities
/*  8*/ DWORD   dwCaps2;                // more driver specific capabilites
/*  c*/ DWORD   dwCKeyCaps;             // color key capabilities of the surface
/* 10*/ DWORD   dwFXCaps;               // driver specific stretching and effects capabilites
/* 14*/ DWORD   dwFXAlphaCaps;          // alpha driver specific capabilities
/* 18*/ DWORD   dwPalCaps;              // palette capabilities
/* 1c*/ DWORD   dwSVCaps;               // stereo vision capabilities
/* 20*/ DWORD   dwAlphaBltConstBitDepths;       // DDBD_2,4,8
/* 24*/ DWORD   dwAlphaBltPixelBitDepths;       // DDBD_1,2,4,8
/* 28*/ DWORD   dwAlphaBltSurfaceBitDepths;     // DDBD_1,2,4,8
/* 2c*/ DWORD   dwAlphaOverlayConstBitDepths;   // DDBD_2,4,8
/* 30*/ DWORD   dwAlphaOverlayPixelBitDepths;   // DDBD_1,2,4,8
/* 34*/ DWORD   dwAlphaOverlaySurfaceBitDepths; // DDBD_1,2,4,8
/* 38*/ DWORD   dwZBufferBitDepths;             // DDBD_8,16,24,32
/* 3c*/ DWORD   dwVidMemTotal;          // total amount of video memory
/* 40*/ DWORD   dwVidMemFree;           // amount of free video memory
/* 44*/ DWORD   dwMaxVisibleOverlays;   // maximum number of visible overlays
/* 48*/ DWORD   dwCurrVisibleOverlays;  // current number of visible overlays
/* 4c*/ DWORD   dwNumFourCCCodes;       // number of four cc codes
/* 50*/ DWORD   dwAlignBoundarySrc;     // source rectangle alignment
/* 54*/ DWORD   dwAlignSizeSrc;         // source rectangle byte size
/* 58*/ DWORD   dwAlignBoundaryDest;    // dest rectangle alignment
/* 5c*/ DWORD   dwAlignSizeDest;        // dest rectangle byte size
/* 60*/ DWORD   dwAlignStrideAlign;     // stride alignment
/* 64*/ DWORD   dwRops[DD_ROP_SPACE];   // ROPS supported
/* 84*/ DDSCAPS ddsOldCaps;             // Was DDSCAPS  ddsCaps. ddsCaps is of type DDSCAPS2 for DX6
/* 88*/ DWORD   dwMinOverlayStretch;    // minimum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
/* 8c*/ DWORD   dwMaxOverlayStretch;    // maximum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
/* 90*/ DWORD   dwMinLiveVideoStretch;  // minimum live video stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
/* 94*/ DWORD   dwMaxLiveVideoStretch;  // maximum live video stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
/* 98*/ DWORD   dwMinHwCodecStretch;    // minimum hardware codec stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
/* 9c*/ DWORD   dwMaxHwCodecStretch;    // maximum hardware codec stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
/* a0*/ DWORD   dwReserved1;            // reserved
/* a4*/ DWORD   dwReserved2;            // reserved
/* a8*/ DWORD   dwReserved3;            // reserved
/* ac*/ DWORD   dwSVBCaps;              // driver specific capabilities for System->Vmem blts
/* b0*/ DWORD   dwSVBCKeyCaps;          // driver color key capabilities for System->Vmem blts
/* b4*/ DWORD   dwSVBFXCaps;            // driver FX capabilities for System->Vmem blts
/* b8*/ DWORD   dwSVBRops[DD_ROP_SPACE];// ROPS supported for System->Vmem blts
/* d8*/ DWORD   dwVSBCaps;              // driver specific capabilities for Vmem->System blts
/* dc*/ DWORD   dwVSBCKeyCaps;          // driver color key capabilities for Vmem->System blts
/* e0*/ DWORD   dwVSBFXCaps;            // driver FX capabilities for Vmem->System blts
/* e4*/ DWORD   dwVSBRops[DD_ROP_SPACE];// ROPS supported for Vmem->System blts
/*104*/ DWORD   dwSSBCaps;              // driver specific capabilities for System->System blts
/*108*/ DWORD   dwSSBCKeyCaps;          // driver color key capabilities for System->System blts
/*10c*/ DWORD   dwSSBFXCaps;            // driver FX capabilities for System->System blts
/*110*/ DWORD   dwSSBRops[DD_ROP_SPACE];// ROPS supported for System->System blts
/*130*/ DWORD   dwMaxVideoPorts;        // maximum number of usable video ports
/*134*/ DWORD   dwCurrVideoPorts;       // current number of video ports used
/*138*/ DWORD   dwSVBCaps2;             // more driver specific capabilities for System->Vmem blts
/*13c*/ DWORD   dwNLVBCaps;               // driver specific capabilities for non-local->local vidmem blts
/*140*/ DWORD   dwNLVBCaps2;              // more driver specific capabilities non-local->local vidmem blts
/*144*/ DWORD   dwNLVBCKeyCaps;           // driver color key capabilities for non-local->local vidmem blts
/*148*/ DWORD   dwNLVBFXCaps;             // driver FX capabilities for non-local->local blts
/*14c*/ DWORD   dwNLVBRops[DD_ROP_SPACE]; // ROPS supported for non-local->local blts
// Members added for DX6 release
/*16c*/ DDSCAPS2 ddsCaps;               // Surface Caps
} DDCAPS_DX7;
typedef DDCAPS_DX7 FAR* LPDDCAPS_DX7;


#if DIRECTDRAW_VERSION <= 0x300
    typedef DDCAPS_DX3 DDCAPS;
#elif DIRECTDRAW_VERSION <= 0x500
    typedef DDCAPS_DX5 DDCAPS;
#elif DIRECTDRAW_VERSION <= 0x600
    typedef DDCAPS_DX6 DDCAPS;
#else
    typedef DDCAPS_DX7 DDCAPS;
#endif

typedef DDCAPS FAR* LPDDCAPS;



/*
 * DDPIXELFORMAT
 */
typedef struct _DDPIXELFORMAT
{
    DWORD       dwSize;                 // size of structure
    DWORD       dwFlags;                // pixel format flags
    DWORD       dwFourCC;               // (FOURCC code)
    union
    {
        DWORD   dwRGBBitCount;          // how many bits per pixel
        DWORD   dwYUVBitCount;          // how many bits per pixel
        DWORD   dwZBufferBitDepth;      // how many total bits/pixel in z buffer (including any stencil bits)
        DWORD   dwAlphaBitDepth;        // how many bits for alpha channels
        DWORD   dwLuminanceBitCount;    // how many bits per pixel
        DWORD   dwBumpBitCount;         // how many bits per "buxel", total
        DWORD   dwPrivateFormatBitCount;// Bits per pixel of private driver formats. Only valid in texture
                                        // format list and if DDPF_D3DFORMAT is set
    } DUMMYUNIONNAMEN(1);
    union
    {
        DWORD   dwRBitMask;             // mask for red bit
        DWORD   dwYBitMask;             // mask for Y bits
        DWORD   dwStencilBitDepth;      // how many stencil bits (note: dwZBufferBitDepth-dwStencilBitDepth is total Z-only bits)
        DWORD   dwLuminanceBitMask;     // mask for luminance bits
        DWORD   dwBumpDuBitMask;        // mask for bump map U delta bits
        DWORD   dwOperations;           // DDPF_D3DFORMAT Operations
    } DUMMYUNIONNAMEN(2);
    union
    {
        DWORD   dwGBitMask;             // mask for green bits
        DWORD   dwUBitMask;             // mask for U bits
        DWORD   dwZBitMask;             // mask for Z bits
        DWORD   dwBumpDvBitMask;        // mask for bump map V delta bits
        struct
        {
            WORD    wFlipMSTypes;       // Multisample methods supported via flip for this D3DFORMAT
            WORD    wBltMSTypes;        // Multisample methods supported via blt for this D3DFORMAT
        } MultiSampleCaps;

    } DUMMYUNIONNAMEN(3);
    union
    {
        DWORD   dwBBitMask;             // mask for blue bits
        DWORD   dwVBitMask;             // mask for V bits
        DWORD   dwStencilBitMask;       // mask for stencil bits
        DWORD   dwBumpLuminanceBitMask; // mask for luminance in bump map
    } DUMMYUNIONNAMEN(4);
    union
    {
        DWORD   dwRGBAlphaBitMask;      // mask for alpha channel
        DWORD   dwYUVAlphaBitMask;      // mask for alpha channel
        DWORD   dwLuminanceAlphaBitMask;// mask for alpha channel
        DWORD   dwRGBZBitMask;          // mask for Z channel
        DWORD   dwYUVZBitMask;          // mask for Z channel
    } DUMMYUNIONNAMEN(5);
} DDPIXELFORMAT;

typedef DDPIXELFORMAT FAR* LPDDPIXELFORMAT;

/*
 * DDOVERLAYFX
 */
typedef struct _DDOVERLAYFX
{
    DWORD       dwSize;                         // size of structure
    DWORD       dwAlphaEdgeBlendBitDepth;       // Bit depth used to specify constant for alpha edge blend
    DWORD       dwAlphaEdgeBlend;               // Constant to use as alpha for edge blend
    DWORD       dwReserved;
    DWORD       dwAlphaDestConstBitDepth;       // Bit depth used to specify alpha constant for destination
    union
    {
        DWORD   dwAlphaDestConst;               // Constant to use as alpha channel for dest
        LPDIRECTDRAWSURFACE lpDDSAlphaDest;     // Surface to use as alpha channel for dest
    } DUMMYUNIONNAMEN(1);
    DWORD       dwAlphaSrcConstBitDepth;        // Bit depth used to specify alpha constant for source
    union
    {
        DWORD   dwAlphaSrcConst;                // Constant to use as alpha channel for src
        LPDIRECTDRAWSURFACE lpDDSAlphaSrc;      // Surface to use as alpha channel for src
    } DUMMYUNIONNAMEN(2);
    DDCOLORKEY  dckDestColorkey;                // DestColorkey override
    DDCOLORKEY  dckSrcColorkey;                 // DestColorkey override
    DWORD       dwDDFX;                         // Overlay FX
    DWORD       dwFlags;                        // flags
} DDOVERLAYFX;

typedef DDOVERLAYFX FAR *LPDDOVERLAYFX;


/*
 * DDBLTBATCH: BltBatch entry structure
 */
typedef struct _DDBLTBATCH
{
    LPRECT              lprDest;
    LPDIRECTDRAWSURFACE lpDDSSrc;
    LPRECT              lprSrc;
    DWORD               dwFlags;
    LPDDBLTFX           lpDDBltFx;
} DDBLTBATCH;

typedef DDBLTBATCH FAR * LPDDBLTBATCH;


/*
 * DDGAMMARAMP
 */
typedef struct _DDGAMMARAMP
{
    WORD                red[256];
    WORD                green[256];
    WORD                blue[256];
} DDGAMMARAMP;
typedef DDGAMMARAMP FAR * LPDDGAMMARAMP;

/*
 *  This is the structure within which DirectDraw returns data about the current graphics driver and chipset
 */

#define MAX_DDDEVICEID_STRING           512

typedef struct tagDDDEVICEIDENTIFIER
{
    /*
     * These elements are for presentation to the user only. They should not be used to identify particular
     * drivers, since this is unreliable and many different strings may be associated with the same
     * device, and the same driver from different vendors.
     */
    char    szDriver[MAX_DDDEVICEID_STRING];
    char    szDescription[MAX_DDDEVICEID_STRING];

    /*
     * This element is the version of the DirectDraw/3D driver. It is legal to do <, > comparisons
     * on the whole 64 bits. Caution should be exercised if you use this element to identify problematic
     * drivers. It is recommended that guidDeviceIdentifier is used for this purpose.
     *
     * This version has the form:
     *  wProduct = HIWORD(liDriverVersion.HighPart)
     *  wVersion = LOWORD(liDriverVersion.HighPart)
     *  wSubVersion = HIWORD(liDriverVersion.LowPart)
     *  wBuild = LOWORD(liDriverVersion.LowPart)
     */
#ifdef _WIN32
    LARGE_INTEGER liDriverVersion;      /* Defined for applications and other 32 bit components */
#else
    DWORD   dwDriverVersionLowPart;     /* Defined for 16 bit driver components */
    DWORD   dwDriverVersionHighPart;
#endif


    /*
     * These elements can be used to identify particular chipsets. Use with extreme caution.
     *   dwVendorId     Identifies the manufacturer. May be zero if unknown.
     *   dwDeviceId     Identifies the type of chipset. May be zero if unknown.
     *   dwSubSysId     Identifies the subsystem, typically this means the particular board. May be zero if unknown.
     *   dwRevision     Identifies the revision level of the chipset. May be zero if unknown.
     */
    DWORD   dwVendorId;
    DWORD   dwDeviceId;
    DWORD   dwSubSysId;
    DWORD   dwRevision;

    /*
     * This element can be used to check changes in driver/chipset. This GUID is a unique identifier for the
     * driver/chipset pair. Use this element if you wish to track changes to the driver/chipset in order to
     * reprofile the graphics subsystem.
     * This element can also be used to identify particular problematic drivers.
     */
    GUID    guidDeviceIdentifier;
} DDDEVICEIDENTIFIER, * LPDDDEVICEIDENTIFIER;

typedef struct tagDDDEVICEIDENTIFIER2
{
    /*
     * These elements are for presentation to the user only. They should not be used to identify particular
     * drivers, since this is unreliable and many different strings may be associated with the same
     * device, and the same driver from different vendors.
     */
    char    szDriver[MAX_DDDEVICEID_STRING];
    char    szDescription[MAX_DDDEVICEID_STRING];

    /*
     * This element is the version of the DirectDraw/3D driver. It is legal to do <, > comparisons
     * on the whole 64 bits. Caution should be exercised if you use this element to identify problematic
     * drivers. It is recommended that guidDeviceIdentifier is used for this purpose.
     *
     * This version has the form:
     *  wProduct = HIWORD(liDriverVersion.HighPart)
     *  wVersion = LOWORD(liDriverVersion.HighPart)
     *  wSubVersion = HIWORD(liDriverVersion.LowPart)
     *  wBuild = LOWORD(liDriverVersion.LowPart)
     */
#ifdef _WIN32
    LARGE_INTEGER liDriverVersion;      /* Defined for applications and other 32 bit components */
#else
    DWORD   dwDriverVersionLowPart;     /* Defined for 16 bit driver components */
    DWORD   dwDriverVersionHighPart;
#endif


    /*
     * These elements can be used to identify particular chipsets. Use with extreme caution.
     *   dwVendorId     Identifies the manufacturer. May be zero if unknown.
     *   dwDeviceId     Identifies the type of chipset. May be zero if unknown.
     *   dwSubSysId     Identifies the subsystem, typically this means the particular board. May be zero if unknown.
     *   dwRevision     Identifies the revision level of the chipset. May be zero if unknown.
     */
    DWORD   dwVendorId;
    DWORD   dwDeviceId;
    DWORD   dwSubSysId;
    DWORD   dwRevision;

    /*
     * This element can be used to check changes in driver/chipset. This GUID is a unique identifier for the
     * driver/chipset pair. Use this element if you wish to track changes to the driver/chipset in order to
     * reprofile the graphics subsystem.
     * This element can also be used to identify particular problematic drivers.
     */
    GUID    guidDeviceIdentifier;

    /*
     * This element is used to determine the Windows Hardware Quality Lab (WHQL)
     * certification level for this driver/device pair.
     */
    DWORD   dwWHQLLevel;

} DDDEVICEIDENTIFIER2, * LPDDDEVICEIDENTIFIER2;

/*
 * Flags for the IDirectDraw4::GetDeviceIdentifier method
 */

/*
 * This flag causes GetDeviceIdentifier to return information about the host (typically 2D) adapter in a system equipped
 * with a stacked secondary 3D adapter. Such an adapter appears to the application as if it were part of the
 * host adapter, but is typically physcially located on a separate card. The stacked secondary's information is
 * returned when GetDeviceIdentifier's dwFlags field is zero, since this most accurately reflects the qualities
 * of the DirectDraw object involved.
 */
#define DDGDI_GETHOSTIDENTIFIER         0x00000001L

/*
 * Macros for interpretting DDEVICEIDENTIFIER2.dwWHQLLevel
 */
#define GET_WHQL_YEAR( dwWHQLLevel ) \
    ( (dwWHQLLevel) / 0x10000 )
#define GET_WHQL_MONTH( dwWHQLLevel ) \
    ( ( (dwWHQLLevel) / 0x100 ) & 0x00ff )
#define GET_WHQL_DAY( dwWHQLLevel ) \
    ( (dwWHQLLevel) & 0xff )


/*
 * callbacks
 */
typedef DWORD   (FAR PASCAL *LPCLIPPERCALLBACK)(LPDIRECTDRAWCLIPPER lpDDClipper, HWND hWnd, DWORD code, LPVOID lpContext );
#ifdef STREAMING
typedef DWORD   (FAR PASCAL *LPSURFACESTREAMINGCALLBACK)(DWORD);
#endif


/*
 * INTERACES FOLLOW:
 *      IDirectDraw
 *      IDirectDrawClipper
 *      IDirectDrawPalette
 *      IDirectDrawSurface
 */

/*
 * IDirectDraw
 */
#if defined( _WIN32 ) && !defined( _NO_COM )
#undef INTERFACE
#define INTERFACE IDirectDraw
DECLARE_INTERFACE_( IDirectDraw, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDraw methods ***/
    STDMETHOD(Compact)(THIS) PURE;
    STDMETHOD(CreateClipper)(THIS_ DWORD, LPDIRECTDRAWCLIPPER FAR*, IUnknown FAR * ) PURE;
    STDMETHOD(CreatePalette)(THIS_ DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE FAR*, IUnknown FAR * ) PURE;
    STDMETHOD(CreateSurface)(THIS_  LPDDSURFACEDESC, LPDIRECTDRAWSURFACE FAR *, IUnknown FAR *) PURE;
    STDMETHOD(DuplicateSurface)( THIS_ LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE FAR * ) PURE;
    STDMETHOD(EnumDisplayModes)( THIS_ DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMMODESCALLBACK ) PURE;
    STDMETHOD(EnumSurfaces)(THIS_ DWORD, LPDDSURFACEDESC, LPVOID,LPDDENUMSURFACESCALLBACK ) PURE;
    STDMETHOD(FlipToGDISurface)(THIS) PURE;
    STDMETHOD(GetCaps)( THIS_ LPDDCAPS, LPDDCAPS) PURE;
    STDMETHOD(GetDisplayMode)( THIS_ LPDDSURFACEDESC) PURE;
    STDMETHOD(GetFourCCCodes)(THIS_  LPDWORD, LPDWORD ) PURE;
    STDMETHOD(GetGDISurface)(THIS_ LPDIRECTDRAWSURFACE FAR *) PURE;
    STDMETHOD(GetMonitorFrequency)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetScanLine)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetVerticalBlankStatus)(THIS_ LPBOOL ) PURE;
    STDMETHOD(Initialize)(THIS_ GUID FAR *) PURE;
    STDMETHOD(RestoreDisplayMode)(THIS) PURE;
    STDMETHOD(SetCooperativeLevel)(THIS_ HWND, DWORD) PURE;
    STDMETHOD(SetDisplayMode)(THIS_ DWORD, DWORD,DWORD) PURE;
    STDMETHOD(WaitForVerticalBlank)(THIS_ DWORD, HANDLE ) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectDraw_QueryInterface(p, a, b)         (p)->lpVtbl->QueryInterface(p, a, b)
#define IDirectDraw_AddRef(p)                       (p)->lpVtbl->AddRef(p)
#define IDirectDraw_Release(p)                      (p)->lpVtbl->Release(p)
#define IDirectDraw_Compact(p)                      (p)->lpVtbl->Compact(p)
#define IDirectDraw_CreateClipper(p, a, b, c)       (p)->lpVtbl->CreateClipper(p, a, b, c)
#define IDirectDraw_CreatePalette(p, a, b, c, d)    (p)->lpVtbl->CreatePalette(p, a, b, c, d)
#define IDirectDraw_CreateSurface(p, a, b, c)       (p)->lpVtbl->CreateSurface(p, a, b, c)
#define IDirectDraw_DuplicateSurface(p, a, b)       (p)->lpVtbl->DuplicateSurface(p, a, b)
#define IDirectDraw_EnumDisplayModes(p, a, b, c, d) (p)->lpVtbl->EnumDisplayModes(p, a, b, c, d)
#define IDirectDraw_EnumSurfaces(p, a, b, c, d)     (p)->lpVtbl->EnumSurfaces(p, a, b, c, d)
#define IDirectDraw_FlipToGDISurface(p)             (p)->lpVtbl->FlipToGDISurface(p)
#define IDirectDraw_GetCaps(p, a, b)                (p)->lpVtbl->GetCaps(p, a, b)
#define IDirectDraw_GetDisplayMode(p, a)            (p)->lpVtbl->GetDisplayMode(p, a)
#define IDirectDraw_GetFourCCCodes(p, a, b)         (p)->lpVtbl->GetFourCCCodes(p, a, b)
#define IDirectDraw_GetGDISurface(p, a)             (p)->lpVtbl->GetGDISurface(p, a)
#define IDirectDraw_GetMonitorFrequency(p, a)       (p)->lpVtbl->GetMonitorFrequency(p, a)
#define IDirectDraw_GetScanLine(p, a)               (p)->lpVtbl->GetScanLine(p, a)
#define IDirectDraw_GetVerticalBlankStatus(p, a)    (p)->lpVtbl->GetVerticalBlankStatus(p, a)
#define IDirectDraw_Initialize(p, a)                (p)->lpVtbl->Initialize(p, a)
#define IDirectDraw_RestoreDisplayMode(p)           (p)->lpVtbl->RestoreDisplayMode(p)
#define IDirectDraw_SetCooperativeLevel(p, a, b)    (p)->lpVtbl->SetCooperativeLevel(p, a, b)
#define IDirectDraw_SetDisplayMode(p, a, b, c)      (p)->lpVtbl->SetDisplayMode(p, a, b, c)
#define IDirectDraw_WaitForVerticalBlank(p, a, b)   (p)->lpVtbl->WaitForVerticalBlank(p, a, b)
#else
#define IDirectDraw_QueryInterface(p, a, b)         (p)->QueryInterface(a, b)
#define IDirectDraw_AddRef(p)                       (p)->AddRef()
#define IDirectDraw_Release(p)                      (p)->Release()
#define IDirectDraw_Compact(p)                      (p)->Compact()
#define IDirectDraw_CreateClipper(p, a, b, c)       (p)->CreateClipper(a, b, c)
#define IDirectDraw_CreatePalette(p, a, b, c, d)    (p)->CreatePalette(a, b, c, d)
#define IDirectDraw_CreateSurface(p, a, b, c)       (p)->CreateSurface(a, b, c)
#define IDirectDraw_DuplicateSurface(p, a, b)       (p)->DuplicateSurface(a, b)
#define IDirectDraw_EnumDisplayModes(p, a, b, c, d) (p)->EnumDisplayModes(a, b, c, d)
#define IDirectDraw_EnumSurfaces(p, a, b, c, d)     (p)->EnumSurfaces(a, b, c, d)
#define IDirectDraw_FlipToGDISurface(p)             (p)->FlipToGDISurface()
#define IDirectDraw_GetCaps(p, a, b)                (p)->GetCaps(a, b)
#define IDirectDraw_GetDisplayMode(p, a)            (p)->GetDisplayMode(a)
#define IDirectDraw_GetFourCCCodes(p, a, b)         (p)->GetFourCCCodes(a, b)
#define IDirectDraw_GetGDISurface(p, a)             (p)->GetGDISurface(a)
#define IDirectDraw_GetMonitorFrequency(p, a)       (p)->GetMonitorFrequency(a)
#define IDirectDraw_GetScanLine(p, a)               (p)->GetScanLine(a)
#define IDirectDraw_GetVerticalBlankStatus(p, a)    (p)->GetVerticalBlankStatus(a)
#define IDirectDraw_Initialize(p, a)                (p)->Initialize(a)
#define IDirectDraw_RestoreDisplayMode(p)           (p)->RestoreDisplayMode()
#define IDirectDraw_SetCooperativeLevel(p, a, b)    (p)->SetCooperativeLevel(a, b)
#define IDirectDraw_SetDisplayMode(p, a, b, c)      (p)->SetDisplayMode(a, b, c)
#define IDirectDraw_WaitForVerticalBlank(p, a, b)   (p)->WaitForVerticalBlank(a, b)
#endif

#endif

#if defined( _WIN32 ) && !defined( _NO_COM )
#undef INTERFACE
#define INTERFACE IDirectDraw2
DECLARE_INTERFACE_( IDirectDraw2, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDraw methods ***/
    STDMETHOD(Compact)(THIS) PURE;
    STDMETHOD(CreateClipper)(THIS_ DWORD, LPDIRECTDRAWCLIPPER FAR*, IUnknown FAR * ) PURE;
    STDMETHOD(CreatePalette)(THIS_ DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE FAR*, IUnknown FAR * ) PURE;
    STDMETHOD(CreateSurface)(THIS_  LPDDSURFACEDESC, LPDIRECTDRAWSURFACE FAR *, IUnknown FAR *) PURE;
    STDMETHOD(DuplicateSurface)( THIS_ LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE FAR * ) PURE;
    STDMETHOD(EnumDisplayModes)( THIS_ DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMMODESCALLBACK ) PURE;
    STDMETHOD(EnumSurfaces)(THIS_ DWORD, LPDDSURFACEDESC, LPVOID,LPDDENUMSURFACESCALLBACK ) PURE;
    STDMETHOD(FlipToGDISurface)(THIS) PURE;
    STDMETHOD(GetCaps)( THIS_ LPDDCAPS, LPDDCAPS) PURE;
    STDMETHOD(GetDisplayMode)( THIS_ LPDDSURFACEDESC) PURE;
    STDMETHOD(GetFourCCCodes)(THIS_  LPDWORD, LPDWORD ) PURE;
    STDMETHOD(GetGDISurface)(THIS_ LPDIRECTDRAWSURFACE FAR *) PURE;
    STDMETHOD(GetMonitorFrequency)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetScanLine)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetVerticalBlankStatus)(THIS_ LPBOOL ) PURE;
    STDMETHOD(Initialize)(THIS_ GUID FAR *) PURE;
    STDMETHOD(RestoreDisplayMode)(THIS) PURE;
    STDMETHOD(SetCooperativeLevel)(THIS_ HWND, DWORD) PURE;
    STDMETHOD(SetDisplayMode)(THIS_ DWORD, DWORD,DWORD, DWORD, DWORD) PURE;
    STDMETHOD(WaitForVerticalBlank)(THIS_ DWORD, HANDLE ) PURE;
    /*** Added in the v2 interface ***/
    STDMETHOD(GetAvailableVidMem)(THIS_ LPDDSCAPS, LPDWORD, LPDWORD) PURE;
};
#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectDraw2_QueryInterface(p, a, b)         (p)->lpVtbl->QueryInterface(p, a, b)
#define IDirectDraw2_AddRef(p)                       (p)->lpVtbl->AddRef(p)
#define IDirectDraw2_Release(p)                      (p)->lpVtbl->Release(p)
#define IDirectDraw2_Compact(p)                      (p)->lpVtbl->Compact(p)
#define IDirectDraw2_CreateClipper(p, a, b, c)       (p)->lpVtbl->CreateClipper(p, a, b, c)
#define IDirectDraw2_CreatePalette(p, a, b, c, d)    (p)->lpVtbl->CreatePalette(p, a, b, c, d)
#define IDirectDraw2_CreateSurface(p, a, b, c)       (p)->lpVtbl->CreateSurface(p, a, b, c)
#define IDirectDraw2_DuplicateSurface(p, a, b)       (p)->lpVtbl->DuplicateSurface(p, a, b)
#define IDirectDraw2_EnumDisplayModes(p, a, b, c, d) (p)->lpVtbl->EnumDisplayModes(p, a, b, c, d)
#define IDirectDraw2_EnumSurfaces(p, a, b, c, d)     (p)->lpVtbl->EnumSurfaces(p, a, b, c, d)
#define IDirectDraw2_FlipToGDISurface(p)             (p)->lpVtbl->FlipToGDISurface(p)
#define IDirectDraw2_GetCaps(p, a, b)                (p)->lpVtbl->GetCaps(p, a, b)
#define IDirectDraw2_GetDisplayMode(p, a)            (p)->lpVtbl->GetDisplayMode(p, a)
#define IDirectDraw2_GetFourCCCodes(p, a, b)         (p)->lpVtbl->GetFourCCCodes(p, a, b)
#define IDirectDraw2_GetGDISurface(p, a)             (p)->lpVtbl->GetGDISurface(p, a)
#define IDirectDraw2_GetMonitorFrequency(p, a)       (p)->lpVtbl->GetMonitorFrequency(p, a)
#define IDirectDraw2_GetScanLine(p, a)               (p)->lpVtbl->GetScanLine(p, a)
#define IDirectDraw2_GetVerticalBlankStatus(p, a)    (p)->lpVtbl->GetVerticalBlankStatus(p, a)
#define IDirectDraw2_Initialize(p, a)                (p)->lpVtbl->Initialize(p, a)
#define IDirectDraw2_RestoreDisplayMode(p)           (p)->lpVtbl->RestoreDisplayMode(p)
#define IDirectDraw2_SetCooperativeLevel(p, a, b)    (p)->lpVtbl->SetCooperativeLevel(p, a, b)
#define IDirectDraw2_SetDisplayMode(p, a, b, c, d, e) (p)->lpVtbl->SetDisplayMode(p, a, b, c, d, e)
#define IDirectDraw2_WaitForVerticalBlank(p, a, b)   (p)->lpVtbl->WaitForVerticalBlank(p, a, b)
#define IDirectDraw2_GetAvailableVidMem(p, a, b, c)  (p)->lpVtbl->GetAvailableVidMem(p, a, b, c)
#else
#define IDirectDraw2_QueryInterface(p, a, b)         (p)->QueryInterface(a, b)
#define IDirectDraw2_AddRef(p)                       (p)->AddRef()
#define IDirectDraw2_Release(p)                      (p)->Release()
#define IDirectDraw2_Compact(p)                      (p)->Compact()
#define IDirectDraw2_CreateClipper(p, a, b, c)       (p)->CreateClipper(a, b, c)
#define IDirectDraw2_CreatePalette(p, a, b, c, d)    (p)->CreatePalette(a, b, c, d)
#define IDirectDraw2_CreateSurface(p, a, b, c)       (p)->CreateSurface(a, b, c)
#define IDirectDraw2_DuplicateSurface(p, a, b)       (p)->DuplicateSurface(a, b)
#define IDirectDraw2_EnumDisplayModes(p, a, b, c, d) (p)->EnumDisplayModes(a, b, c, d)
#define IDirectDraw2_EnumSurfaces(p, a, b, c, d)     (p)->EnumSurfaces(a, b, c, d)
#define IDirectDraw2_FlipToGDISurface(p)             (p)->FlipToGDISurface()
#define IDirectDraw2_GetCaps(p, a, b)                (p)->GetCaps(a, b)
#define IDirectDraw2_GetDisplayMode(p, a)            (p)->GetDisplayMode(a)
#define IDirectDraw2_GetFourCCCodes(p, a, b)         (p)->GetFourCCCodes(a, b)
#define IDirectDraw2_GetGDISurface(p, a)             (p)->GetGDISurface(a)
#define IDirectDraw2_GetMonitorFrequency(p, a)       (p)->GetMonitorFrequency(a)
#define IDirectDraw2_GetScanLine(p, a)               (p)->GetScanLine(a)
#define IDirectDraw2_GetVerticalBlankStatus(p, a)    (p)->GetVerticalBlankStatus(a)
#define IDirectDraw2_Initialize(p, a)                (p)->Initialize(a)
#define IDirectDraw2_RestoreDisplayMode(p)           (p)->RestoreDisplayMode()
#define IDirectDraw2_SetCooperativeLevel(p, a, b)    (p)->SetCooperativeLevel(a, b)
#define IDirectDraw2_SetDisplayMode(p, a, b, c, d, e) (p)->SetDisplayMode(a, b, c, d, e)
#define IDirectDraw2_WaitForVerticalBlank(p, a, b)   (p)->WaitForVerticalBlank(a, b)
#define IDirectDraw2_GetAvailableVidMem(p, a, b, c)  (p)->GetAvailableVidMem(a, b, c)
#endif

#endif

#if defined( _WIN32 ) && !defined( _NO_COM )
#undef INTERFACE
#define INTERFACE IDirectDraw4
DECLARE_INTERFACE_( IDirectDraw4, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDraw methods ***/
    STDMETHOD(Compact)(THIS) PURE;
    STDMETHOD(CreateClipper)(THIS_ DWORD, LPDIRECTDRAWCLIPPER FAR*, IUnknown FAR * ) PURE;
    STDMETHOD(CreatePalette)(THIS_ DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE FAR*, IUnknown FAR * ) PURE;
    STDMETHOD(CreateSurface)(THIS_  LPDDSURFACEDESC2, LPDIRECTDRAWSURFACE4 FAR *, IUnknown FAR *) PURE;
    STDMETHOD(DuplicateSurface)( THIS_ LPDIRECTDRAWSURFACE4, LPDIRECTDRAWSURFACE4 FAR * ) PURE;
    STDMETHOD(EnumDisplayModes)( THIS_ DWORD, LPDDSURFACEDESC2, LPVOID, LPDDENUMMODESCALLBACK2 ) PURE;
    STDMETHOD(EnumSurfaces)(THIS_ DWORD, LPDDSURFACEDESC2, LPVOID,LPDDENUMSURFACESCALLBACK2 ) PURE;
    STDMETHOD(FlipToGDISurface)(THIS) PURE;
    STDMETHOD(GetCaps)( THIS_ LPDDCAPS, LPDDCAPS) PURE;
    STDMETHOD(GetDisplayMode)( THIS_ LPDDSURFACEDESC2) PURE;
    STDMETHOD(GetFourCCCodes)(THIS_  LPDWORD, LPDWORD ) PURE;
    STDMETHOD(GetGDISurface)(THIS_ LPDIRECTDRAWSURFACE4 FAR *) PURE;
    STDMETHOD(GetMonitorFrequency)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetScanLine)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetVerticalBlankStatus)(THIS_ LPBOOL ) PURE;
    STDMETHOD(Initialize)(THIS_ GUID FAR *) PURE;
    STDMETHOD(RestoreDisplayMode)(THIS) PURE;
    STDMETHOD(SetCooperativeLevel)(THIS_ HWND, DWORD) PURE;
    STDMETHOD(SetDisplayMode)(THIS_ DWORD, DWORD,DWORD, DWORD, DWORD) PURE;
    STDMETHOD(WaitForVerticalBlank)(THIS_ DWORD, HANDLE ) PURE;
    /*** Added in the v2 interface ***/
    STDMETHOD(GetAvailableVidMem)(THIS_ LPDDSCAPS2, LPDWORD, LPDWORD) PURE;
    /*** Added in the V4 Interface ***/
    STDMETHOD(GetSurfaceFromDC) (THIS_ HDC, LPDIRECTDRAWSURFACE4 *) PURE;
    STDMETHOD(RestoreAllSurfaces)(THIS) PURE;
    STDMETHOD(TestCooperativeLevel)(THIS) PURE;
    STDMETHOD(GetDeviceIdentifier)(THIS_ LPDDDEVICEIDENTIFIER, DWORD ) PURE;
};
#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectDraw4_QueryInterface(p, a, b)         (p)->lpVtbl->QueryInterface(p, a, b)
#define IDirectDraw4_AddRef(p)                       (p)->lpVtbl->AddRef(p)
#define IDirectDraw4_Release(p)                      (p)->lpVtbl->Release(p)
#define IDirectDraw4_Compact(p)                      (p)->lpVtbl->Compact(p)
#define IDirectDraw4_CreateClipper(p, a, b, c)       (p)->lpVtbl->CreateClipper(p, a, b, c)
#define IDirectDraw4_CreatePalette(p, a, b, c, d)    (p)->lpVtbl->CreatePalette(p, a, b, c, d)
#define IDirectDraw4_CreateSurface(p, a, b, c)       (p)->lpVtbl->CreateSurface(p, a, b, c)
#define IDirectDraw4_DuplicateSurface(p, a, b)       (p)->lpVtbl->DuplicateSurface(p, a, b)
#define IDirectDraw4_EnumDisplayModes(p, a, b, c, d) (p)->lpVtbl->EnumDisplayModes(p, a, b, c, d)
#define IDirectDraw4_EnumSurfaces(p, a, b, c, d)     (p)->lpVtbl->EnumSurfaces(p, a, b, c, d)
#define IDirectDraw4_FlipToGDISurface(p)             (p)->lpVtbl->FlipToGDISurface(p)
#define IDirectDraw4_GetCaps(p, a, b)                (p)->lpVtbl->GetCaps(p, a, b)
#define IDirectDraw4_GetDisplayMode(p, a)            (p)->lpVtbl->GetDisplayMode(p, a)
#define IDirectDraw4_GetFourCCCodes(p, a, b)         (p)->lpVtbl->GetFourCCCodes(p, a, b)
#define IDirectDraw4_GetGDISurface(p, a)             (p)->lpVtbl->GetGDISurface(p, a)
#define IDirectDraw4_GetMonitorFrequency(p, a)       (p)->lpVtbl->GetMonitorFrequency(p, a)
#define IDirectDraw4_GetScanLine(p, a)               (p)->lpVtbl->GetScanLine(p, a)
#define IDirectDraw4_GetVerticalBlankStatus(p, a)    (p)->lpVtbl->GetVerticalBlankStatus(p, a)
#define IDirectDraw4_Initialize(p, a)                (p)->lpVtbl->Initialize(p, a)
#define IDirectDraw4_RestoreDisplayMode(p)           (p)->lpVtbl->RestoreDisplayMode(p)
#define IDirectDraw4_SetCooperativeLevel(p, a, b)    (p)->lpVtbl->SetCooperativeLevel(p, a, b)
#define IDirectDraw4_SetDisplayMode(p, a, b, c, d, e) (p)->lpVtbl->SetDisplayMode(p, a, b, c, d, e)
#define IDirectDraw4_WaitForVerticalBlank(p, a, b)   (p)->lpVtbl->WaitForVerticalBlank(p, a, b)
#define IDirectDraw4_GetAvailableVidMem(p, a, b, c)  (p)->lpVtbl->GetAvailableVidMem(p, a, b, c)
#define IDirectDraw4_GetSurfaceFromDC(p, a, b)       (p)->lpVtbl->GetSurfaceFromDC(p, a, b)
#define IDirectDraw4_RestoreAllSurfaces(p)           (p)->lpVtbl->RestoreAllSurfaces(p)
#define IDirectDraw4_TestCooperativeLevel(p)         (p)->lpVtbl->TestCooperativeLevel(p)
#define IDirectDraw4_GetDeviceIdentifier(p,a,b)      (p)->lpVtbl->GetDeviceIdentifier(p,a,b)
#else
#define IDirectDraw4_QueryInterface(p, a, b)         (p)->QueryInterface(a, b)
#define IDirectDraw4_AddRef(p)                       (p)->AddRef()
#define IDirectDraw4_Release(p)                      (p)->Release()
#define IDirectDraw4_Compact(p)                      (p)->Compact()
#define IDirectDraw4_CreateClipper(p, a, b, c)       (p)->CreateClipper(a, b, c)
#define IDirectDraw4_CreatePalette(p, a, b, c, d)    (p)->CreatePalette(a, b, c, d)
#define IDirectDraw4_CreateSurface(p, a, b, c)       (p)->CreateSurface(a, b, c)
#define IDirectDraw4_DuplicateSurface(p, a, b)       (p)->DuplicateSurface(a, b)
#define IDirectDraw4_EnumDisplayModes(p, a, b, c, d) (p)->EnumDisplayModes(a, b, c, d)
#define IDirectDraw4_EnumSurfaces(p, a, b, c, d)     (p)->EnumSurfaces(a, b, c, d)
#define IDirectDraw4_FlipToGDISurface(p)             (p)->FlipToGDISurface()
#define IDirectDraw4_GetCaps(p, a, b)                (p)->GetCaps(a, b)
#define IDirectDraw4_GetDisplayMode(p, a)            (p)->GetDisplayMode(a)
#define IDirectDraw4_GetFourCCCodes(p, a, b)         (p)->GetFourCCCodes(a, b)
#define IDirectDraw4_GetGDISurface(p, a)             (p)->GetGDISurface(a)
#define IDirectDraw4_GetMonitorFrequency(p, a)       (p)->GetMonitorFrequency(a)
#define IDirectDraw4_GetScanLine(p, a)               (p)->GetScanLine(a)
#define IDirectDraw4_GetVerticalBlankStatus(p, a)    (p)->GetVerticalBlankStatus(a)
#define IDirectDraw4_Initialize(p, a)                (p)->Initialize(a)
#define IDirectDraw4_RestoreDisplayMode(p)           (p)->RestoreDisplayMode()
#define IDirectDraw4_SetCooperativeLevel(p, a, b)    (p)->SetCooperativeLevel(a, b)
#define IDirectDraw4_SetDisplayMode(p, a, b, c, d, e) (p)->SetDisplayMode(a, b, c, d, e)
#define IDirectDraw4_WaitForVerticalBlank(p, a, b)   (p)->WaitForVerticalBlank(a, b)
#define IDirectDraw4_GetAvailableVidMem(p, a, b, c)  (p)->GetAvailableVidMem(a, b, c)
#define IDirectDraw4_GetSurfaceFromDC(p, a, b)       (p)->GetSurfaceFromDC(a, b)
#define IDirectDraw4_RestoreAllSurfaces(p)           (p)->RestoreAllSurfaces()
#define IDirectDraw4_TestCooperativeLevel(p)         (p)->TestCooperativeLevel()
#define IDirectDraw4_GetDeviceIdentifier(p,a,b)      (p)->GetDeviceIdentifier(a,b)
#endif

#endif

#if defined( _WIN32 ) && !defined( _NO_COM )
#undef INTERFACE
#define INTERFACE IDirectDraw7
DECLARE_INTERFACE_( IDirectDraw7, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDraw methods ***/
    STDMETHOD(Compact)(THIS) PURE;
    STDMETHOD(CreateClipper)(THIS_ DWORD, LPDIRECTDRAWCLIPPER FAR*, IUnknown FAR * ) PURE;
    STDMETHOD(CreatePalette)(THIS_ DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE FAR*, IUnknown FAR * ) PURE;
    STDMETHOD(CreateSurface)(THIS_  LPDDSURFACEDESC2, LPDIRECTDRAWSURFACE7 FAR *, IUnknown FAR *) PURE;
    STDMETHOD(DuplicateSurface)( THIS_ LPDIRECTDRAWSURFACE7, LPDIRECTDRAWSURFACE7 FAR * ) PURE;
    STDMETHOD(EnumDisplayModes)( THIS_ DWORD, LPDDSURFACEDESC2, LPVOID, LPDDENUMMODESCALLBACK2 ) PURE;
    STDMETHOD(EnumSurfaces)(THIS_ DWORD, LPDDSURFACEDESC2, LPVOID,LPDDENUMSURFACESCALLBACK7 ) PURE;
    STDMETHOD(FlipToGDISurface)(THIS) PURE;
    STDMETHOD(GetCaps)( THIS_ LPDDCAPS, LPDDCAPS) PURE;
    STDMETHOD(GetDisplayMode)( THIS_ LPDDSURFACEDESC2) PURE;
    STDMETHOD(GetFourCCCodes)(THIS_  LPDWORD, LPDWORD ) PURE;
    STDMETHOD(GetGDISurface)(THIS_ LPDIRECTDRAWSURFACE7 FAR *) PURE;
    STDMETHOD(GetMonitorFrequency)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetScanLine)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetVerticalBlankStatus)(THIS_ LPBOOL ) PURE;
    STDMETHOD(Initialize)(THIS_ GUID FAR *) PURE;
    STDMETHOD(RestoreDisplayMode)(THIS) PURE;
    STDMETHOD(SetCooperativeLevel)(THIS_ HWND, DWORD) PURE;
    STDMETHOD(SetDisplayMode)(THIS_ DWORD, DWORD,DWORD, DWORD, DWORD) PURE;
    STDMETHOD(WaitForVerticalBlank)(THIS_ DWORD, HANDLE ) PURE;
    /*** Added in the v2 interface ***/
    STDMETHOD(GetAvailableVidMem)(THIS_ LPDDSCAPS2, LPDWORD, LPDWORD) PURE;
    /*** Added in the V4 Interface ***/
    STDMETHOD(GetSurfaceFromDC) (THIS_ HDC, LPDIRECTDRAWSURFACE7 *) PURE;
    STDMETHOD(RestoreAllSurfaces)(THIS) PURE;
    STDMETHOD(TestCooperativeLevel)(THIS) PURE;
    STDMETHOD(GetDeviceIdentifier)(THIS_ LPDDDEVICEIDENTIFIER2, DWORD ) PURE;
    STDMETHOD(StartModeTest)(THIS_ LPSIZE, DWORD, DWORD ) PURE;
    STDMETHOD(EvaluateMode)(THIS_ DWORD, DWORD * ) PURE;
};
#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectDraw7_QueryInterface(p, a, b)         (p)->lpVtbl->QueryInterface(p, a, b)
#define IDirectDraw7_AddRef(p)                       (p)->lpVtbl->AddRef(p)
#define IDirectDraw7_Release(p)                      (p)->lpVtbl->Release(p)
#define IDirectDraw7_Compact(p)                      (p)->lpVtbl->Compact(p)
#define IDirectDraw7_CreateClipper(p, a, b, c)       (p)->lpVtbl->CreateClipper(p, a, b, c)
#define IDirectDraw7_CreatePalette(p, a, b, c, d)    (p)->lpVtbl->CreatePalette(p, a, b, c, d)
#define IDirectDraw7_CreateSurface(p, a, b, c)       (p)->lpVtbl->CreateSurface(p, a, b, c)
#define IDirectDraw7_DuplicateSurface(p, a, b)       (p)->lpVtbl->DuplicateSurface(p, a, b)
#define IDirectDraw7_EnumDisplayModes(p, a, b, c, d) (p)->lpVtbl->EnumDisplayModes(p, a, b, c, d)
#define IDirectDraw7_EnumSurfaces(p, a, b, c, d)     (p)->lpVtbl->EnumSurfaces(p, a, b, c, d)
#define IDirectDraw7_FlipToGDISurface(p)             (p)->lpVtbl->FlipToGDISurface(p)
#define IDirectDraw7_GetCaps(p, a, b)                (p)->lpVtbl->GetCaps(p, a, b)
#define IDirectDraw7_GetDisplayMode(p, a)            (p)->lpVtbl->GetDisplayMode(p, a)
#define IDirectDraw7_GetFourCCCodes(p, a, b)         (p)->lpVtbl->GetFourCCCodes(p, a, b)
#define IDirectDraw7_GetGDISurface(p, a)             (p)->lpVtbl->GetGDISurface(p, a)
#define IDirectDraw7_GetMonitorFrequency(p, a)       (p)->lpVtbl->GetMonitorFrequency(p, a)
#define IDirectDraw7_GetScanLine(p, a)               (p)->lpVtbl->GetScanLine(p, a)
#define IDirectDraw7_GetVerticalBlankStatus(p, a)    (p)->lpVtbl->GetVerticalBlankStatus(p, a)
#define IDirectDraw7_Initialize(p, a)                (p)->lpVtbl->Initialize(p, a)
#define IDirectDraw7_RestoreDisplayMode(p)           (p)->lpVtbl->RestoreDisplayMode(p)
#define IDirectDraw7_SetCooperativeLevel(p, a, b)    (p)->lpVtbl->SetCooperativeLevel(p, a, b)
#define IDirectDraw7_SetDisplayMode(p, a, b, c, d, e) (p)->lpVtbl->SetDisplayMode(p, a, b, c, d, e)
#define IDirectDraw7_WaitForVerticalBlank(p, a, b)   (p)->lpVtbl->WaitForVerticalBlank(p, a, b)
#define IDirectDraw7_GetAvailableVidMem(p, a, b, c)  (p)->lpVtbl->GetAvailableVidMem(p, a, b, c)
#define IDirectDraw7_GetSurfaceFromDC(p, a, b)       (p)->lpVtbl->GetSurfaceFromDC(p, a, b)
#define IDirectDraw7_RestoreAllSurfaces(p)           (p)->lpVtbl->RestoreAllSurfaces(p)
#define IDirectDraw7_TestCooperativeLevel(p)         (p)->lpVtbl->TestCooperativeLevel(p)
#define IDirectDraw7_GetDeviceIdentifier(p,a,b)      (p)->lpVtbl->GetDeviceIdentifier(p,a,b)
#define IDirectDraw7_StartModeTest(p,a,b,c)        (p)->lpVtbl->StartModeTest(p,a,b,c)
#define IDirectDraw7_EvaluateMode(p,a,b)           (p)->lpVtbl->EvaluateMode(p,a,b)
#else
#define IDirectDraw7_QueryInterface(p, a, b)         (p)->QueryInterface(a, b)
#define IDirectDraw7_AddRef(p)                       (p)->AddRef()
#define IDirectDraw7_Release(p)                      (p)->Release()
#define IDirectDraw7_Compact(p)                      (p)->Compact()
#define IDirectDraw7_CreateClipper(p, a, b, c)       (p)->CreateClipper(a, b, c)
#define IDirectDraw7_CreatePalette(p, a, b, c, d)    (p)->CreatePalette(a, b, c, d)
#define IDirectDraw7_CreateSurface(p, a, b, c)       (p)->CreateSurface(a, b, c)
#define IDirectDraw7_DuplicateSurface(p, a, b)       (p)->DuplicateSurface(a, b)
#define IDirectDraw7_EnumDisplayModes(p, a, b, c, d) (p)->EnumDisplayModes(a, b, c, d)
#define IDirectDraw7_EnumSurfaces(p, a, b, c, d)     (p)->EnumSurfaces(a, b, c, d)
#define IDirectDraw7_FlipToGDISurface(p)             (p)->FlipToGDISurface()
#define IDirectDraw7_GetCaps(p, a, b)                (p)->GetCaps(a, b)
#define IDirectDraw7_GetDisplayMode(p, a)            (p)->GetDisplayMode(a)
#define IDirectDraw7_GetFourCCCodes(p, a, b)         (p)->GetFourCCCodes(a, b)
#define IDirectDraw7_GetGDISurface(p, a)             (p)->GetGDISurface(a)
#define IDirectDraw7_GetMonitorFrequency(p, a)       (p)->GetMonitorFrequency(a)
#define IDirectDraw7_GetScanLine(p, a)               (p)->GetScanLine(a)
#define IDirectDraw7_GetVerticalBlankStatus(p, a)    (p)->GetVerticalBlankStatus(a)
#define IDirectDraw7_Initialize(p, a)                (p)->Initialize(a)
#define IDirectDraw7_RestoreDisplayMode(p)           (p)->RestoreDisplayMode()
#define IDirectDraw7_SetCooperativeLevel(p, a, b)    (p)->SetCooperativeLevel(a, b)
#define IDirectDraw7_SetDisplayMode(p, a, b, c, d, e) (p)->SetDisplayMode(a, b, c, d, e)
#define IDirectDraw7_WaitForVerticalBlank(p, a, b)   (p)->WaitForVerticalBlank(a, b)
#define IDirectDraw7_GetAvailableVidMem(p, a, b, c)  (p)->GetAvailableVidMem(a, b, c)
#define IDirectDraw7_GetSurfaceFromDC(p, a, b)       (p)->GetSurfaceFromDC(a, b)
#define IDirectDraw7_RestoreAllSurfaces(p)           (p)->RestoreAllSurfaces()
#define IDirectDraw7_TestCooperativeLevel(p)         (p)->TestCooperativeLevel()
#define IDirectDraw7_GetDeviceIdentifier(p,a,b)      (p)->GetDeviceIdentifier(a,b)
#define IDirectDraw7_StartModeTest(p,a,b,c)        (p)->lpVtbl->StartModeTest(a,b,c)
#define IDirectDraw7_EvaluateMode(p,a,b)           (p)->lpVtbl->EvaluateMode(a,b)
#endif

#endif


/*
 * IDirectDrawPalette
 */
#if defined( _WIN32 ) && !defined( _NO_COM )
#undef INTERFACE
#define INTERFACE IDirectDrawPalette
DECLARE_INTERFACE_( IDirectDrawPalette, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDrawPalette methods ***/
    STDMETHOD(GetCaps)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetEntries)(THIS_ DWORD,DWORD,DWORD,LPPALETTEENTRY) PURE;
    STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, DWORD, LPPALETTEENTRY) PURE;
    STDMETHOD(SetEntries)(THIS_ DWORD,DWORD,DWORD,LPPALETTEENTRY) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectDrawPalette_QueryInterface(p, a, b)      (p)->lpVtbl->QueryInterface(p, a, b)
#define IDirectDrawPalette_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IDirectDrawPalette_Release(p)                   (p)->lpVtbl->Release(p)
#define IDirectDrawPalette_GetCaps(p, a)                (p)->lpVtbl->GetCaps(p, a)
#define IDirectDrawPalette_GetEntries(p, a, b, c, d)    (p)->lpVtbl->GetEntries(p, a, b, c, d)
#define IDirectDrawPalette_Initialize(p, a, b, c)       (p)->lpVtbl->Initialize(p, a, b, c)
#define IDirectDrawPalette_SetEntries(p, a, b, c, d)    (p)->lpVtbl->SetEntries(p, a, b, c, d)
#else
#define IDirectDrawPalette_QueryInterface(p, a, b)      (p)->QueryInterface(a, b)
#define IDirectDrawPalette_AddRef(p)                    (p)->AddRef()
#define IDirectDrawPalette_Release(p)                   (p)->Release()
#define IDirectDrawPalette_GetCaps(p, a)                (p)->GetCaps(a)
#define IDirectDrawPalette_GetEntries(p, a, b, c, d)    (p)->GetEntries(a, b, c, d)
#define IDirectDrawPalette_Initialize(p, a, b, c)       (p)->Initialize(a, b, c)
#define IDirectDrawPalette_SetEntries(p, a, b, c, d)    (p)->SetEntries(a, b, c, d)
#endif

#endif


/*
 * IDirectDrawClipper
 */
#if defined( _WIN32 ) && !defined( _NO_COM )
#undef INTERFACE
#define INTERFACE IDirectDrawClipper
DECLARE_INTERFACE_( IDirectDrawClipper, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDrawClipper methods ***/
    STDMETHOD(GetClipList)(THIS_ LPRECT, LPRGNDATA, LPDWORD) PURE;
    STDMETHOD(GetHWnd)(THIS_ HWND FAR *) PURE;
    STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, DWORD) PURE;
    STDMETHOD(IsClipListChanged)(THIS_ BOOL FAR *) PURE;
    STDMETHOD(SetClipList)(THIS_ LPRGNDATA,DWORD) PURE;
    STDMETHOD(SetHWnd)(THIS_ DWORD, HWND ) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectDrawClipper_QueryInterface(p, a, b)  (p)->lpVtbl->QueryInterface(p, a, b)
#define IDirectDrawClipper_AddRef(p)                (p)->lpVtbl->AddRef(p)
#define IDirectDrawClipper_Release(p)               (p)->lpVtbl->Release(p)
#define IDirectDrawClipper_GetClipList(p, a, b, c)  (p)->lpVtbl->GetClipList(p, a, b, c)
#define IDirectDrawClipper_GetHWnd(p, a)            (p)->lpVtbl->GetHWnd(p, a)
#define IDirectDrawClipper_Initialize(p, a, b)      (p)->lpVtbl->Initialize(p, a, b)
#define IDirectDrawClipper_IsClipListChanged(p, a)  (p)->lpVtbl->IsClipListChanged(p, a)
#define IDirectDrawClipper_SetClipList(p, a, b)     (p)->lpVtbl->SetClipList(p, a, b)
#define IDirectDrawClipper_SetHWnd(p, a, b)         (p)->lpVtbl->SetHWnd(p, a, b)
#else
#define IDirectDrawClipper_QueryInterface(p, a, b)  (p)->QueryInterface(a, b)
#define IDirectDrawClipper_AddRef(p)                (p)->AddRef()
#define IDirectDrawClipper_Release(p)               (p)->Release()
#define IDirectDrawClipper_GetClipList(p, a, b, c)  (p)->GetClipList(a, b, c)
#define IDirectDrawClipper_GetHWnd(p, a)            (p)->GetHWnd(a)
#define IDirectDrawClipper_Initialize(p, a, b)      (p)->Initialize(a, b)
#define IDirectDrawClipper_IsClipListChanged(p, a)  (p)->IsClipListChanged(a)
#define IDirectDrawClipper_SetClipList(p, a, b)     (p)->SetClipList(a, b)
#define IDirectDrawClipper_SetHWnd(p, a, b)         (p)->SetHWnd(a, b)
#endif

#endif

/*
 * IDirectDrawSurface and related interfaces
 */
#if defined( _WIN32 ) && !defined( _NO_COM )
#undef INTERFACE
#define INTERFACE IDirectDrawSurface
DECLARE_INTERFACE_( IDirectDrawSurface, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDrawSurface methods ***/
    STDMETHOD(AddAttachedSurface)(THIS_ LPDIRECTDRAWSURFACE) PURE;
    STDMETHOD(AddOverlayDirtyRect)(THIS_ LPRECT) PURE;
    STDMETHOD(Blt)(THIS_ LPRECT,LPDIRECTDRAWSURFACE, LPRECT,DWORD, LPDDBLTFX) PURE;
    STDMETHOD(BltBatch)(THIS_ LPDDBLTBATCH, DWORD, DWORD ) PURE;
    STDMETHOD(BltFast)(THIS_ DWORD,DWORD,LPDIRECTDRAWSURFACE, LPRECT,DWORD) PURE;
    STDMETHOD(DeleteAttachedSurface)(THIS_ DWORD,LPDIRECTDRAWSURFACE) PURE;
    STDMETHOD(EnumAttachedSurfaces)(THIS_ LPVOID,LPDDENUMSURFACESCALLBACK) PURE;
    STDMETHOD(EnumOverlayZOrders)(THIS_ DWORD,LPVOID,LPDDENUMSURFACESCALLBACK) PURE;
    STDMETHOD(Flip)(THIS_ LPDIRECTDRAWSURFACE, DWORD) PURE;
    STDMETHOD(GetAttachedSurface)(THIS_ LPDDSCAPS, LPDIRECTDRAWSURFACE FAR *) PURE;
    STDMETHOD(GetBltStatus)(THIS_ DWORD) PURE;
    STDMETHOD(GetCaps)(THIS_ LPDDSCAPS) PURE;
    STDMETHOD(GetClipper)(THIS_ LPDIRECTDRAWCLIPPER FAR*) PURE;
    STDMETHOD(GetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
    STDMETHOD(GetDC)(THIS_ HDC FAR *) PURE;
    STDMETHOD(GetFlipStatus)(THIS_ DWORD) PURE;
    STDMETHOD(GetOverlayPosition)(THIS_ LPLONG, LPLONG ) PURE;
    STDMETHOD(GetPalette)(THIS_ LPDIRECTDRAWPALETTE FAR*) PURE;
    STDMETHOD(GetPixelFormat)(THIS_ LPDDPIXELFORMAT) PURE;
    STDMETHOD(GetSurfaceDesc)(THIS_ LPDDSURFACEDESC) PURE;
    STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, LPDDSURFACEDESC) PURE;
    STDMETHOD(IsLost)(THIS) PURE;
    STDMETHOD(Lock)(THIS_ LPRECT,LPDDSURFACEDESC,DWORD,HANDLE) PURE;
    STDMETHOD(ReleaseDC)(THIS_ HDC) PURE;
    STDMETHOD(Restore)(THIS) PURE;
    STDMETHOD(SetClipper)(THIS_ LPDIRECTDRAWCLIPPER) PURE;
    STDMETHOD(SetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
    STDMETHOD(SetOverlayPosition)(THIS_ LONG, LONG ) PURE;
    STDMETHOD(SetPalette)(THIS_ LPDIRECTDRAWPALETTE) PURE;
    STDMETHOD(Unlock)(THIS_ LPVOID) PURE;
    STDMETHOD(UpdateOverlay)(THIS_ LPRECT, LPDIRECTDRAWSURFACE,LPRECT,DWORD, LPDDOVERLAYFX) PURE;
    STDMETHOD(UpdateOverlayDisplay)(THIS_ DWORD) PURE;
    STDMETHOD(UpdateOverlayZOrder)(THIS_ DWORD, LPDIRECTDRAWSURFACE) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectDrawSurface_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectDrawSurface_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IDirectDrawSurface_Release(p)                   (p)->lpVtbl->Release(p)
#define IDirectDrawSurface_AddAttachedSurface(p,a)      (p)->lpVtbl->AddAttachedSurface(p,a)
#define IDirectDrawSurface_AddOverlayDirtyRect(p,a)     (p)->lpVtbl->AddOverlayDirtyRect(p,a)
#define IDirectDrawSurface_Blt(p,a,b,c,d,e)             (p)->lpVtbl->Blt(p,a,b,c,d,e)
#define IDirectDrawSurface_BltBatch(p,a,b,c)            (p)->lpVtbl->BltBatch(p,a,b,c)
#define IDirectDrawSurface_BltFast(p,a,b,c,d,e)         (p)->lpVtbl->BltFast(p,a,b,c,d,e)
#define IDirectDrawSurface_DeleteAttachedSurface(p,a,b) (p)->lpVtbl->DeleteAttachedSurface(p,a,b)
#define IDirectDrawSurface_EnumAttachedSurfaces(p,a,b)  (p)->lpVtbl->EnumAttachedSurfaces(p,a,b)
#define IDirectDrawSurface_EnumOverlayZOrders(p,a,b,c)  (p)->lpVtbl->EnumOverlayZOrders(p,a,b,c)
#define IDirectDrawSurface_Flip(p,a,b)                  (p)->lpVtbl->Flip(p,a,b)
#define IDirectDrawSurface_GetAttachedSurface(p,a,b)    (p)->lpVtbl->GetAttachedSurface(p,a,b)
#define IDirectDrawSurface_GetBltStatus(p,a)            (p)->lpVtbl->GetBltStatus(p,a)
#define IDirectDrawSurface_GetCaps(p,b)                 (p)->lpVtbl->GetCaps(p,b)
#define IDirectDrawSurface_GetClipper(p,a)              (p)->lpVtbl->GetClipper(p,a)
#define IDirectDrawSurface_GetColorKey(p,a,b)           (p)->lpVtbl->GetColorKey(p,a,b)
#define IDirectDrawSurface_GetDC(p,a)                   (p)->lpVtbl->GetDC(p,a)
#define IDirectDrawSurface_GetFlipStatus(p,a)           (p)->lpVtbl->GetFlipStatus(p,a)
#define IDirectDrawSurface_GetOverlayPosition(p,a,b)    (p)->lpVtbl->GetOverlayPosition(p,a,b)
#define IDirectDrawSurface_GetPalette(p,a)              (p)->lpVtbl->GetPalette(p,a)
#define IDirectDrawSurface_GetPixelFormat(p,a)          (p)->lpVtbl->GetPixelFormat(p,a)
#define IDirectDrawSurface_GetSurfaceDesc(p,a)          (p)->lpVtbl->GetSurfaceDesc(p,a)
#define IDirectDrawSurface_Initialize(p,a,b)            (p)->lpVtbl->Initialize(p,a,b)
#define IDirectDrawSurface_IsLost(p)                    (p)->lpVtbl->IsLost(p)
#define IDirectDrawSurface_Lock(p,a,b,c,d)              (p)->lpVtbl->Lock(p,a,b,c,d)
#define IDirectDrawSurface_ReleaseDC(p,a)               (p)->lpVtbl->ReleaseDC(p,a)
#define IDirectDrawSurface_Restore(p)                   (p)->lpVtbl->Restore(p)
#define IDirectDrawSurface_SetClipper(p,a)              (p)->lpVtbl->SetClipper(p,a)
#define IDirectDrawSurface_SetColorKey(p,a,b)           (p)->lpVtbl->SetColorKey(p,a,b)
#define IDirectDrawSurface_SetOverlayPosition(p,a,b)    (p)->lpVtbl->SetOverlayPosition(p,a,b)
#define IDirectDrawSurface_SetPalette(p,a)              (p)->lpVtbl->SetPalette(p,a)
#define IDirectDrawSurface_Unlock(p,b)                  (p)->lpVtbl->Unlock(p,b)
#define IDirectDrawSurface_UpdateOverlay(p,a,b,c,d,e)   (p)->lpVtbl->UpdateOverlay(p,a,b,c,d,e)
#define IDirectDrawSurface_UpdateOverlayDisplay(p,a)    (p)->lpVtbl->UpdateOverlayDisplay(p,a)
#define IDirectDrawSurface_UpdateOverlayZOrder(p,a,b)   (p)->lpVtbl->UpdateOverlayZOrder(p,a,b)
#else
#define IDirectDrawSurface_QueryInterface(p,a,b)        (p)->QueryInterface(a,b)
#define IDirectDrawSurface_AddRef(p)                    (p)->AddRef()
#define IDirectDrawSurface_Release(p)                   (p)->Release()
#define IDirectDrawSurface_AddAttachedSurface(p,a)      (p)->AddAttachedSurface(a)
#define IDirectDrawSurface_AddOverlayDirtyRect(p,a)     (p)->AddOverlayDirtyRect(a)
#define IDirectDrawSurface_Blt(p,a,b,c,d,e)             (p)->Blt(a,b,c,d,e)
#define IDirectDrawSurface_BltBatch(p,a,b,c)            (p)->BltBatch(a,b,c)
#define IDirectDrawSurface_BltFast(p,a,b,c,d,e)         (p)->BltFast(a,b,c,d,e)
#define IDirectDrawSurface_DeleteAttachedSurface(p,a,b) (p)->DeleteAttachedSurface(a,b)
#define IDirectDrawSurface_EnumAttachedSurfaces(p,a,b)  (p)->EnumAttachedSurfaces(a,b)
#define IDirectDrawSurface_EnumOverlayZOrders(p,a,b,c)  (p)->EnumOverlayZOrders(a,b,c)
#define IDirectDrawSurface_Flip(p,a,b)                  (p)->Flip(a,b)
#define IDirectDrawSurface_GetAttachedSurface(p,a,b)    (p)->GetAttachedSurface(a,b)
#define IDirectDrawSurface_GetBltStatus(p,a)            (p)->GetBltStatus(a)
#define IDirectDrawSurface_GetCaps(p,b)                 (p)->GetCaps(b)
#define IDirectDrawSurface_GetClipper(p,a)              (p)->GetClipper(a)
#define IDirectDrawSurface_GetColorKey(p,a,b)           (p)->GetColorKey(a,b)
#define IDirectDrawSurface_GetDC(p,a)                   (p)->GetDC(a)
#define IDirectDrawSurface_GetFlipStatus(p,a)           (p)->GetFlipStatus(a)
#define IDirectDrawSurface_GetOverlayPosition(p,a,b)    (p)->GetOverlayPosition(a,b)
#define IDirectDrawSurface_GetPalette(p,a)              (p)->GetPalette(a)
#define IDirectDrawSurface_GetPixelFormat(p,a)          (p)->GetPixelFormat(a)
#define IDirectDrawSurface_GetSurfaceDesc(p,a)          (p)->GetSurfaceDesc(a)
#define IDirectDrawSurface_Initialize(p,a,b)            (p)->Initialize(a,b)
#define IDirectDrawSurface_IsLost(p)                    (p)->IsLost()
#define IDirectDrawSurface_Lock(p,a,b,c,d)              (p)->Lock(a,b,c,d)
#define IDirectDrawSurface_ReleaseDC(p,a)               (p)->ReleaseDC(a)
#define IDirectDrawSurface_Restore(p)                   (p)->Restore()
#define IDirectDrawSurface_SetClipper(p,a)              (p)->SetClipper(a)
#define IDirectDrawSurface_SetColorKey(p,a,b)           (p)->SetColorKey(a,b)
#define IDirectDrawSurface_SetOverlayPosition(p,a,b)    (p)->SetOverlayPosition(a,b)
#define IDirectDrawSurface_SetPalette(p,a)              (p)->SetPalette(a)
#define IDirectDrawSurface_Unlock(p,b)                  (p)->Unlock(b)
#define IDirectDrawSurface_UpdateOverlay(p,a,b,c,d,e)   (p)->UpdateOverlay(a,b,c,d,e)
#define IDirectDrawSurface_UpdateOverlayDisplay(p,a)    (p)->UpdateOverlayDisplay(a)
#define IDirectDrawSurface_UpdateOverlayZOrder(p,a,b)   (p)->UpdateOverlayZOrder(a,b)
#endif

/*
 * IDirectDrawSurface2 and related interfaces
 */
#undef INTERFACE
#define INTERFACE IDirectDrawSurface2
DECLARE_INTERFACE_( IDirectDrawSurface2, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDrawSurface methods ***/
    STDMETHOD(AddAttachedSurface)(THIS_ LPDIRECTDRAWSURFACE2) PURE;
    STDMETHOD(AddOverlayDirtyRect)(THIS_ LPRECT) PURE;
    STDMETHOD(Blt)(THIS_ LPRECT,LPDIRECTDRAWSURFACE2, LPRECT,DWORD, LPDDBLTFX) PURE;
    STDMETHOD(BltBatch)(THIS_ LPDDBLTBATCH, DWORD, DWORD ) PURE;
    STDMETHOD(BltFast)(THIS_ DWORD,DWORD,LPDIRECTDRAWSURFACE2, LPRECT,DWORD) PURE;
    STDMETHOD(DeleteAttachedSurface)(THIS_ DWORD,LPDIRECTDRAWSURFACE2) PURE;
    STDMETHOD(EnumAttachedSurfaces)(THIS_ LPVOID,LPDDENUMSURFACESCALLBACK) PURE;
    STDMETHOD(EnumOverlayZOrders)(THIS_ DWORD,LPVOID,LPDDENUMSURFACESCALLBACK) PURE;
    STDMETHOD(Flip)(THIS_ LPDIRECTDRAWSURFACE2, DWORD) PURE;
    STDMETHOD(GetAttachedSurface)(THIS_ LPDDSCAPS, LPDIRECTDRAWSURFACE2 FAR *) PURE;
    STDMETHOD(GetBltStatus)(THIS_ DWORD) PURE;
    STDMETHOD(GetCaps)(THIS_ LPDDSCAPS) PURE;
    STDMETHOD(GetClipper)(THIS_ LPDIRECTDRAWCLIPPER FAR*) PURE;
    STDMETHOD(GetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
    STDMETHOD(GetDC)(THIS_ HDC FAR *) PURE;
    STDMETHOD(GetFlipStatus)(THIS_ DWORD) PURE;
    STDMETHOD(GetOverlayPosition)(THIS_ LPLONG, LPLONG ) PURE;
    STDMETHOD(GetPalette)(THIS_ LPDIRECTDRAWPALETTE FAR*) PURE;
    STDMETHOD(GetPixelFormat)(THIS_ LPDDPIXELFORMAT) PURE;
    STDMETHOD(GetSurfaceDesc)(THIS_ LPDDSURFACEDESC) PURE;
    STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, LPDDSURFACEDESC) PURE;
    STDMETHOD(IsLost)(THIS) PURE;
    STDMETHOD(Lock)(THIS_ LPRECT,LPDDSURFACEDESC,DWORD,HANDLE) PURE;
    STDMETHOD(ReleaseDC)(THIS_ HDC) PURE;
    STDMETHOD(Restore)(THIS) PURE;
    STDMETHOD(SetClipper)(THIS_ LPDIRECTDRAWCLIPPER) PURE;
    STDMETHOD(SetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
    STDMETHOD(SetOverlayPosition)(THIS_ LONG, LONG ) PURE;
    STDMETHOD(SetPalette)(THIS_ LPDIRECTDRAWPALETTE) PURE;
    STDMETHOD(Unlock)(THIS_ LPVOID) PURE;
    STDMETHOD(UpdateOverlay)(THIS_ LPRECT, LPDIRECTDRAWSURFACE2,LPRECT,DWORD, LPDDOVERLAYFX) PURE;
    STDMETHOD(UpdateOverlayDisplay)(THIS_ DWORD) PURE;
    STDMETHOD(UpdateOverlayZOrder)(THIS_ DWORD, LPDIRECTDRAWSURFACE2) PURE;
    /*** Added in the v2 interface ***/
    STDMETHOD(GetDDInterface)(THIS_ LPVOID FAR *) PURE;
    STDMETHOD(PageLock)(THIS_ DWORD) PURE;
    STDMETHOD(PageUnlock)(THIS_ DWORD) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectDrawSurface2_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectDrawSurface2_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IDirectDrawSurface2_Release(p)                   (p)->lpVtbl->Release(p)
#define IDirectDrawSurface2_AddAttachedSurface(p,a)      (p)->lpVtbl->AddAttachedSurface(p,a)
#define IDirectDrawSurface2_AddOverlayDirtyRect(p,a)     (p)->lpVtbl->AddOverlayDirtyRect(p,a)
#define IDirectDrawSurface2_Blt(p,a,b,c,d,e)             (p)->lpVtbl->Blt(p,a,b,c,d,e)
#define IDirectDrawSurface2_BltBatch(p,a,b,c)            (p)->lpVtbl->BltBatch(p,a,b,c)
#define IDirectDrawSurface2_BltFast(p,a,b,c,d,e)         (p)->lpVtbl->BltFast(p,a,b,c,d,e)
#define IDirectDrawSurface2_DeleteAttachedSurface(p,a,b) (p)->lpVtbl->DeleteAttachedSurface(p,a,b)
#define IDirectDrawSurface2_EnumAttachedSurfaces(p,a,b)  (p)->lpVtbl->EnumAttachedSurfaces(p,a,b)
#define IDirectDrawSurface2_EnumOverlayZOrders(p,a,b,c)  (p)->lpVtbl->EnumOverlayZOrders(p,a,b,c)
#define IDirectDrawSurface2_Flip(p,a,b)                  (p)->lpVtbl->Flip(p,a,b)
#define IDirectDrawSurface2_GetAttachedSurface(p,a,b)    (p)->lpVtbl->GetAttachedSurface(p,a,b)
#define IDirectDrawSurface2_GetBltStatus(p,a)            (p)->lpVtbl->GetBltStatus(p,a)
#define IDirectDrawSurface2_GetCaps(p,b)                 (p)->lpVtbl->GetCaps(p,b)
#define IDirectDrawSurface2_GetClipper(p,a)              (p)->lpVtbl->GetClipper(p,a)
#define IDirectDrawSurface2_GetColorKey(p,a,b)           (p)->lpVtbl->GetColorKey(p,a,b)
#define IDirectDrawSurface2_GetDC(p,a)                   (p)->lpVtbl->GetDC(p,a)
#define IDirectDrawSurface2_GetFlipStatus(p,a)           (p)->lpVtbl->GetFlipStatus(p,a)
#define IDirectDrawSurface2_GetOverlayPosition(p,a,b)    (p)->lpVtbl->GetOverlayPosition(p,a,b)
#define IDirectDrawSurface2_GetPalette(p,a)              (p)->lpVtbl->GetPalette(p,a)
#define IDirectDrawSurface2_GetPixelFormat(p,a)          (p)->lpVtbl->GetPixelFormat(p,a)
#define IDirectDrawSurface2_GetSurfaceDesc(p,a)          (p)->lpVtbl->GetSurfaceDesc(p,a)
#define IDirectDrawSurface2_Initialize(p,a,b)            (p)->lpVtbl->Initialize(p,a,b)
#define IDirectDrawSurface2_IsLost(p)                    (p)->lpVtbl->IsLost(p)
#define IDirectDrawSurface2_Lock(p,a,b,c,d)              (p)->lpVtbl->Lock(p,a,b,c,d)
#define IDirectDrawSurface2_ReleaseDC(p,a)               (p)->lpVtbl->ReleaseDC(p,a)
#define IDirectDrawSurface2_Restore(p)                   (p)->lpVtbl->Restore(p)
#define IDirectDrawSurface2_SetClipper(p,a)              (p)->lpVtbl->SetClipper(p,a)
#define IDirectDrawSurface2_SetColorKey(p,a,b)           (p)->lpVtbl->SetColorKey(p,a,b)
#define IDirectDrawSurface2_SetOverlayPosition(p,a,b)    (p)->lpVtbl->SetOverlayPosition(p,a,b)
#define IDirectDrawSurface2_SetPalette(p,a)              (p)->lpVtbl->SetPalette(p,a)
#define IDirectDrawSurface2_Unlock(p,b)                  (p)->lpVtbl->Unlock(p,b)
#define IDirectDrawSurface2_UpdateOverlay(p,a,b,c,d,e)   (p)->lpVtbl->UpdateOverlay(p,a,b,c,d,e)
#define IDirectDrawSurface2_UpdateOverlayDisplay(p,a)    (p)->lpVtbl->UpdateOverlayDisplay(p,a)
#define IDirectDrawSurface2_UpdateOverlayZOrder(p,a,b)   (p)->lpVtbl->UpdateOverlayZOrder(p,a,b)
#define IDirectDrawSurface2_GetDDInterface(p,a)          (p)->lpVtbl->GetDDInterface(p,a)
#define IDirectDrawSurface2_PageLock(p,a)                (p)->lpVtbl->PageLock(p,a)
#define IDirectDrawSurface2_PageUnlock(p,a)              (p)->lpVtbl->PageUnlock(p,a)
#else
#define IDirectDrawSurface2_QueryInterface(p,a,b)        (p)->QueryInterface(a,b)
#define IDirectDrawSurface2_AddRef(p)                    (p)->AddRef()
#define IDirectDrawSurface2_Release(p)                   (p)->Release()
#define IDirectDrawSurface2_AddAttachedSurface(p,a)      (p)->AddAttachedSurface(a)
#define IDirectDrawSurface2_AddOverlayDirtyRect(p,a)     (p)->AddOverlayDirtyRect(a)
#define IDirectDrawSurface2_Blt(p,a,b,c,d,e)             (p)->Blt(a,b,c,d,e)
#define IDirectDrawSurface2_BltBatch(p,a,b,c)            (p)->BltBatch(a,b,c)
#define IDirectDrawSurface2_BltFast(p,a,b,c,d,e)         (p)->BltFast(a,b,c,d,e)
#define IDirectDrawSurface2_DeleteAttachedSurface(p,a,b) (p)->DeleteAttachedSurface(a,b)
#define IDirectDrawSurface2_EnumAttachedSurfaces(p,a,b)  (p)->EnumAttachedSurfaces(a,b)
#define IDirectDrawSurface2_EnumOverlayZOrders(p,a,b,c)  (p)->EnumOverlayZOrders(a,b,c)
#define IDirectDrawSurface2_Flip(p,a,b)                  (p)->Flip(a,b)
#define IDirectDrawSurface2_GetAttachedSurface(p,a,b)    (p)->GetAttachedSurface(a,b)
#define IDirectDrawSurface2_GetBltStatus(p,a)            (p)->GetBltStatus(a)
#define IDirectDrawSurface2_GetCaps(p,b)                 (p)->GetCaps(b)
#define IDirectDrawSurface2_GetClipper(p,a)              (p)->GetClipper(a)
#define IDirectDrawSurface2_GetColorKey(p,a,b)           (p)->GetColorKey(a,b)
#define IDirectDrawSurface2_GetDC(p,a)                   (p)->GetDC(a)
#define IDirectDrawSurface2_GetFlipStatus(p,a)           (p)->GetFlipStatus(a)
#define IDirectDrawSurface2_GetOverlayPosition(p,a,b)    (p)->GetOverlayPosition(a,b)
#define IDirectDrawSurface2_GetPalette(p,a)              (p)->GetPalette(a)
#define IDirectDrawSurface2_GetPixelFormat(p,a)          (p)->GetPixelFormat(a)
#define IDirectDrawSurface2_GetSurfaceDesc(p,a)          (p)->GetSurfaceDesc(a)
#define IDirectDrawSurface2_Initialize(p,a,b)            (p)->Initialize(a,b)
#define IDirectDrawSurface2_IsLost(p)                    (p)->IsLost()
#define IDirectDrawSurface2_Lock(p,a,b,c,d)              (p)->Lock(a,b,c,d)
#define IDirectDrawSurface2_ReleaseDC(p,a)               (p)->ReleaseDC(a)
#define IDirectDrawSurface2_Restore(p)                   (p)->Restore()
#define IDirectDrawSurface2_SetClipper(p,a)              (p)->SetClipper(a)
#define IDirectDrawSurface2_SetColorKey(p,a,b)           (p)->SetColorKey(a,b)
#define IDirectDrawSurface2_SetOverlayPosition(p,a,b)    (p)->SetOverlayPosition(a,b)
#define IDirectDrawSurface2_SetPalette(p,a)              (p)->SetPalette(a)
#define IDirectDrawSurface2_Unlock(p,b)                  (p)->Unlock(b)
#define IDirectDrawSurface2_UpdateOverlay(p,a,b,c,d,e)   (p)->UpdateOverlay(a,b,c,d,e)
#define IDirectDrawSurface2_UpdateOverlayDisplay(p,a)    (p)->UpdateOverlayDisplay(a)
#define IDirectDrawSurface2_UpdateOverlayZOrder(p,a,b)   (p)->UpdateOverlayZOrder(a,b)
#define IDirectDrawSurface2_GetDDInterface(p,a)          (p)->GetDDInterface(a)
#define IDirectDrawSurface2_PageLock(p,a)                (p)->PageLock(a)
#define IDirectDrawSurface2_PageUnlock(p,a)              (p)->PageUnlock(a)
#endif

/*
 * IDirectDrawSurface3 and related interfaces
 */
#undef INTERFACE
#define INTERFACE IDirectDrawSurface3
DECLARE_INTERFACE_( IDirectDrawSurface3, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDrawSurface methods ***/
    STDMETHOD(AddAttachedSurface)(THIS_ LPDIRECTDRAWSURFACE3) PURE;
    STDMETHOD(AddOverlayDirtyRect)(THIS_ LPRECT) PURE;
    STDMETHOD(Blt)(THIS_ LPRECT,LPDIRECTDRAWSURFACE3, LPRECT,DWORD, LPDDBLTFX) PURE;
    STDMETHOD(BltBatch)(THIS_ LPDDBLTBATCH, DWORD, DWORD ) PURE;
    STDMETHOD(BltFast)(THIS_ DWORD,DWORD,LPDIRECTDRAWSURFACE3, LPRECT,DWORD) PURE;
    STDMETHOD(DeleteAttachedSurface)(THIS_ DWORD,LPDIRECTDRAWSURFACE3) PURE;
    STDMETHOD(EnumAttachedSurfaces)(THIS_ LPVOID,LPDDENUMSURFACESCALLBACK) PURE;
    STDMETHOD(EnumOverlayZOrders)(THIS_ DWORD,LPVOID,LPDDENUMSURFACESCALLBACK) PURE;
    STDMETHOD(Flip)(THIS_ LPDIRECTDRAWSURFACE3, DWORD) PURE;
    STDMETHOD(GetAttachedSurface)(THIS_ LPDDSCAPS, LPDIRECTDRAWSURFACE3 FAR *) PURE;
    STDMETHOD(GetBltStatus)(THIS_ DWORD) PURE;
    STDMETHOD(GetCaps)(THIS_ LPDDSCAPS) PURE;
    STDMETHOD(GetClipper)(THIS_ LPDIRECTDRAWCLIPPER FAR*) PURE;
    STDMETHOD(GetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
    STDMETHOD(GetDC)(THIS_ HDC FAR *) PURE;
    STDMETHOD(GetFlipStatus)(THIS_ DWORD) PURE;
    STDMETHOD(GetOverlayPosition)(THIS_ LPLONG, LPLONG ) PURE;
    STDMETHOD(GetPalette)(THIS_ LPDIRECTDRAWPALETTE FAR*) PURE;
    STDMETHOD(GetPixelFormat)(THIS_ LPDDPIXELFORMAT) PURE;
    STDMETHOD(GetSurfaceDesc)(THIS_ LPDDSURFACEDESC) PURE;
    STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, LPDDSURFACEDESC) PURE;
    STDMETHOD(IsLost)(THIS) PURE;
    STDMETHOD(Lock)(THIS_ LPRECT,LPDDSURFACEDESC,DWORD,HANDLE) PURE;
    STDMETHOD(ReleaseDC)(THIS_ HDC) PURE;
    STDMETHOD(Restore)(THIS) PURE;
    STDMETHOD(SetClipper)(THIS_ LPDIRECTDRAWCLIPPER) PURE;
    STDMETHOD(SetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
    STDMETHOD(SetOverlayPosition)(THIS_ LONG, LONG ) PURE;
    STDMETHOD(SetPalette)(THIS_ LPDIRECTDRAWPALETTE) PURE;
    STDMETHOD(Unlock)(THIS_ LPVOID) PURE;
    STDMETHOD(UpdateOverlay)(THIS_ LPRECT, LPDIRECTDRAWSURFACE3,LPRECT,DWORD, LPDDOVERLAYFX) PURE;
    STDMETHOD(UpdateOverlayDisplay)(THIS_ DWORD) PURE;
    STDMETHOD(UpdateOverlayZOrder)(THIS_ DWORD, LPDIRECTDRAWSURFACE3) PURE;
    /*** Added in the v2 interface ***/
    STDMETHOD(GetDDInterface)(THIS_ LPVOID FAR *) PURE;
    STDMETHOD(PageLock)(THIS_ DWORD) PURE;
    STDMETHOD(PageUnlock)(THIS_ DWORD) PURE;
    /*** Added in the V3 interface ***/
    STDMETHOD(SetSurfaceDesc)(THIS_ LPDDSURFACEDESC, DWORD) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectDrawSurface3_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectDrawSurface3_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IDirectDrawSurface3_Release(p)                   (p)->lpVtbl->Release(p)
#define IDirectDrawSurface3_AddAttachedSurface(p,a)      (p)->lpVtbl->AddAttachedSurface(p,a)
#define IDirectDrawSurface3_AddOverlayDirtyRect(p,a)     (p)->lpVtbl->AddOverlayDirtyRect(p,a)
#define IDirectDrawSurface3_Blt(p,a,b,c,d,e)             (p)->lpVtbl->Blt(p,a,b,c,d,e)
#define IDirectDrawSurface3_BltBatch(p,a,b,c)            (p)->lpVtbl->BltBatch(p,a,b,c)
#define IDirectDrawSurface3_BltFast(p,a,b,c,d,e)         (p)->lpVtbl->BltFast(p,a,b,c,d,e)
#define IDirectDrawSurface3_DeleteAttachedSurface(p,a,b) (p)->lpVtbl->DeleteAttachedSurface(p,a,b)
#define IDirectDrawSurface3_EnumAttachedSurfaces(p,a,b)  (p)->lpVtbl->EnumAttachedSurfaces(p,a,b)
#define IDirectDrawSurface3_EnumOverlayZOrders(p,a,b,c)  (p)->lpVtbl->EnumOverlayZOrders(p,a,b,c)
#define IDirectDrawSurface3_Flip(p,a,b)                  (p)->lpVtbl->Flip(p,a,b)
#define IDirectDrawSurface3_GetAttachedSurface(p,a,b)    (p)->lpVtbl->GetAttachedSurface(p,a,b)
#define IDirectDrawSurface3_GetBltStatus(p,a)            (p)->lpVtbl->GetBltStatus(p,a)
#define IDirectDrawSurface3_GetCaps(p,b)                 (p)->lpVtbl->GetCaps(p,b)
#define IDirectDrawSurface3_GetClipper(p,a)              (p)->lpVtbl->GetClipper(p,a)
#define IDirectDrawSurface3_GetColorKey(p,a,b)           (p)->lpVtbl->GetColorKey(p,a,b)
#define IDirectDrawSurface3_GetDC(p,a)                   (p)->lpVtbl->GetDC(p,a)
#define IDirectDrawSurface3_GetFlipStatus(p,a)           (p)->lpVtbl->GetFlipStatus(p,a)
#define IDirectDrawSurface3_GetOverlayPosition(p,a,b)    (p)->lpVtbl->GetOverlayPosition(p,a,b)
#define IDirectDrawSurface3_GetPalette(p,a)              (p)->lpVtbl->GetPalette(p,a)
#define IDirectDrawSurface3_GetPixelFormat(p,a)          (p)->lpVtbl->GetPixelFormat(p,a)
#define IDirectDrawSurface3_GetSurfaceDesc(p,a)          (p)->lpVtbl->GetSurfaceDesc(p,a)
#define IDirectDrawSurface3_Initialize(p,a,b)            (p)->lpVtbl->Initialize(p,a,b)
#define IDirectDrawSurface3_IsLost(p)                    (p)->lpVtbl->IsLost(p)
#define IDirectDrawSurface3_Lock(p,a,b,c,d)              (p)->lpVtbl->Lock(p,a,b,c,d)
#define IDirectDrawSurface3_ReleaseDC(p,a)               (p)->lpVtbl->ReleaseDC(p,a)
#define IDirectDrawSurface3_Restore(p)                   (p)->lpVtbl->Restore(p)
#define IDirectDrawSurface3_SetClipper(p,a)              (p)->lpVtbl->SetClipper(p,a)
#define IDirectDrawSurface3_SetColorKey(p,a,b)           (p)->lpVtbl->SetColorKey(p,a,b)
#define IDirectDrawSurface3_SetOverlayPosition(p,a,b)    (p)->lpVtbl->SetOverlayPosition(p,a,b)
#define IDirectDrawSurface3_SetPalette(p,a)              (p)->lpVtbl->SetPalette(p,a)
#define IDirectDrawSurface3_Unlock(p,b)                  (p)->lpVtbl->Unlock(p,b)
#define IDirectDrawSurface3_UpdateOverlay(p,a,b,c,d,e)   (p)->lpVtbl->UpdateOverlay(p,a,b,c,d,e)
#define IDirectDrawSurface3_UpdateOverlayDisplay(p,a)    (p)->lpVtbl->UpdateOverlayDisplay(p,a)
#define IDirectDrawSurface3_UpdateOverlayZOrder(p,a,b)   (p)->lpVtbl->UpdateOverlayZOrder(p,a,b)
#define IDirectDrawSurface3_GetDDInterface(p,a)          (p)->lpVtbl->GetDDInterface(p,a)
#define IDirectDrawSurface3_PageLock(p,a)                (p)->lpVtbl->PageLock(p,a)
#define IDirectDrawSurface3_PageUnlock(p,a)              (p)->lpVtbl->PageUnlock(p,a)
#define IDirectDrawSurface3_SetSurfaceDesc(p,a,b)        (p)->lpVtbl->SetSurfaceDesc(p,a,b)
#else
#define IDirectDrawSurface3_QueryInterface(p,a,b)        (p)->QueryInterface(a,b)
#define IDirectDrawSurface3_AddRef(p)                    (p)->AddRef()
#define IDirectDrawSurface3_Release(p)                   (p)->Release()
#define IDirectDrawSurface3_AddAttachedSurface(p,a)      (p)->AddAttachedSurface(a)
#define IDirectDrawSurface3_AddOverlayDirtyRect(p,a)     (p)->AddOverlayDirtyRect(a)
#define IDirectDrawSurface3_Blt(p,a,b,c,d,e)             (p)->Blt(a,b,c,d,e)
#define IDirectDrawSurface3_BltBatch(p,a,b,c)            (p)->BltBatch(a,b,c)
#define IDirectDrawSurface3_BltFast(p,a,b,c,d,e)         (p)->BltFast(a,b,c,d,e)
#define IDirectDrawSurface3_DeleteAttachedSurface(p,a,b) (p)->DeleteAttachedSurface(a,b)
#define IDirectDrawSurface3_EnumAttachedSurfaces(p,a,b)  (p)->EnumAttachedSurfaces(a,b)
#define IDirectDrawSurface3_EnumOverlayZOrders(p,a,b,c)  (p)->EnumOverlayZOrders(a,b,c)
#define IDirectDrawSurface3_Flip(p,a,b)                  (p)->Flip(a,b)
#define IDirectDrawSurface3_GetAttachedSurface(p,a,b)    (p)->GetAttachedSurface(a,b)
#define IDirectDrawSurface3_GetBltStatus(p,a)            (p)->GetBltStatus(a)
#define IDirectDrawSurface3_GetCaps(p,b)                 (p)->GetCaps(b)
#define IDirectDrawSurface3_GetClipper(p,a)              (p)->GetClipper(a)
#define IDirectDrawSurface3_GetColorKey(p,a,b)           (p)->GetColorKey(a,b)
#define IDirectDrawSurface3_GetDC(p,a)                   (p)->GetDC(a)
#define IDirectDrawSurface3_GetFlipStatus(p,a)           (p)->GetFlipStatus(a)
#define IDirectDrawSurface3_GetOverlayPosition(p,a,b)    (p)->GetOverlayPosition(a,b)
#define IDirectDrawSurface3_GetPalette(p,a)              (p)->GetPalette(a)
#define IDirectDrawSurface3_GetPixelFormat(p,a)          (p)->GetPixelFormat(a)
#define IDirectDrawSurface3_GetSurfaceDesc(p,a)          (p)->GetSurfaceDesc(a)
#define IDirectDrawSurface3_Initialize(p,a,b)            (p)->Initialize(a,b)
#define IDirectDrawSurface3_IsLost(p)                    (p)->IsLost()
#define IDirectDrawSurface3_Lock(p,a,b,c,d)              (p)->Lock(a,b,c,d)
#define IDirectDrawSurface3_ReleaseDC(p,a)               (p)->ReleaseDC(a)
#define IDirectDrawSurface3_Restore(p)                   (p)->Restore()
#define IDirectDrawSurface3_SetClipper(p,a)              (p)->SetClipper(a)
#define IDirectDrawSurface3_SetColorKey(p,a,b)           (p)->SetColorKey(a,b)
#define IDirectDrawSurface3_SetOverlayPosition(p,a,b)    (p)->SetOverlayPosition(a,b)
#define IDirectDrawSurface3_SetPalette(p,a)              (p)->SetPalette(a)
#define IDirectDrawSurface3_Unlock(p,b)                  (p)->Unlock(b)
#define IDirectDrawSurface3_UpdateOverlay(p,a,b,c,d,e)   (p)->UpdateOverlay(a,b,c,d,e)
#define IDirectDrawSurface3_UpdateOverlayDisplay(p,a)    (p)->UpdateOverlayDisplay(a)
#define IDirectDrawSurface3_UpdateOverlayZOrder(p,a,b)   (p)->UpdateOverlayZOrder(a,b)
#define IDirectDrawSurface3_GetDDInterface(p,a)          (p)->GetDDInterface(a)
#define IDirectDrawSurface3_PageLock(p,a)                (p)->PageLock(a)
#define IDirectDrawSurface3_PageUnlock(p,a)              (p)->PageUnlock(a)
#define IDirectDrawSurface3_SetSurfaceDesc(p,a,b)        (p)->SetSurfaceDesc(a,b)
#endif

/*
 * IDirectDrawSurface4 and related interfaces
 */
#undef INTERFACE
#define INTERFACE IDirectDrawSurface4
DECLARE_INTERFACE_( IDirectDrawSurface4, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDrawSurface methods ***/
    STDMETHOD(AddAttachedSurface)(THIS_ LPDIRECTDRAWSURFACE4) PURE;
    STDMETHOD(AddOverlayDirtyRect)(THIS_ LPRECT) PURE;
    STDMETHOD(Blt)(THIS_ LPRECT,LPDIRECTDRAWSURFACE4, LPRECT,DWORD, LPDDBLTFX) PURE;
    STDMETHOD(BltBatch)(THIS_ LPDDBLTBATCH, DWORD, DWORD ) PURE;
    STDMETHOD(BltFast)(THIS_ DWORD,DWORD,LPDIRECTDRAWSURFACE4, LPRECT,DWORD) PURE;
    STDMETHOD(DeleteAttachedSurface)(THIS_ DWORD,LPDIRECTDRAWSURFACE4) PURE;
    STDMETHOD(EnumAttachedSurfaces)(THIS_ LPVOID,LPDDENUMSURFACESCALLBACK2) PURE;
    STDMETHOD(EnumOverlayZOrders)(THIS_ DWORD,LPVOID,LPDDENUMSURFACESCALLBACK2) PURE;
    STDMETHOD(Flip)(THIS_ LPDIRECTDRAWSURFACE4, DWORD) PURE;
    STDMETHOD(GetAttachedSurface)(THIS_ LPDDSCAPS2, LPDIRECTDRAWSURFACE4 FAR *) PURE;
    STDMETHOD(GetBltStatus)(THIS_ DWORD) PURE;
    STDMETHOD(GetCaps)(THIS_ LPDDSCAPS2) PURE;
    STDMETHOD(GetClipper)(THIS_ LPDIRECTDRAWCLIPPER FAR*) PURE;
    STDMETHOD(GetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
    STDMETHOD(GetDC)(THIS_ HDC FAR *) PURE;
    STDMETHOD(GetFlipStatus)(THIS_ DWORD) PURE;
    STDMETHOD(GetOverlayPosition)(THIS_ LPLONG, LPLONG ) PURE;
    STDMETHOD(GetPalette)(THIS_ LPDIRECTDRAWPALETTE FAR*) PURE;
    STDMETHOD(GetPixelFormat)(THIS_ LPDDPIXELFORMAT) PURE;
    STDMETHOD(GetSurfaceDesc)(THIS_ LPDDSURFACEDESC2) PURE;
    STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, LPDDSURFACEDESC2) PURE;
    STDMETHOD(IsLost)(THIS) PURE;
    STDMETHOD(Lock)(THIS_ LPRECT,LPDDSURFACEDESC2,DWORD,HANDLE) PURE;
    STDMETHOD(ReleaseDC)(THIS_ HDC) PURE;
    STDMETHOD(Restore)(THIS) PURE;
    STDMETHOD(SetClipper)(THIS_ LPDIRECTDRAWCLIPPER) PURE;
    STDMETHOD(SetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
    STDMETHOD(SetOverlayPosition)(THIS_ LONG, LONG ) PURE;
    STDMETHOD(SetPalette)(THIS_ LPDIRECTDRAWPALETTE) PURE;
    STDMETHOD(Unlock)(THIS_ LPRECT) PURE;
    STDMETHOD(UpdateOverlay)(THIS_ LPRECT, LPDIRECTDRAWSURFACE4,LPRECT,DWORD, LPDDOVERLAYFX) PURE;
    STDMETHOD(UpdateOverlayDisplay)(THIS_ DWORD) PURE;
    STDMETHOD(UpdateOverlayZOrder)(THIS_ DWORD, LPDIRECTDRAWSURFACE4) PURE;
    /*** Added in the v2 interface ***/
    STDMETHOD(GetDDInterface)(THIS_ LPVOID FAR *) PURE;
    STDMETHOD(PageLock)(THIS_ DWORD) PURE;
    STDMETHOD(PageUnlock)(THIS_ DWORD) PURE;
    /*** Added in the v3 interface ***/
    STDMETHOD(SetSurfaceDesc)(THIS_ LPDDSURFACEDESC2, DWORD) PURE;
    /*** Added in the v4 interface ***/
    STDMETHOD(SetPrivateData)(THIS_ REFGUID, LPVOID, DWORD, DWORD) PURE;
    STDMETHOD(GetPrivateData)(THIS_ REFGUID, LPVOID, LPDWORD) PURE;
    STDMETHOD(FreePrivateData)(THIS_ REFGUID) PURE;
    STDMETHOD(GetUniquenessValue)(THIS_ LPDWORD) PURE;
    STDMETHOD(ChangeUniquenessValue)(THIS) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectDrawSurface4_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectDrawSurface4_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IDirectDrawSurface4_Release(p)                   (p)->lpVtbl->Release(p)
#define IDirectDrawSurface4_AddAttachedSurface(p,a)      (p)->lpVtbl->AddAttachedSurface(p,a)
#define IDirectDrawSurface4_AddOverlayDirtyRect(p,a)     (p)->lpVtbl->AddOverlayDirtyRect(p,a)
#define IDirectDrawSurface4_Blt(p,a,b,c,d,e)             (p)->lpVtbl->Blt(p,a,b,c,d,e)
#define IDirectDrawSurface4_BltBatch(p,a,b,c)            (p)->lpVtbl->BltBatch(p,a,b,c)
#define IDirectDrawSurface4_BltFast(p,a,b,c,d,e)         (p)->lpVtbl->BltFast(p,a,b,c,d,e)
#define IDirectDrawSurface4_DeleteAttachedSurface(p,a,b) (p)->lpVtbl->DeleteAttachedSurface(p,a,b)
#define IDirectDrawSurface4_EnumAttachedSurfaces(p,a,b)  (p)->lpVtbl->EnumAttachedSurfaces(p,a,b)
#define IDirectDrawSurface4_EnumOverlayZOrders(p,a,b,c)  (p)->lpVtbl->EnumOverlayZOrders(p,a,b,c)
#define IDirectDrawSurface4_Flip(p,a,b)                  (p)->lpVtbl->Flip(p,a,b)
#define IDirectDrawSurface4_GetAttachedSurface(p,a,b)    (p)->lpVtbl->GetAttachedSurface(p,a,b)
#define IDirectDrawSurface4_GetBltStatus(p,a)            (p)->lpVtbl->GetBltStatus(p,a)
#define IDirectDrawSurface4_GetCaps(p,b)                 (p)->lpVtbl->GetCaps(p,b)
#define IDirectDrawSurface4_GetClipper(p,a)              (p)->lpVtbl->GetClipper(p,a)
#define IDirectDrawSurface4_GetColorKey(p,a,b)           (p)->lpVtbl->GetColorKey(p,a,b)
#define IDirectDrawSurface4_GetDC(p,a)                   (p)->lpVtbl->GetDC(p,a)
#define IDirectDrawSurface4_GetFlipStatus(p,a)           (p)->lpVtbl->GetFlipStatus(p,a)
#define IDirectDrawSurface4_GetOverlayPosition(p,a,b)    (p)->lpVtbl->GetOverlayPosition(p,a,b)
#define IDirectDrawSurface4_GetPalette(p,a)              (p)->lpVtbl->GetPalette(p,a)
#define IDirectDrawSurface4_GetPixelFormat(p,a)          (p)->lpVtbl->GetPixelFormat(p,a)
#define IDirectDrawSurface4_GetSurfaceDesc(p,a)          (p)->lpVtbl->GetSurfaceDesc(p,a)
#define IDirectDrawSurface4_Initialize(p,a,b)            (p)->lpVtbl->Initialize(p,a,b)
#define IDirectDrawSurface4_IsLost(p)                    (p)->lpVtbl->IsLost(p)
#define IDirectDrawSurface4_Lock(p,a,b,c,d)              (p)->lpVtbl->Lock(p,a,b,c,d)
#define IDirectDrawSurface4_ReleaseDC(p,a)               (p)->lpVtbl->ReleaseDC(p,a)
#define IDirectDrawSurface4_Restore(p)                   (p)->lpVtbl->Restore(p)
#define IDirectDrawSurface4_SetClipper(p,a)              (p)->lpVtbl->SetClipper(p,a)
#define IDirectDrawSurface4_SetColorKey(p,a,b)           (p)->lpVtbl->SetColorKey(p,a,b)
#define IDirectDrawSurface4_SetOverlayPosition(p,a,b)    (p)->lpVtbl->SetOverlayPosition(p,a,b)
#define IDirectDrawSurface4_SetPalette(p,a)              (p)->lpVtbl->SetPalette(p,a)
#define IDirectDrawSurface4_Unlock(p,b)                  (p)->lpVtbl->Unlock(p,b)
#define IDirectDrawSurface4_UpdateOverlay(p,a,b,c,d,e)   (p)->lpVtbl->UpdateOverlay(p,a,b,c,d,e)
#define IDirectDrawSurface4_UpdateOverlayDisplay(p,a)    (p)->lpVtbl->UpdateOverlayDisplay(p,a)
#define IDirectDrawSurface4_UpdateOverlayZOrder(p,a,b)   (p)->lpVtbl->UpdateOverlayZOrder(p,a,b)
#define IDirectDrawSurface4_GetDDInterface(p,a)          (p)->lpVtbl->GetDDInterface(p,a)
#define IDirectDrawSurface4_PageLock(p,a)                (p)->lpVtbl->PageLock(p,a)
#define IDirectDrawSurface4_PageUnlock(p,a)              (p)->lpVtbl->PageUnlock(p,a)
#define IDirectDrawSurface4_SetSurfaceDesc(p,a,b)        (p)->lpVtbl->SetSurfaceDesc(p,a,b)
#define IDirectDrawSurface4_SetPrivateData(p,a,b,c,d)    (p)->lpVtbl->SetPrivateData(p,a,b,c,d)
#define IDirectDrawSurface4_GetPrivateData(p,a,b,c)      (p)->lpVtbl->GetPrivateData(p,a,b,c)
#define IDirectDrawSurface4_FreePrivateData(p,a)         (p)->lpVtbl->FreePrivateData(p,a)
#define IDirectDrawSurface4_GetUniquenessValue(p, a)     (p)->lpVtbl->GetUniquenessValue(p, a)
#define IDirectDrawSurface4_ChangeUniquenessValue(p)     (p)->lpVtbl->ChangeUniquenessValue(p)
#else
#define IDirectDrawSurface4_QueryInterface(p,a,b)        (p)->QueryInterface(a,b)
#define IDirectDrawSurface4_AddRef(p)                    (p)->AddRef()
#define IDirectDrawSurface4_Release(p)                   (p)->Release()
#define IDirectDrawSurface4_AddAttachedSurface(p,a)      (p)->AddAttachedSurface(a)
#define IDirectDrawSurface4_AddOverlayDirtyRect(p,a)     (p)->AddOverlayDirtyRect(a)
#define IDirectDrawSurface4_Blt(p,a,b,c,d,e)             (p)->Blt(a,b,c,d,e)
#define IDirectDrawSurface4_BltBatch(p,a,b,c)            (p)->BltBatch(a,b,c)
#define IDirectDrawSurface4_BltFast(p,a,b,c,d,e)         (p)->BltFast(a,b,c,d,e)
#define IDirectDrawSurface4_DeleteAttachedSurface(p,a,b) (p)->DeleteAttachedSurface(a,b)
#define IDirectDrawSurface4_EnumAttachedSurfaces(p,a,b)  (p)->EnumAttachedSurfaces(a,b)
#define IDirectDrawSurface4_EnumOverlayZOrders(p,a,b,c)  (p)->EnumOverlayZOrders(a,b,c)
#define IDirectDrawSurface4_Flip(p,a,b)                  (p)->Flip(a,b)
#define IDirectDrawSurface4_GetAttachedSurface(p,a,b)    (p)->GetAttachedSurface(a,b)
#define IDirectDrawSurface4_GetBltStatus(p,a)            (p)->GetBltStatus(a)
#define IDirectDrawSurface4_GetCaps(p,b)                 (p)->GetCaps(b)
#define IDirectDrawSurface4_GetClipper(p,a)              (p)->GetClipper(a)
#define IDirectDrawSurface4_GetColorKey(p,a,b)           (p)->GetColorKey(a,b)
#define IDirectDrawSurface4_GetDC(p,a)                   (p)->GetDC(a)
#define IDirectDrawSurface4_GetFlipStatus(p,a)           (p)->GetFlipStatus(a)
#define IDirectDrawSurface4_GetOverlayPosition(p,a,b)    (p)->GetOverlayPosition(a,b)
#define IDirectDrawSurface4_GetPalette(p,a)              (p)->GetPalette(a)
#define IDirectDrawSurface4_GetPixelFormat(p,a)          (p)->GetPixelFormat(a)
#define IDirectDrawSurface4_GetSurfaceDesc(p,a)          (p)->GetSurfaceDesc(a)
#define IDirectDrawSurface4_Initialize(p,a,b)            (p)->Initialize(a,b)
#define IDirectDrawSurface4_IsLost(p)                    (p)->IsLost()
#define IDirectDrawSurface4_Lock(p,a,b,c,d)              (p)->Lock(a,b,c,d)
#define IDirectDrawSurface4_ReleaseDC(p,a)               (p)->ReleaseDC(a)
#define IDirectDrawSurface4_Restore(p)                   (p)->Restore()
#define IDirectDrawSurface4_SetClipper(p,a)              (p)->SetClipper(a)
#define IDirectDrawSurface4_SetColorKey(p,a,b)           (p)->SetColorKey(a,b)
#define IDirectDrawSurface4_SetOverlayPosition(p,a,b)    (p)->SetOverlayPosition(a,b)
#define IDirectDrawSurface4_SetPalette(p,a)              (p)->SetPalette(a)
#define IDirectDrawSurface4_Unlock(p,b)                  (p)->Unlock(b)
#define IDirectDrawSurface4_UpdateOverlay(p,a,b,c,d,e)   (p)->UpdateOverlay(a,b,c,d,e)
#define IDirectDrawSurface4_UpdateOverlayDisplay(p,a)    (p)->UpdateOverlayDisplay(a)
#define IDirectDrawSurface4_UpdateOverlayZOrder(p,a,b)   (p)->UpdateOverlayZOrder(a,b)
#define IDirectDrawSurface4_GetDDInterface(p,a)          (p)->GetDDInterface(a)
#define IDirectDrawSurface4_PageLock(p,a)                (p)->PageLock(a)
#define IDirectDrawSurface4_PageUnlock(p,a)              (p)->PageUnlock(a)
#define IDirectDrawSurface4_SetSurfaceDesc(p,a,b)        (p)->SetSurfaceDesc(a,b)
#define IDirectDrawSurface4_SetPrivateData(p,a,b,c,d)    (p)->SetPrivateData(a,b,c,d)
#define IDirectDrawSurface4_GetPrivateData(p,a,b,c)      (p)->GetPrivateData(a,b,c)
#define IDirectDrawSurface4_FreePrivateData(p,a)         (p)->FreePrivateData(a)
#define IDirectDrawSurface4_GetUniquenessValue(p, a)     (p)->GetUniquenessValue(a)
#define IDirectDrawSurface4_ChangeUniquenessValue(p)     (p)->ChangeUniquenessValue()
#endif

/*
 * IDirectDrawSurface7 and related interfaces
 */
#undef INTERFACE
#define INTERFACE IDirectDrawSurface7
DECLARE_INTERFACE_( IDirectDrawSurface7, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDrawSurface methods ***/
    STDMETHOD(AddAttachedSurface)(THIS_ LPDIRECTDRAWSURFACE7) PURE;
    STDMETHOD(AddOverlayDirtyRect)(THIS_ LPRECT) PURE;
    STDMETHOD(Blt)(THIS_ LPRECT,LPDIRECTDRAWSURFACE7, LPRECT,DWORD, LPDDBLTFX) PURE;
    STDMETHOD(BltBatch)(THIS_ LPDDBLTBATCH, DWORD, DWORD ) PURE;
    STDMETHOD(BltFast)(THIS_ DWORD,DWORD,LPDIRECTDRAWSURFACE7, LPRECT,DWORD) PURE;
    STDMETHOD(DeleteAttachedSurface)(THIS_ DWORD,LPDIRECTDRAWSURFACE7) PURE;
    STDMETHOD(EnumAttachedSurfaces)(THIS_ LPVOID,LPDDENUMSURFACESCALLBACK7) PURE;
    STDMETHOD(EnumOverlayZOrders)(THIS_ DWORD,LPVOID,LPDDENUMSURFACESCALLBACK7) PURE;
    STDMETHOD(Flip)(THIS_ LPDIRECTDRAWSURFACE7, DWORD) PURE;
    STDMETHOD(GetAttachedSurface)(THIS_ LPDDSCAPS2, LPDIRECTDRAWSURFACE7 FAR *) PURE;
    STDMETHOD(GetBltStatus)(THIS_ DWORD) PURE;
    STDMETHOD(GetCaps)(THIS_ LPDDSCAPS2) PURE;
    STDMETHOD(GetClipper)(THIS_ LPDIRECTDRAWCLIPPER FAR*) PURE;
    STDMETHOD(GetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
    STDMETHOD(GetDC)(THIS_ HDC FAR *) PURE;
    STDMETHOD(GetFlipStatus)(THIS_ DWORD) PURE;
    STDMETHOD(GetOverlayPosition)(THIS_ LPLONG, LPLONG ) PURE;
    STDMETHOD(GetPalette)(THIS_ LPDIRECTDRAWPALETTE FAR*) PURE;
    STDMETHOD(GetPixelFormat)(THIS_ LPDDPIXELFORMAT) PURE;
    STDMETHOD(GetSurfaceDesc)(THIS_ LPDDSURFACEDESC2) PURE;
    STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, LPDDSURFACEDESC2) PURE;
    STDMETHOD(IsLost)(THIS) PURE;
    STDMETHOD(Lock)(THIS_ LPRECT,LPDDSURFACEDESC2,DWORD,HANDLE) PURE;
    STDMETHOD(ReleaseDC)(THIS_ HDC) PURE;
    STDMETHOD(Restore)(THIS) PURE;
    STDMETHOD(SetClipper)(THIS_ LPDIRECTDRAWCLIPPER) PURE;
    STDMETHOD(SetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
    STDMETHOD(SetOverlayPosition)(THIS_ LONG, LONG ) PURE;
    STDMETHOD(SetPalette)(THIS_ LPDIRECTDRAWPALETTE) PURE;
    STDMETHOD(Unlock)(THIS_ LPRECT) PURE;
    STDMETHOD(UpdateOverlay)(THIS_ LPRECT, LPDIRECTDRAWSURFACE7,LPRECT,DWORD, LPDDOVERLAYFX) PURE;
    STDMETHOD(UpdateOverlayDisplay)(THIS_ DWORD) PURE;
    STDMETHOD(UpdateOverlayZOrder)(THIS_ DWORD, LPDIRECTDRAWSURFACE7) PURE;
    /*** Added in the v2 interface ***/
    STDMETHOD(GetDDInterface)(THIS_ LPVOID FAR *) PURE;
    STDMETHOD(PageLock)(THIS_ DWORD) PURE;
    STDMETHOD(PageUnlock)(THIS_ DWORD) PURE;
    /*** Added in the v3 interface ***/
    STDMETHOD(SetSurfaceDesc)(THIS_ LPDDSURFACEDESC2, DWORD) PURE;
    /*** Added in the v4 interface ***/
    STDMETHOD(SetPrivateData)(THIS_ REFGUID, LPVOID, DWORD, DWORD) PURE;
    STDMETHOD(GetPrivateData)(THIS_ REFGUID, LPVOID, LPDWORD) PURE;
    STDMETHOD(FreePrivateData)(THIS_ REFGUID) PURE;
    STDMETHOD(GetUniquenessValue)(THIS_ LPDWORD) PURE;
    STDMETHOD(ChangeUniquenessValue)(THIS) PURE;
    /*** Moved Texture7 methods here ***/
    STDMETHOD(SetPriority)(THIS_ DWORD) PURE;
    STDMETHOD(GetPriority)(THIS_ LPDWORD) PURE;
    STDMETHOD(SetLOD)(THIS_ DWORD) PURE;
    STDMETHOD(GetLOD)(THIS_ LPDWORD) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectDrawSurface7_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectDrawSurface7_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IDirectDrawSurface7_Release(p)                   (p)->lpVtbl->Release(p)
#define IDirectDrawSurface7_AddAttachedSurface(p,a)      (p)->lpVtbl->AddAttachedSurface(p,a)
#define IDirectDrawSurface7_AddOverlayDirtyRect(p,a)     (p)->lpVtbl->AddOverlayDirtyRect(p,a)
#define IDirectDrawSurface7_Blt(p,a,b,c,d,e)             (p)->lpVtbl->Blt(p,a,b,c,d,e)
#define IDirectDrawSurface7_BltBatch(p,a,b,c)            (p)->lpVtbl->BltBatch(p,a,b,c)
#define IDirectDrawSurface7_BltFast(p,a,b,c,d,e)         (p)->lpVtbl->BltFast(p,a,b,c,d,e)
#define IDirectDrawSurface7_DeleteAttachedSurface(p,a,b) (p)->lpVtbl->DeleteAttachedSurface(p,a,b)
#define IDirectDrawSurface7_EnumAttachedSurfaces(p,a,b)  (p)->lpVtbl->EnumAttachedSurfaces(p,a,b)
#define IDirectDrawSurface7_EnumOverlayZOrders(p,a,b,c)  (p)->lpVtbl->EnumOverlayZOrders(p,a,b,c)
#define IDirectDrawSurface7_Flip(p,a,b)                  (p)->lpVtbl->Flip(p,a,b)
#define IDirectDrawSurface7_GetAttachedSurface(p,a,b)    (p)->lpVtbl->GetAttachedSurface(p,a,b)
#define IDirectDrawSurface7_GetBltStatus(p,a)            (p)->lpVtbl->GetBltStatus(p,a)
#define IDirectDrawSurface7_GetCaps(p,b)                 (p)->lpVtbl->GetCaps(p,b)
#define IDirectDrawSurface7_GetClipper(p,a)              (p)->lpVtbl->GetClipper(p,a)
#define IDirectDrawSurface7_GetColorKey(p,a,b)           (p)->lpVtbl->GetColorKey(p,a,b)
#define IDirectDrawSurface7_GetDC(p,a)                   (p)->lpVtbl->GetDC(p,a)
#define IDirectDrawSurface7_GetFlipStatus(p,a)           (p)->lpVtbl->GetFlipStatus(p,a)
#define IDirectDrawSurface7_GetOverlayPosition(p,a,b)    (p)->lpVtbl->GetOverlayPosition(p,a,b)
#define IDirectDrawSurface7_GetPalette(p,a)              (p)->lpVtbl->GetPalette(p,a)
#define IDirectDrawSurface7_GetPixelFormat(p,a)          (p)->lpVtbl->GetPixelFormat(p,a)
#define IDirectDrawSurface7_GetSurfaceDesc(p,a)          (p)->lpVtbl->GetSurfaceDesc(p,a)
#define IDirectDrawSurface7_Initialize(p,a,b)            (p)->lpVtbl->Initialize(p,a,b)
#define IDirectDrawSurface7_IsLost(p)                    (p)->lpVtbl->IsLost(p)
#define IDirectDrawSurface7_Lock(p,a,b,c,d)              (p)->lpVtbl->Lock(p,a,b,c,d)
#define IDirectDrawSurface7_ReleaseDC(p,a)               (p)->lpVtbl->ReleaseDC(p,a)
#define IDirectDrawSurface7_Restore(p)                   (p)->lpVtbl->Restore(p)
#define IDirectDrawSurface7_SetClipper(p,a)              (p)->lpVtbl->SetClipper(p,a)
#define IDirectDrawSurface7_SetColorKey(p,a,b)           (p)->lpVtbl->SetColorKey(p,a,b)
#define IDirectDrawSurface7_SetOverlayPosition(p,a,b)    (p)->lpVtbl->SetOverlayPosition(p,a,b)
#define IDirectDrawSurface7_SetPalette(p,a)              (p)->lpVtbl->SetPalette(p,a)
#define IDirectDrawSurface7_Unlock(p,b)                  (p)->lpVtbl->Unlock(p,b)
#define IDirectDrawSurface7_UpdateOverlay(p,a,b,c,d,e)   (p)->lpVtbl->UpdateOverlay(p,a,b,c,d,e)
#define IDirectDrawSurface7_UpdateOverlayDisplay(p,a)    (p)->lpVtbl->UpdateOverlayDisplay(p,a)
#define IDirectDrawSurface7_UpdateOverlayZOrder(p,a,b)   (p)->lpVtbl->UpdateOverlayZOrder(p,a,b)
#define IDirectDrawSurface7_GetDDInterface(p,a)          (p)->lpVtbl->GetDDInterface(p,a)
#define IDirectDrawSurface7_PageLock(p,a)                (p)->lpVtbl->PageLock(p,a)
#define IDirectDrawSurface7_PageUnlock(p,a)              (p)->lpVtbl->PageUnlock(p,a)
#define IDirectDrawSurface7_SetSurfaceDesc(p,a,b)        (p)->lpVtbl->SetSurfaceDesc(p,a,b)
#define IDirectDrawSurface7_SetPrivateData(p,a,b,c,d)    (p)->lpVtbl->SetPrivateData(p,a,b,c,d)
#define IDirectDrawSurface7_GetPrivateData(p,a,b,c)      (p)->lpVtbl->GetPrivateData(p,a,b,c)
#define IDirectDrawSurface7_FreePrivateData(p,a)         (p)->lpVtbl->FreePrivateData(p,a)
#define IDirectDrawSurface7_GetUniquenessValue(p, a)     (p)->lpVtbl->GetUniquenessValue(p, a)
#define IDirectDrawSurface7_ChangeUniquenessValue(p)     (p)->lpVtbl->ChangeUniquenessValue(p)
#define IDirectDrawSurface7_SetPriority(p,a)             (p)->lpVtbl->SetPriority(p,a)
#define IDirectDrawSurface7_GetPriority(p,a)             (p)->lpVtbl->GetPriority(p,a)
#define IDirectDrawSurface7_SetLOD(p,a)                  (p)->lpVtbl->SetLOD(p,a)
#define IDirectDrawSurface7_GetLOD(p,a)                  (p)->lpVtbl->GetLOD(p,a)
#else
#define IDirectDrawSurface7_QueryInterface(p,a,b)        (p)->QueryInterface(a,b)
#define IDirectDrawSurface7_AddRef(p)                    (p)->AddRef()
#define IDirectDrawSurface7_Release(p)                   (p)->Release()
#define IDirectDrawSurface7_AddAttachedSurface(p,a)      (p)->AddAttachedSurface(a)
#define IDirectDrawSurface7_AddOverlayDirtyRect(p,a)     (p)->AddOverlayDirtyRect(a)
#define IDirectDrawSurface7_Blt(p,a,b,c,d,e)             (p)->Blt(a,b,c,d,e)
#define IDirectDrawSurface7_BltBatch(p,a,b,c)            (p)->BltBatch(a,b,c)
#define IDirectDrawSurface7_BltFast(p,a,b,c,d,e)         (p)->BltFast(a,b,c,d,e)
#define IDirectDrawSurface7_DeleteAttachedSurface(p,a,b) (p)->DeleteAttachedSurface(a,b)
#define IDirectDrawSurface7_EnumAttachedSurfaces(p,a,b)  (p)->EnumAttachedSurfaces(a,b)
#define IDirectDrawSurface7_EnumOverlayZOrders(p,a,b,c)  (p)->EnumOverlayZOrders(a,b,c)
#define IDirectDrawSurface7_Flip(p,a,b)                  (p)->Flip(a,b)
#define IDirectDrawSurface7_GetAttachedSurface(p,a,b)    (p)->GetAttachedSurface(a,b)
#define IDirectDrawSurface7_GetBltStatus(p,a)            (p)->GetBltStatus(a)
#define IDirectDrawSurface7_GetCaps(p,b)                 (p)->GetCaps(b)
#define IDirectDrawSurface7_GetClipper(p,a)              (p)->GetClipper(a)
#define IDirectDrawSurface7_GetColorKey(p,a,b)           (p)->GetColorKey(a,b)
#define IDirectDrawSurface7_GetDC(p,a)                   (p)->GetDC(a)
#define IDirectDrawSurface7_GetFlipStatus(p,a)           (p)->GetFlipStatus(a)
#define IDirectDrawSurface7_GetOverlayPosition(p,a,b)    (p)->GetOverlayPosition(a,b)
#define IDirectDrawSurface7_GetPalette(p,a)              (p)->GetPalette(a)
#define IDirectDrawSurface7_GetPixelFormat(p,a)          (p)->GetPixelFormat(a)
#define IDirectDrawSurface7_GetSurfaceDesc(p,a)          (p)->GetSurfaceDesc(a)
#define IDirectDrawSurface7_Initialize(p,a,b)            (p)->Initialize(a,b)
#define IDirectDrawSurface7_IsLost(p)                    (p)->IsLost()
#define IDirectDrawSurface7_Lock(p,a,b,c,d)              (p)->Lock(a,b,c,d)
#define IDirectDrawSurface7_ReleaseDC(p,a)               (p)->ReleaseDC(a)
#define IDirectDrawSurface7_Restore(p)                   (p)->Restore()
#define IDirectDrawSurface7_SetClipper(p,a)              (p)->SetClipper(a)
#define IDirectDrawSurface7_SetColorKey(p,a,b)           (p)->SetColorKey(a,b)
#define IDirectDrawSurface7_SetOverlayPosition(p,a,b)    (p)->SetOverlayPosition(a,b)
#define IDirectDrawSurface7_SetPalette(p,a)              (p)->SetPalette(a)
#define IDirectDrawSurface7_Unlock(p,b)                  (p)->Unlock(b)
#define IDirectDrawSurface7_UpdateOverlay(p,a,b,c,d,e)   (p)->UpdateOverlay(a,b,c,d,e)
#define IDirectDrawSurface7_UpdateOverlayDisplay(p,a)    (p)->UpdateOverlayDisplay(a)
#define IDirectDrawSurface7_UpdateOverlayZOrder(p,a,b)   (p)->UpdateOverlayZOrder(a,b)
#define IDirectDrawSurface7_GetDDInterface(p,a)          (p)->GetDDInterface(a)
#define IDirectDrawSurface7_PageLock(p,a)                (p)->PageLock(a)
#define IDirectDrawSurface7_PageUnlock(p,a)              (p)->PageUnlock(a)
#define IDirectDrawSurface7_SetSurfaceDesc(p,a,b)        (p)->SetSurfaceDesc(a,b)
#define IDirectDrawSurface7_SetPrivateData(p,a,b,c,d)    (p)->SetPrivateData(a,b,c,d)
#define IDirectDrawSurface7_GetPrivateData(p,a,b,c)      (p)->GetPrivateData(a,b,c)
#define IDirectDrawSurface7_FreePrivateData(p,a)         (p)->FreePrivateData(a)
#define IDirectDrawSurface7_GetUniquenessValue(p, a)     (p)->GetUniquenessValue(a)
#define IDirectDrawSurface7_ChangeUniquenessValue(p)     (p)->ChangeUniquenessValue()
#define IDirectDrawSurface7_SetPriority(p,a)             (p)->SetPriority(a)
#define IDirectDrawSurface7_GetPriority(p,a)             (p)->GetPriority(a)
#define IDirectDrawSurface7_SetLOD(p,a)                  (p)->SetLOD(a)
#define IDirectDrawSurface7_GetLOD(p,a)                  (p)->GetLOD(a)
#endif


/*
 * IDirectDrawColorControl
 */
#if defined( _WIN32 ) && !defined( _NO_COM )
#undef INTERFACE
#define INTERFACE IDirectDrawColorControl
DECLARE_INTERFACE_( IDirectDrawColorControl, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDrawColorControl methods ***/
    STDMETHOD(GetColorControls)(THIS_ LPDDCOLORCONTROL) PURE;
    STDMETHOD(SetColorControls)(THIS_ LPDDCOLORCONTROL) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectDrawColorControl_QueryInterface(p, a, b)  (p)->lpVtbl->QueryInterface(p, a, b)
#define IDirectDrawColorControl_AddRef(p)                (p)->lpVtbl->AddRef(p)
#define IDirectDrawColorControl_Release(p)               (p)->lpVtbl->Release(p)
#define IDirectDrawColorControl_GetColorControls(p, a)   (p)->lpVtbl->GetColorControls(p, a)
#define IDirectDrawColorControl_SetColorControls(p, a)   (p)->lpVtbl->SetColorControls(p, a)
#else
#define IDirectDrawColorControl_QueryInterface(p, a, b)  (p)->QueryInterface(a, b)
#define IDirectDrawColorControl_AddRef(p)                (p)->AddRef()
#define IDirectDrawColorControl_Release(p)               (p)->Release()
#define IDirectDrawColorControl_GetColorControls(p, a)   (p)->GetColorControls(a)
#define IDirectDrawColorControl_SetColorControls(p, a)   (p)->SetColorControls(a)
#endif

#endif


/*
 * IDirectDrawGammaControl
 */
#if defined( _WIN32 ) && !defined( _NO_COM )
#undef INTERFACE
#define INTERFACE IDirectDrawGammaControl
DECLARE_INTERFACE_( IDirectDrawGammaControl, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDrawGammaControl methods ***/
    STDMETHOD(GetGammaRamp)(THIS_ DWORD, LPDDGAMMARAMP) PURE;
    STDMETHOD(SetGammaRamp)(THIS_ DWORD, LPDDGAMMARAMP) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectDrawGammaControl_QueryInterface(p, a, b)  (p)->lpVtbl->QueryInterface(p, a, b)
#define IDirectDrawGammaControl_AddRef(p)                (p)->lpVtbl->AddRef(p)
#define IDirectDrawGammaControl_Release(p)               (p)->lpVtbl->Release(p)
#define IDirectDrawGammaControl_GetGammaRamp(p, a, b)    (p)->lpVtbl->GetGammaRamp(p, a, b)
#define IDirectDrawGammaControl_SetGammaRamp(p, a, b)    (p)->lpVtbl->SetGammaRamp(p, a, b)
#else
#define IDirectDrawGammaControl_QueryInterface(p, a, b)  (p)->QueryInterface(a, b)
#define IDirectDrawGammaControl_AddRef(p)                (p)->AddRef()
#define IDirectDrawGammaControl_Release(p)               (p)->Release()
#define IDirectDrawGammaControl_GetGammaRamp(p, a, b)    (p)->GetGammaRamp(a, b)
#define IDirectDrawGammaControl_SetGammaRamp(p, a, b)    (p)->SetGammaRamp(a, b)
#endif

#endif



#endif


/*
 * DDSURFACEDESC
 */
typedef struct _DDSURFACEDESC
{
    DWORD               dwSize;                 // size of the DDSURFACEDESC structure
    DWORD               dwFlags;                // determines what fields are valid
    DWORD               dwHeight;               // height of surface to be created
    DWORD               dwWidth;                // width of input surface
    union
    {
        LONG            lPitch;                 // distance to start of next line (return value only)
        DWORD           dwLinearSize;           // Formless late-allocated optimized surface size
    } DUMMYUNIONNAMEN(1);
    DWORD               dwBackBufferCount;      // number of back buffers requested
    union
    {
        DWORD           dwMipMapCount;          // number of mip-map levels requested
        DWORD           dwZBufferBitDepth;      // depth of Z buffer requested
        DWORD           dwRefreshRate;          // refresh rate (used when display mode is described)
    } DUMMYUNIONNAMEN(2);
    DWORD               dwAlphaBitDepth;        // depth of alpha buffer requested
    DWORD               dwReserved;             // reserved
    LPVOID              lpSurface;              // pointer to the associated surface memory
    DDCOLORKEY          ddckCKDestOverlay;      // color key for destination overlay use
    DDCOLORKEY          ddckCKDestBlt;          // color key for destination blt use
    DDCOLORKEY          ddckCKSrcOverlay;       // color key for source overlay use
    DDCOLORKEY          ddckCKSrcBlt;           // color key for source blt use
    DDPIXELFORMAT       ddpfPixelFormat;        // pixel format description of the surface
    DDSCAPS             ddsCaps;                // direct draw surface capabilities
} DDSURFACEDESC;

/*
 * DDSURFACEDESC2
 */
typedef struct _DDSURFACEDESC2
{
    DWORD               dwSize;                 // size of the DDSURFACEDESC structure
    DWORD               dwFlags;                // determines what fields are valid
    DWORD               dwHeight;               // height of surface to be created
    DWORD               dwWidth;                // width of input surface
    union
    {
        LONG            lPitch;                 // distance to start of next line (return value only)
        DWORD           dwLinearSize;           // Formless late-allocated optimized surface size
    } DUMMYUNIONNAMEN(1);
    union
    {
        DWORD           dwBackBufferCount;      // number of back buffers requested
        DWORD           dwDepth;                // the depth if this is a volume texture 
    } DUMMYUNIONNAMEN(5);
    union
    {
        DWORD           dwMipMapCount;          // number of mip-map levels requestde
                                                // dwZBufferBitDepth removed, use ddpfPixelFormat one instead
        DWORD           dwRefreshRate;          // refresh rate (used when display mode is described)
        DWORD           dwSrcVBHandle;          // The source used in VB::Optimize
    } DUMMYUNIONNAMEN(2);
    DWORD               dwAlphaBitDepth;        // depth of alpha buffer requested
    DWORD               dwReserved;             // reserved
    LPVOID              lpSurface;              // pointer to the associated surface memory
    union
    {
        DDCOLORKEY      ddckCKDestOverlay;      // color key for destination overlay use
        DWORD           dwEmptyFaceColor;       // Physical color for empty cubemap faces
    } DUMMYUNIONNAMEN(3);
    DDCOLORKEY          ddckCKDestBlt;          // color key for destination blt use
    DDCOLORKEY          ddckCKSrcOverlay;       // color key for source overlay use
    DDCOLORKEY          ddckCKSrcBlt;           // color key for source blt use
    union
    {
        DDPIXELFORMAT   ddpfPixelFormat;        // pixel format description of the surface
        DWORD           dwFVF;                  // vertex format description of vertex buffers
    } DUMMYUNIONNAMEN(4);
    DDSCAPS2            ddsCaps;                // direct draw surface capabilities
    DWORD               dwTextureStage;         // stage in multitexture cascade
} DDSURFACEDESC2;

/*
 * ddsCaps field is valid.
 */
#define DDSD_CAPS               0x00000001l     // default

/*
 * dwHeight field is valid.
 */
#define DDSD_HEIGHT             0x00000002l

/*
 * dwWidth field is valid.
 */
#define DDSD_WIDTH              0x00000004l

/*
 * lPitch is valid.
 */
#define DDSD_PITCH              0x00000008l

/*
 * dwBackBufferCount is valid.
 */
#define DDSD_BACKBUFFERCOUNT    0x00000020l

/*
 * dwZBufferBitDepth is valid.  (shouldnt be used in DDSURFACEDESC2)
 */
#define DDSD_ZBUFFERBITDEPTH    0x00000040l

/*
 * dwAlphaBitDepth is valid.
 */
#define DDSD_ALPHABITDEPTH      0x00000080l


/*
 * lpSurface is valid.
 */
#define DDSD_LPSURFACE          0x00000800l

/*
 * ddpfPixelFormat is valid.
 */
#define DDSD_PIXELFORMAT        0x00001000l

/*
 * ddckCKDestOverlay is valid.
 */
#define DDSD_CKDESTOVERLAY      0x00002000l

/*
 * ddckCKDestBlt is valid.
 */
#define DDSD_CKDESTBLT          0x00004000l

/*
 * ddckCKSrcOverlay is valid.
 */
#define DDSD_CKSRCOVERLAY       0x00008000l

/*
 * ddckCKSrcBlt is valid.
 */
#define DDSD_CKSRCBLT           0x00010000l

/*
 * dwMipMapCount is valid.
 */
#define DDSD_MIPMAPCOUNT        0x00020000l

 /*
  * dwRefreshRate is valid
  */
#define DDSD_REFRESHRATE        0x00040000l

/*
 * dwLinearSize is valid
 */
#define DDSD_LINEARSIZE         0x00080000l

/*
 * dwTextureStage is valid
 */
#define DDSD_TEXTURESTAGE       0x00100000l
/*
 * dwFVF is valid
 */
#define DDSD_FVF                0x00200000l
/*
 * dwSrcVBHandle is valid
 */
#define DDSD_SRCVBHANDLE        0x00400000l

/*
 * dwDepth is valid
 */
#define DDSD_DEPTH              0x00800000l

/*
 * All input fields are valid.
 */
#define DDSD_ALL                0x00fff9eel

/*
 * DDOPTSURFACEDESC
 */
typedef struct _DDOPTSURFACEDESC
{
    DWORD       dwSize;             // size of the DDOPTSURFACEDESC structure
    DWORD       dwFlags;            // determines what fields are valid
    DDSCAPS2    ddSCaps;            // Common caps like: Memory type
    DDOSCAPS    ddOSCaps;           // Common caps like: Memory type
    GUID        guid;               // Compression technique GUID
    DWORD       dwCompressionRatio; // Compression ratio
} DDOPTSURFACEDESC;

/*
 * guid field is valid.
 */
#define DDOSD_GUID                  0x00000001l

/*
 * dwCompressionRatio field is valid.
 */
#define DDOSD_COMPRESSION_RATIO     0x00000002l

/*
 * ddSCaps field is valid.
 */
#define DDOSD_SCAPS                 0x00000004l

/*
 * ddOSCaps field is valid.
 */
#define DDOSD_OSCAPS                0x00000008l

/*
 * All input fields are valid.
 */
#define DDOSD_ALL                   0x0000000fl

/*
 * The surface's optimized pixelformat is compressed
 */
#define DDOSDCAPS_OPTCOMPRESSED                 0x00000001l

/*
 * The surface's optimized pixelformat is reordered
 */
#define DDOSDCAPS_OPTREORDERED                  0x00000002l

/*
 * The opt surface is a monolithic mipmap
 */
#define DDOSDCAPS_MONOLITHICMIPMAP              0x00000004l

/*
 * The valid Surf caps:
 * #define DDSCAPS_SYSTEMMEMORY                 0x00000800l
 * #define DDSCAPS_VIDEOMEMORY          0x00004000l
 * #define DDSCAPS_LOCALVIDMEM          0x10000000l
 * #define DDSCAPS_NONLOCALVIDMEM       0x20000000l
 */
#define DDOSDCAPS_VALIDSCAPS            0x30004800l

/*
 * The valid OptSurf caps
 */
#define DDOSDCAPS_VALIDOSCAPS           0x00000007l


/*
 * DDCOLORCONTROL
 */
typedef struct _DDCOLORCONTROL
{
    DWORD               dwSize;
    DWORD               dwFlags;
    LONG                lBrightness;
    LONG                lContrast;
    LONG                lHue;
    LONG                lSaturation;
    LONG                lSharpness;
    LONG                lGamma;
    LONG                lColorEnable;
    DWORD               dwReserved1;
} DDCOLORCONTROL;


/*
 * lBrightness field is valid.
 */
#define DDCOLOR_BRIGHTNESS              0x00000001l

/*
 * lContrast field is valid.
 */
#define DDCOLOR_CONTRAST                0x00000002l

/*
 * lHue field is valid.
 */
#define DDCOLOR_HUE                     0x00000004l

/*
 * lSaturation field is valid.
 */
#define DDCOLOR_SATURATION              0x00000008l

/*
 * lSharpness field is valid.
 */
#define DDCOLOR_SHARPNESS               0x00000010l

/*
 * lGamma field is valid.
 */
#define DDCOLOR_GAMMA                   0x00000020l

/*
 * lColorEnable field is valid.
 */
#define DDCOLOR_COLORENABLE             0x00000040l



/*============================================================================
 *
 * Direct Draw Capability Flags
 *
 * These flags are used to describe the capabilities of a given Surface.
 * All flags are bit flags.
 *
 *==========================================================================*/

/****************************************************************************
 *
 * DIRECTDRAWSURFACE CAPABILITY FLAGS
 *
 ****************************************************************************/

/*
 * This bit is reserved. It should not be specified.
 */
#define DDSCAPS_RESERVED1                       0x00000001l

/*
 * Indicates that this surface contains alpha-only information.
 * (To determine if a surface is RGBA/YUVA, the pixel format must be
 * interrogated.)
 */
#define DDSCAPS_ALPHA                           0x00000002l

/*
 * Indicates that this surface is a backbuffer.  It is generally
 * set by CreateSurface when the DDSCAPS_FLIP capability bit is set.
 * It indicates that this surface is THE back buffer of a surface
 * flipping structure.  DirectDraw supports N surfaces in a
 * surface flipping structure.  Only the surface that immediately
 * precedeces the DDSCAPS_FRONTBUFFER has this capability bit set.
 * The other surfaces are identified as back buffers by the presence
 * of the DDSCAPS_FLIP capability, their attachment order, and the
 * absence of the DDSCAPS_FRONTBUFFER and DDSCAPS_BACKBUFFER
 * capabilities.  The bit is sent to CreateSurface when a standalone
 * back buffer is being created.  This surface could be attached to
 * a front buffer and/or back buffers to form a flipping surface
 * structure after the CreateSurface call.  See AddAttachments for
 * a detailed description of the behaviors in this case.
 */
#define DDSCAPS_BACKBUFFER                      0x00000004l

/*
 * Indicates a complex surface structure is being described.  A
 * complex surface structure results in the creation of more than
 * one surface.  The additional surfaces are attached to the root
 * surface.  The complex structure can only be destroyed by
 * destroying the root.
 */
#define DDSCAPS_COMPLEX                         0x00000008l

/*
 * Indicates that this surface is a part of a surface flipping structure.
 * When it is passed to CreateSurface the DDSCAPS_FRONTBUFFER and
 * DDSCAP_BACKBUFFER bits are not set.  They are set by CreateSurface
 * on the resulting creations.  The dwBackBufferCount field in the
 * DDSURFACEDESC structure must be set to at least 1 in order for
 * the CreateSurface call to succeed.  The DDSCAPS_COMPLEX capability
 * must always be set with creating multiple surfaces through CreateSurface.
 */
#define DDSCAPS_FLIP                            0x00000010l

/*
 * Indicates that this surface is THE front buffer of a surface flipping
 * structure.  It is generally set by CreateSurface when the DDSCAPS_FLIP
 * capability bit is set.
 * If this capability is sent to CreateSurface then a standalonw front buffer
 * is created.  This surface will not have the DDSCAPS_FLIP capability.
 * It can be attached to other back buffers to form a flipping structure.
 * See AddAttachments for a detailed description of the behaviors in this
 * case.
 */
#define DDSCAPS_FRONTBUFFER                     0x00000020l

/*
 * Indicates that this surface is any offscreen surface that is not an overlay,
 * texture, zbuffer, front buffer, back buffer, or alpha surface.  It is used
 * to identify plain vanilla surfaces.
 */
#define DDSCAPS_OFFSCREENPLAIN                  0x00000040l

/*
 * Indicates that this surface is an overlay.  It may or may not be directly visible
 * depending on whether or not it is currently being overlayed onto the primary
 * surface.  DDSCAPS_VISIBLE can be used to determine whether or not it is being
 * overlayed at the moment.
 */
#define DDSCAPS_OVERLAY                         0x00000080l

/*
 * Indicates that unique DirectDrawPalette objects can be created and
 * attached to this surface.
 */
#define DDSCAPS_PALETTE                         0x00000100l

/*
 * Indicates that this surface is the primary surface.  The primary
 * surface represents what the user is seeing at the moment.
 */
#define DDSCAPS_PRIMARYSURFACE                  0x00000200l


/*
 * This flag used to be DDSCAPS_PRIMARYSURFACELEFT, which is now
 * obsolete.
 */
#define DDSCAPS_RESERVED3               0x00000400l
#define DDSCAPS_PRIMARYSURFACELEFT              0x00000000l

/*
 * Indicates that this surface memory was allocated in system memory
 */
#define DDSCAPS_SYSTEMMEMORY                    0x00000800l

/*
 * Indicates that this surface can be used as a 3D texture.  It does not
 * indicate whether or not the surface is being used for that purpose.
 */
#define DDSCAPS_TEXTURE                         0x00001000l

/*
 * Indicates that a surface may be a destination for 3D rendering.  This
 * bit must be set in order to query for a Direct3D Device Interface
 * from this surface.
 */
#define DDSCAPS_3DDEVICE                        0x00002000l

/*
 * Indicates that this surface exists in video memory.
 */
#define DDSCAPS_VIDEOMEMORY                     0x00004000l

/*
 * Indicates that changes made to this surface are immediately visible.
 * It is always set for the primary surface and is set for overlays while
 * they are being overlayed and texture maps while they are being textured.
 */
#define DDSCAPS_VISIBLE                         0x00008000l

/*
 * Indicates that only writes are permitted to the surface.  Read accesses
 * from the surface may or may not generate a protection fault, but the
 * results of a read from this surface will not be meaningful.  READ ONLY.
 */
#define DDSCAPS_WRITEONLY                       0x00010000l

/*
 * Indicates that this surface is a z buffer. A z buffer does not contain
 * displayable information.  Instead it contains bit depth information that is
 * used to determine which pixels are visible and which are obscured.
 */
#define DDSCAPS_ZBUFFER                         0x00020000l

/*
 * Indicates surface will have a DC associated long term
 */
#define DDSCAPS_OWNDC                           0x00040000l

/*
 * Indicates surface should be able to receive live video
 */
#define DDSCAPS_LIVEVIDEO                       0x00080000l

/*
 * Indicates surface should be able to have a stream decompressed
 * to it by the hardware.
 */
#define DDSCAPS_HWCODEC                         0x00100000l

/*
 * Surface is a ModeX surface.
 *
 */
#define DDSCAPS_MODEX                           0x00200000l

/*
 * Indicates surface is one level of a mip-map. This surface will
 * be attached to other DDSCAPS_MIPMAP surfaces to form the mip-map.
 * This can be done explicitly, by creating a number of surfaces and
 * attaching them with AddAttachedSurface or by implicitly by CreateSurface.
 * If this bit is set then DDSCAPS_TEXTURE must also be set.
 */
#define DDSCAPS_MIPMAP                          0x00400000l

/*
 * This bit is reserved. It should not be specified.
 */
#define DDSCAPS_RESERVED2                       0x00800000l


/*
 * Indicates that memory for the surface is not allocated until the surface
 * is loaded (via the Direct3D texture Load() function).
 */
#define DDSCAPS_ALLOCONLOAD                     0x04000000l

/*
 * Indicates that the surface will recieve data from a video port.
 */
#define DDSCAPS_VIDEOPORT                       0x08000000l

/*
 * Indicates that a video memory surface is resident in true, local video
 * memory rather than non-local video memory. If this flag is specified then
 * so must DDSCAPS_VIDEOMEMORY. This flag is mutually exclusive with
 * DDSCAPS_NONLOCALVIDMEM.
 */
#define DDSCAPS_LOCALVIDMEM                     0x10000000l

/*
 * Indicates that a video memory surface is resident in non-local video
 * memory rather than true, local video memory. If this flag is specified
 * then so must DDSCAPS_VIDEOMEMORY. This flag is mutually exclusive with
 * DDSCAPS_LOCALVIDMEM.
 */
#define DDSCAPS_NONLOCALVIDMEM                  0x20000000l

/*
 * Indicates that this surface is a standard VGA mode surface, and not a
 * ModeX surface. (This flag will never be set in combination with the
 * DDSCAPS_MODEX flag).
 */
#define DDSCAPS_STANDARDVGAMODE                 0x40000000l

/*
 * Indicates that this surface will be an optimized surface. This flag is
 * currently only valid in conjunction with the DDSCAPS_TEXTURE flag. The surface
 * will be created without any underlying video memory until loaded.
 */
#define DDSCAPS_OPTIMIZED                       0x80000000l



/*
 * This bit is reserved
 */
#define DDSCAPS2_RESERVED4                      0x00000002L
#define DDSCAPS2_HARDWAREDEINTERLACE            0x00000000L

/*
 * Indicates to the driver that this surface will be locked very frequently
 * (for procedural textures, dynamic lightmaps, etc). Surfaces with this cap
 * set must also have DDSCAPS_TEXTURE. This cap cannot be used with
 * DDSCAPS2_HINTSTATIC and DDSCAPS2_OPAQUE.
 */
#define DDSCAPS2_HINTDYNAMIC                    0x00000004L

/*
 * Indicates to the driver that this surface can be re-ordered/retiled on
 * load. This operation will not change the size of the texture. It is
 * relatively fast and symmetrical, since the application may lock these
 * bits (although it will take a performance hit when doing so). Surfaces
 * with this cap set must also have DDSCAPS_TEXTURE. This cap cannot be
 * used with DDSCAPS2_HINTDYNAMIC and DDSCAPS2_OPAQUE.
 */
#define DDSCAPS2_HINTSTATIC                     0x00000008L

/*
 * Indicates that the client would like this texture surface to be managed by the
 * DirectDraw/Direct3D runtime. Surfaces with this cap set must also have
 * DDSCAPS_TEXTURE set.
 */
#define DDSCAPS2_TEXTUREMANAGE                  0x00000010L

/*
 * These bits are reserved for internal use */
#define DDSCAPS2_RESERVED1                      0x00000020L
#define DDSCAPS2_RESERVED2                      0x00000040L

/*
 * Indicates to the driver that this surface will never be locked again.
 * The driver is free to optimize this surface via retiling and actual compression.
 * All calls to Lock() or Blts from this surface will fail. Surfaces with this
 * cap set must also have DDSCAPS_TEXTURE. This cap cannot be used with
 * DDSCAPS2_HINTDYNAMIC and DDSCAPS2_HINTSTATIC.
 */
#define DDSCAPS2_OPAQUE                         0x00000080L

/*
 * Applications should set this bit at CreateSurface time to indicate that they
 * intend to use antialiasing. Only valid if DDSCAPS_3DDEVICE is also set.
 */
#define DDSCAPS2_HINTANTIALIASING               0x00000100L


/*
 * This flag is used at CreateSurface time to indicate that this set of
 * surfaces is a cubic environment map
 */
#define DDSCAPS2_CUBEMAP                        0x00000200L

/*
 * These flags preform two functions:
 * - At CreateSurface time, they define which of the six cube faces are
 *   required by the application.
 * - After creation, each face in the cubemap will have exactly one of these
 *   bits set.
 */
#define DDSCAPS2_CUBEMAP_POSITIVEX              0x00000400L
#define DDSCAPS2_CUBEMAP_NEGATIVEX              0x00000800L
#define DDSCAPS2_CUBEMAP_POSITIVEY              0x00001000L
#define DDSCAPS2_CUBEMAP_NEGATIVEY              0x00002000L
#define DDSCAPS2_CUBEMAP_POSITIVEZ              0x00004000L
#define DDSCAPS2_CUBEMAP_NEGATIVEZ              0x00008000L

/*
 * This macro may be used to specify all faces of a cube map at CreateSurface time
 */
#define DDSCAPS2_CUBEMAP_ALLFACES ( DDSCAPS2_CUBEMAP_POSITIVEX |\
                                    DDSCAPS2_CUBEMAP_NEGATIVEX |\
                                    DDSCAPS2_CUBEMAP_POSITIVEY |\
                                    DDSCAPS2_CUBEMAP_NEGATIVEY |\
                                    DDSCAPS2_CUBEMAP_POSITIVEZ |\
                                    DDSCAPS2_CUBEMAP_NEGATIVEZ )


/*
 * This flag is an additional flag which is present on mipmap sublevels from DX7 onwards
 * It enables easier use of GetAttachedSurface rather than EnumAttachedSurfaces for surface
 * constructs such as Cube Maps, wherein there are more than one mipmap surface attached
 * to the root surface.
 * This caps bit is ignored by CreateSurface
 */
#define DDSCAPS2_MIPMAPSUBLEVEL                 0x00010000L

/* This flag indicates that the texture should be managed by D3D only */
#define DDSCAPS2_D3DTEXTUREMANAGE               0x00020000L

/* This flag indicates that the managed surface can be safely lost */
#define DDSCAPS2_DONOTPERSIST                   0x00040000L

/* indicates that this surface is part of a stereo flipping chain */
#define DDSCAPS2_STEREOSURFACELEFT              0x00080000L


/*
 * Indicates that the surface is a volume.
 * Can be combined with DDSCAPS_MIPMAP to indicate a multi-level volume
 */
#define DDSCAPS2_VOLUME                         0x00200000L

/*
 * Indicates that the surface may be locked multiple times by the application.
 * This cap cannot be used with DDSCAPS2_OPAQUE.
 */
#define DDSCAPS2_NOTUSERLOCKABLE                0x00400000L

/*
 * Indicates that the vertex buffer data can be used to render points and
 * point sprites.
 */
#define DDSCAPS2_POINTS                         0x00800000L

/*
 * Indicates that the vertex buffer data can be used to render rt pactches.
 */
#define DDSCAPS2_RTPATCHES                      0x01000000L

/*
 * Indicates that the vertex buffer data can be used to render n patches.
 */
#define DDSCAPS2_NPATCHES                       0x02000000L

/*
 * This bit is reserved for internal use 
 */
#define DDSCAPS2_RESERVED3                      0x04000000L


/*
 * Indicates that the contents of the backbuffer do not have to be preserved
 * the contents of the backbuffer after they are presented.
 */
#define DDSCAPS2_DISCARDBACKBUFFER              0x10000000L

/*
 * Indicates that all surfaces in this creation chain should be given an alpha channel.
 * This flag will be set on primary surface chains that may have no explicit pixel format
 * (and thus take on the format of the current display mode).
 * The driver should infer that all these surfaces have a format having an alpha channel.
 * (e.g. assume D3DFMT_A8R8G8B8 if the display mode is x888.)
 */
#define DDSCAPS2_ENABLEALPHACHANNEL             0x20000000L

/*
 * Indicates that all surfaces in this creation chain is extended primary surface format.
 * This flag will be set on extended primary surface chains that always have explicit pixel
 * format and the pixel format is typically GDI (Graphics Device Interface) couldn't handle,
 * thus only used with fullscreen application. (e.g. D3DFMT_A2R10G10B10 format)
 */
#define DDSCAPS2_EXTENDEDFORMATPRIMARY          0x40000000L

/*
 * Indicates that all surfaces in this creation chain is additional primary surface.
 * This flag will be set on primary surface chains which must present on the adapter
 * id provided on dwCaps4. Typically this will be used to create secondary primary surface
 * on DualView display adapter.
 */
#define DDSCAPS2_ADDITIONALPRIMARY              0x80000000L

/*
 * This is a mask that indicates the set of bits that may be set
 * at createsurface time to indicate number of samples per pixel
 * when multisampling
 */
#define DDSCAPS3_MULTISAMPLE_MASK               0x0000001FL

/*
 * This is a mask that indicates the set of bits that may be set
 * at createsurface time to indicate the quality level of rendering
 * for the current number of samples per pixel
 */
#define DDSCAPS3_MULTISAMPLE_QUALITY_MASK       0x000000E0L
#define DDSCAPS3_MULTISAMPLE_QUALITY_SHIFT      5

/*
 * This bit is reserved for internal use 
 */
#define DDSCAPS3_RESERVED1                      0x00000100L

/*
 * This bit is reserved for internal use 
 */
#define DDSCAPS3_RESERVED2                      0x00000200L

/*
 * This indicates whether this surface has light-weight miplevels
 */
#define DDSCAPS3_LIGHTWEIGHTMIPMAP              0x00000400L

/*
 * This indicates that the mipsublevels for this surface are auto-generated
 */
#define DDSCAPS3_AUTOGENMIPMAP                  0x00000800L

/*
 * This indicates that the mipsublevels for this surface are auto-generated
 */
#define DDSCAPS3_DMAP                           0x00001000L


 /****************************************************************************
 *
 * DIRECTDRAW DRIVER CAPABILITY FLAGS
 *
 ****************************************************************************/

/*
 * Display hardware has 3D acceleration.
 */
#define DDCAPS_3D                       0x00000001l

/*
 * Indicates that DirectDraw will support only dest rectangles that are aligned
 * on DIRECTDRAWCAPS.dwAlignBoundaryDest boundaries of the surface, respectively.
 * READ ONLY.
 */
#define DDCAPS_ALIGNBOUNDARYDEST        0x00000002l

/*
 * Indicates that DirectDraw will support only source rectangles  whose sizes in
 * BYTEs are DIRECTDRAWCAPS.dwAlignSizeDest multiples, respectively.  READ ONLY.
 */
#define DDCAPS_ALIGNSIZEDEST            0x00000004l
/*
 * Indicates that DirectDraw will support only source rectangles that are aligned
 * on DIRECTDRAWCAPS.dwAlignBoundarySrc boundaries of the surface, respectively.
 * READ ONLY.
 */
#define DDCAPS_ALIGNBOUNDARYSRC         0x00000008l

/*
 * Indicates that DirectDraw will support only source rectangles  whose sizes in
 * BYTEs are DIRECTDRAWCAPS.dwAlignSizeSrc multiples, respectively.  READ ONLY.
 */
#define DDCAPS_ALIGNSIZESRC             0x00000010l

/*
 * Indicates that DirectDraw will create video memory surfaces that have a stride
 * alignment equal to DIRECTDRAWCAPS.dwAlignStride.  READ ONLY.
 */
#define DDCAPS_ALIGNSTRIDE              0x00000020l

/*
 * Display hardware is capable of blt operations.
 */
#define DDCAPS_BLT                      0x00000040l

/*
 * Display hardware is capable of asynchronous blt operations.
 */
#define DDCAPS_BLTQUEUE                 0x00000080l

/*
 * Display hardware is capable of color space conversions during the blt operation.
 */
#define DDCAPS_BLTFOURCC                0x00000100l

/*
 * Display hardware is capable of stretching during blt operations.
 */
#define DDCAPS_BLTSTRETCH               0x00000200l

/*
 * Display hardware is shared with GDI.
 */
#define DDCAPS_GDI                      0x00000400l

/*
 * Display hardware can overlay.
 */
#define DDCAPS_OVERLAY                  0x00000800l

/*
 * Set if display hardware supports overlays but can not clip them.
 */
#define DDCAPS_OVERLAYCANTCLIP          0x00001000l

/*
 * Indicates that overlay hardware is capable of color space conversions during
 * the overlay operation.
 */
#define DDCAPS_OVERLAYFOURCC            0x00002000l

/*
 * Indicates that stretching can be done by the overlay hardware.
 */
#define DDCAPS_OVERLAYSTRETCH           0x00004000l

/*
 * Indicates that unique DirectDrawPalettes can be created for DirectDrawSurfaces
 * other than the primary surface.
 */
#define DDCAPS_PALETTE                  0x00008000l

/*
 * Indicates that palette changes can be syncd with the veritcal refresh.
 */
#define DDCAPS_PALETTEVSYNC             0x00010000l

/*
 * Display hardware can return the current scan line.
 */
#define DDCAPS_READSCANLINE             0x00020000l


/*
 * This flag used to bo DDCAPS_STEREOVIEW, which is now obsolete
 */
#define DDCAPS_RESERVED1                0x00040000l

/*
 * Display hardware is capable of generating a vertical blank interrupt.
 */
#define DDCAPS_VBI                      0x00080000l

/*
 * Supports the use of z buffers with blt operations.
 */
#define DDCAPS_ZBLTS                    0x00100000l

/*
 * Supports Z Ordering of overlays.
 */
#define DDCAPS_ZOVERLAYS                0x00200000l

/*
 * Supports color key
 */
#define DDCAPS_COLORKEY                 0x00400000l

/*
 * Supports alpha surfaces
 */
#define DDCAPS_ALPHA                    0x00800000l

/*
 * colorkey is hardware assisted(DDCAPS_COLORKEY will also be set)
 */
#define DDCAPS_COLORKEYHWASSIST         0x01000000l

/*
 * no hardware support at all
 */
#define DDCAPS_NOHARDWARE               0x02000000l

/*
 * Display hardware is capable of color fill with bltter
 */
#define DDCAPS_BLTCOLORFILL             0x04000000l

/*
 * Display hardware is bank switched, and potentially very slow at
 * random access to VRAM.
 */
#define DDCAPS_BANKSWITCHED             0x08000000l

/*
 * Display hardware is capable of depth filling Z-buffers with bltter
 */
#define DDCAPS_BLTDEPTHFILL             0x10000000l

/*
 * Display hardware is capable of clipping while bltting.
 */
#define DDCAPS_CANCLIP                  0x20000000l

/*
 * Display hardware is capable of clipping while stretch bltting.
 */
#define DDCAPS_CANCLIPSTRETCHED         0x40000000l

/*
 * Display hardware is capable of bltting to or from system memory
 */
#define DDCAPS_CANBLTSYSMEM             0x80000000l


 /****************************************************************************
 *
 * MORE DIRECTDRAW DRIVER CAPABILITY FLAGS (dwCaps2)
 *
 ****************************************************************************/

/*
 * Display hardware is certified
 */
#define DDCAPS2_CERTIFIED              0x00000001l

/*
 * Driver cannot interleave 2D operations (lock and blt) to surfaces with
 * Direct3D rendering operations between calls to BeginScene() and EndScene()
 */
#define DDCAPS2_NO2DDURING3DSCENE       0x00000002l

/*
 * Display hardware contains a video port
 */
#define DDCAPS2_VIDEOPORT               0x00000004l

/*
 * The overlay can be automatically flipped according to the video port
 * VSYNCs, providing automatic doubled buffered display of video port
 * data using an overlay
 */
#define DDCAPS2_AUTOFLIPOVERLAY         0x00000008l

/*
 * Overlay can display each field of interlaced data individually while
 * it is interleaved in memory without causing jittery artifacts.
 */
#define DDCAPS2_CANBOBINTERLEAVED       0x00000010l

/*
 * Overlay can display each field of interlaced data individually while
 * it is not interleaved in memory without causing jittery artifacts.
 */
#define DDCAPS2_CANBOBNONINTERLEAVED    0x00000020l

/*
 * The overlay surface contains color controls (brightness, sharpness, etc.)
 */
#define DDCAPS2_COLORCONTROLOVERLAY     0x00000040l

/*
 * The primary surface contains color controls (gamma, etc.)
 */
#define DDCAPS2_COLORCONTROLPRIMARY     0x00000080l

/*
 * RGBZ -> RGB supported for 16:16 RGB:Z
 */
#define DDCAPS2_CANDROPZ16BIT           0x00000100l

/*
 * Driver supports non-local video memory.
 */
#define DDCAPS2_NONLOCALVIDMEM          0x00000200l

/*
 * Dirver supports non-local video memory but has different capabilities for
 * non-local video memory surfaces. If this bit is set then so must
 * DDCAPS2_NONLOCALVIDMEM.
 */
#define DDCAPS2_NONLOCALVIDMEMCAPS      0x00000400l

/*
 * Driver neither requires nor prefers surfaces to be pagelocked when performing
 * blts involving system memory surfaces
 */
#define DDCAPS2_NOPAGELOCKREQUIRED      0x00000800l

/*
 * Driver can create surfaces which are wider than the primary surface
 */
#define DDCAPS2_WIDESURFACES            0x00001000l

/*
 * Driver supports bob without using a video port by handling the
 * DDFLIP_ODD and DDFLIP_EVEN flags specified in Flip.
 */
#define DDCAPS2_CANFLIPODDEVEN          0x00002000l

/*
 * Driver supports bob using hardware
 */
#define DDCAPS2_CANBOBHARDWARE          0x00004000l

/*
 * Driver supports bltting any FOURCC surface to another surface of the same FOURCC
 */
#define DDCAPS2_COPYFOURCC              0x00008000l


/*
 * Driver supports loadable gamma ramps for the primary surface
 */
#define DDCAPS2_PRIMARYGAMMA            0x00020000l

/*
 * Driver can render in windowed mode.
 */
#define DDCAPS2_CANRENDERWINDOWED       0x00080000l

/*
 * A calibrator is available to adjust the gamma ramp according to the
 * physical display properties so that the result will be identical on
 * all calibrated systems.
 */
#define DDCAPS2_CANCALIBRATEGAMMA       0x00100000l

/*
 * Indicates that the driver will respond to DDFLIP_INTERVALn flags
 */
#define DDCAPS2_FLIPINTERVAL            0x00200000l

/*
 * Indicates that the driver will respond to DDFLIP_NOVSYNC
 */
#define DDCAPS2_FLIPNOVSYNC             0x00400000l

/*
 * Driver supports management of video memory, if this flag is ON,
 * driver manages the texture if requested with DDSCAPS2_TEXTUREMANAGE on
 * DirectX manages the texture if this flag is OFF and surface has DDSCAPS2_TEXTUREMANAGE on
 */
#define DDCAPS2_CANMANAGETEXTURE        0x00800000l

/*
 * The Direct3D texture manager uses this cap to decide whether to put managed
 * surfaces in non-local video memory. If the cap is set, the texture manager will
 * put managed surfaces in non-local vidmem. Drivers that cannot texture from
 * local vidmem SHOULD NOT set this cap.
 */
#define DDCAPS2_TEXMANINNONLOCALVIDMEM  0x01000000l

/*
 * Indicates that the driver supports DX7 type of stereo in at least one mode (which may
 * not necessarily be the current mode). Applications should use IDirectDraw7 (or higher)
 * ::EnumDisplayModes and check the DDSURFACEDESC.ddsCaps.dwCaps2 field for the presence of
 * DDSCAPS2_STEREOSURFACELEFT to check if a particular mode supports stereo. The application
 * can also use IDirectDraw7(or higher)::GetDisplayMode to check the current mode.
 */
#define DDCAPS2_STEREO                  0x02000000L

/*
 * This caps bit is intended for internal DirectDraw use.
 * -It is only valid if DDCAPS2_NONLOCALVIDMEMCAPS is set.
 * -If this bit is set, then DDCAPS_CANBLTSYSMEM MUST be set by the driver (and
 *  all the assoicated system memory blt caps must be correct).
 * -It implies that the system->video blt caps in DDCAPS also apply to system to
 *  nonlocal blts. I.e. the dwSVBCaps, dwSVBCKeyCaps, dwSVBFXCaps and dwSVBRops
 *  members of DDCAPS (DDCORECAPS) are filled in correctly.
 * -Any blt from system to nonlocal memory that matches these caps bits will
 *  be passed to the driver.
 *
 * NOTE: This is intended to enable the driver itself to do efficient reordering
 * of textures. This is NOT meant to imply that hardware can write into AGP memory.
 * This operation is not currently supported.
 */
#define DDCAPS2_SYSTONONLOCAL_AS_SYSTOLOCAL   0x04000000L

/*
 * was DDCAPS2_PUREHAL
 */
#define DDCAPS2_RESERVED1                     0x08000000L

/*
 * Driver supports management of video memory, if this flag is ON,
 * driver manages the resource if requested with DDSCAPS2_TEXTUREMANAGE on
 * DirectX manages the resource if this flag is OFF and surface has DDSCAPS2_TEXTUREMANAGE on
 */
#define DDCAPS2_CANMANAGERESOURCE             0x10000000L

/*
 * Driver supports dynamic textures. This will allow the application to set
 * D3DUSAGE_DYNAMIC (DDSCAPS2_HINTDYNAMIC for drivers) at texture create time.
 * Video memory dynamic textures WILL be lockable by applications. It is
 * expected that these locks will be very efficient (which implies that the
 * driver should always maintain a linear copy, a pointer to which can be
 * quickly handed out to the application).
 */
#define DDCAPS2_DYNAMICTEXTURES               0x20000000L

/*
 * Driver supports auto-generation of mipmaps.
 */
#define DDCAPS2_CANAUTOGENMIPMAP              0x40000000L


/****************************************************************************
 *
 * DIRECTDRAW FX ALPHA CAPABILITY FLAGS
 *
 ****************************************************************************/

/*
 * Supports alpha blending around the edge of a source color keyed surface.
 * For Blt.
 */
#define DDFXALPHACAPS_BLTALPHAEDGEBLEND         0x00000001l

/*
 * Supports alpha information in the pixel format.  The bit depth of alpha
 * information in the pixel format can be 1,2,4, or 8.  The alpha value becomes
 * more opaque as the alpha value increases.  (0 is transparent.)
 * For Blt.
 */
#define DDFXALPHACAPS_BLTALPHAPIXELS            0x00000002l

/*
 * Supports alpha information in the pixel format.  The bit depth of alpha
 * information in the pixel format can be 1,2,4, or 8.  The alpha value
 * becomes more transparent as the alpha value increases.  (0 is opaque.)
 * This flag can only be set if DDCAPS_ALPHA is set.
 * For Blt.
 */
#define DDFXALPHACAPS_BLTALPHAPIXELSNEG         0x00000004l

/*
 * Supports alpha only surfaces.  The bit depth of an alpha only surface can be
 * 1,2,4, or 8.  The alpha value becomes more opaque as the alpha value increases.
 * (0 is transparent.)
 * For Blt.
 */
#define DDFXALPHACAPS_BLTALPHASURFACES          0x00000008l

/*
 * The depth of the alpha channel data can range can be 1,2,4, or 8.
 * The NEG suffix indicates that this alpha channel becomes more transparent
 * as the alpha value increases. (0 is opaque.)  This flag can only be set if
 * DDCAPS_ALPHA is set.
 * For Blt.
 */
#define DDFXALPHACAPS_BLTALPHASURFACESNEG       0x00000010l

/*
 * Supports alpha blending around the edge of a source color keyed surface.
 * For Overlays.
 */
#define DDFXALPHACAPS_OVERLAYALPHAEDGEBLEND     0x00000020l

/*
 * Supports alpha information in the pixel format.  The bit depth of alpha
 * information in the pixel format can be 1,2,4, or 8.  The alpha value becomes
 * more opaque as the alpha value increases.  (0 is transparent.)
 * For Overlays.
 */
#define DDFXALPHACAPS_OVERLAYALPHAPIXELS        0x00000040l

/*
 * Supports alpha information in the pixel format.  The bit depth of alpha
 * information in the pixel format can be 1,2,4, or 8.  The alpha value
 * becomes more transparent as the alpha value increases.  (0 is opaque.)
 * This flag can only be set if DDCAPS_ALPHA is set.
 * For Overlays.
 */
#define DDFXALPHACAPS_OVERLAYALPHAPIXELSNEG     0x00000080l

/*
 * Supports alpha only surfaces.  The bit depth of an alpha only surface can be
 * 1,2,4, or 8.  The alpha value becomes more opaque as the alpha value increases.
 * (0 is transparent.)
 * For Overlays.
 */
#define DDFXALPHACAPS_OVERLAYALPHASURFACES      0x00000100l

/*
 * The depth of the alpha channel data can range can be 1,2,4, or 8.
 * The NEG suffix indicates that this alpha channel becomes more transparent
 * as the alpha value increases. (0 is opaque.)  This flag can only be set if
 * DDCAPS_ALPHA is set.
 * For Overlays.
 */
#define DDFXALPHACAPS_OVERLAYALPHASURFACESNEG   0x00000200l

#if DIRECTDRAW_VERSION < 0x0600
#endif  //DIRECTDRAW_VERSION


/****************************************************************************
 *
 * DIRECTDRAW FX CAPABILITY FLAGS
 *
 ****************************************************************************/

/*
 * Uses arithmetic operations to stretch and shrink surfaces during blt
 * rather than pixel doubling techniques.  Along the Y axis.
 */
#define DDFXCAPS_BLTARITHSTRETCHY       0x00000020l

/*
 * Uses arithmetic operations to stretch during blt
 * rather than pixel doubling techniques.  Along the Y axis. Only
 * works for x1, x2, etc.
 */
#define DDFXCAPS_BLTARITHSTRETCHYN      0x00000010l

/*
 * Supports mirroring left to right in blt.
 */
#define DDFXCAPS_BLTMIRRORLEFTRIGHT     0x00000040l

/*
 * Supports mirroring top to bottom in blt.
 */
#define DDFXCAPS_BLTMIRRORUPDOWN        0x00000080l

/*
 * Supports arbitrary rotation for blts.
 */
#define DDFXCAPS_BLTROTATION            0x00000100l

/*
 * Supports 90 degree rotations for blts.
 */
#define DDFXCAPS_BLTROTATION90          0x00000200l

/*
 * DirectDraw supports arbitrary shrinking of a surface along the
 * x axis (horizontal direction) for blts.
 */
#define DDFXCAPS_BLTSHRINKX             0x00000400l

/*
 * DirectDraw supports integer shrinking (1x,2x,) of a surface
 * along the x axis (horizontal direction) for blts.
 */
#define DDFXCAPS_BLTSHRINKXN            0x00000800l

/*
 * DirectDraw supports arbitrary shrinking of a surface along the
 * y axis (horizontal direction) for blts.
 */
#define DDFXCAPS_BLTSHRINKY             0x00001000l

/*
 * DirectDraw supports integer shrinking (1x,2x,) of a surface
 * along the y axis (vertical direction) for blts.
 */
#define DDFXCAPS_BLTSHRINKYN            0x00002000l

/*
 * DirectDraw supports arbitrary stretching of a surface along the
 * x axis (horizontal direction) for blts.
 */
#define DDFXCAPS_BLTSTRETCHX            0x00004000l

/*
 * DirectDraw supports integer stretching (1x,2x,) of a surface
 * along the x axis (horizontal direction) for blts.
 */
#define DDFXCAPS_BLTSTRETCHXN           0x00008000l

/*
 * DirectDraw supports arbitrary stretching of a surface along the
 * y axis (horizontal direction) for blts.
 */
#define DDFXCAPS_BLTSTRETCHY            0x00010000l

/*
 * DirectDraw supports integer stretching (1x,2x,) of a surface
 * along the y axis (vertical direction) for blts.
 */
#define DDFXCAPS_BLTSTRETCHYN           0x00020000l

/*
 * Uses arithmetic operations to stretch and shrink surfaces during
 * overlay rather than pixel doubling techniques.  Along the Y axis
 * for overlays.
 */
#define DDFXCAPS_OVERLAYARITHSTRETCHY   0x00040000l

/*
 * Uses arithmetic operations to stretch surfaces during
 * overlay rather than pixel doubling techniques.  Along the Y axis
 * for overlays. Only works for x1, x2, etc.
 */
#define DDFXCAPS_OVERLAYARITHSTRETCHYN  0x00000008l

/*
 * DirectDraw supports arbitrary shrinking of a surface along the
 * x axis (horizontal direction) for overlays.
 */
#define DDFXCAPS_OVERLAYSHRINKX         0x00080000l

/*
 * DirectDraw supports integer shrinking (1x,2x,) of a surface
 * along the x axis (horizontal direction) for overlays.
 */
#define DDFXCAPS_OVERLAYSHRINKXN        0x00100000l

/*
 * DirectDraw supports arbitrary shrinking of a surface along the
 * y axis (horizontal direction) for overlays.
 */
#define DDFXCAPS_OVERLAYSHRINKY         0x00200000l

/*
 * DirectDraw supports integer shrinking (1x,2x,) of a surface
 * along the y axis (vertical direction) for overlays.
 */
#define DDFXCAPS_OVERLAYSHRINKYN        0x00400000l

/*
 * DirectDraw supports arbitrary stretching of a surface along the
 * x axis (horizontal direction) for overlays.
 */
#define DDFXCAPS_OVERLAYSTRETCHX        0x00800000l

/*
 * DirectDraw supports integer stretching (1x,2x,) of a surface
 * along the x axis (horizontal direction) for overlays.
 */
#define DDFXCAPS_OVERLAYSTRETCHXN       0x01000000l

/*
 * DirectDraw supports arbitrary stretching of a surface along the
 * y axis (horizontal direction) for overlays.
 */
#define DDFXCAPS_OVERLAYSTRETCHY        0x02000000l

/*
 * DirectDraw supports integer stretching (1x,2x,) of a surface
 * along the y axis (vertical direction) for overlays.
 */
#define DDFXCAPS_OVERLAYSTRETCHYN       0x04000000l

/*
 * DirectDraw supports mirroring of overlays across the vertical axis
 */
#define DDFXCAPS_OVERLAYMIRRORLEFTRIGHT 0x08000000l

/*
 * DirectDraw supports mirroring of overlays across the horizontal axis
 */
#define DDFXCAPS_OVERLAYMIRRORUPDOWN    0x10000000l

/*
 * DirectDraw supports deinterlacing of overlay surfaces
 */
#define DDFXCAPS_OVERLAYDEINTERLACE		0x20000000l

/*
 * Driver can do alpha blending for blits.
 */
#define DDFXCAPS_BLTALPHA               0x00000001l


/*
 * Driver can do surface-reconstruction filtering for warped blits.
 */
#define DDFXCAPS_BLTFILTER              DDFXCAPS_BLTARITHSTRETCHY

/*
 * Driver can do alpha blending for overlays.
 */
#define DDFXCAPS_OVERLAYALPHA           0x00000004l


/*
 * Driver can do surface-reconstruction filtering for warped overlays.
 */
#define DDFXCAPS_OVERLAYFILTER          DDFXCAPS_OVERLAYARITHSTRETCHY

/****************************************************************************
 *
 * DIRECTDRAW STEREO VIEW CAPABILITIES
 *
 ****************************************************************************/

/*
 * This flag used to be DDSVCAPS_ENIGMA, which is now obsolete
 */

#define DDSVCAPS_RESERVED1              0x00000001l

/*
 * This flag used to be DDSVCAPS_FLICKER, which is now obsolete
 */
#define DDSVCAPS_RESERVED2              0x00000002l

/*
 * This flag used to be DDSVCAPS_REDBLUE, which is now obsolete
 */
#define DDSVCAPS_RESERVED3              0x00000004l

/*
 * This flag used to be DDSVCAPS_SPLIT, which is now obsolete
 */
#define DDSVCAPS_RESERVED4              0x00000008l

/*
 * The stereo view is accomplished with switching technology
 */

#define DDSVCAPS_STEREOSEQUENTIAL       0x00000010L



/****************************************************************************
 *
 * DIRECTDRAWPALETTE CAPABILITIES
 *
 ****************************************************************************/

/*
 * Index is 4 bits.  There are sixteen color entries in the palette table.
 */
#define DDPCAPS_4BIT                    0x00000001l

/*
 * Index is onto a 8 bit color index.  This field is only valid with the
 * DDPCAPS_1BIT, DDPCAPS_2BIT or DDPCAPS_4BIT capability and the target
 * surface is in 8bpp. Each color entry is one byte long and is an index
 * into destination surface's 8bpp palette.
 */
#define DDPCAPS_8BITENTRIES             0x00000002l

/*
 * Index is 8 bits.  There are 256 color entries in the palette table.
 */
#define DDPCAPS_8BIT                    0x00000004l

/*
 * Indicates that this DIRECTDRAWPALETTE should use the palette color array
 * passed into the lpDDColorArray parameter to initialize the DIRECTDRAWPALETTE
 * object.
 * This flag is obsolete. DirectDraw always initializes the color array from
 * the lpDDColorArray parameter. The definition remains for source-level
 * compatibility.
 */
#define DDPCAPS_INITIALIZE              0x00000000l

/*
 * This palette is the one attached to the primary surface.  Changing this
 * table has immediate effect on the display unless DDPSETPAL_VSYNC is specified
 * and supported.
 */
#define DDPCAPS_PRIMARYSURFACE          0x00000010l

/*
 * This palette is the one attached to the primary surface left.  Changing
 * this table has immediate effect on the display for the left eye unless
 * DDPSETPAL_VSYNC is specified and supported.
 */
#define DDPCAPS_PRIMARYSURFACELEFT      0x00000020l

/*
 * This palette can have all 256 entries defined
 */
#define DDPCAPS_ALLOW256                0x00000040l

/*
 * This palette can have modifications to it synced with the monitors
 * refresh rate.
 */
#define DDPCAPS_VSYNC                   0x00000080l

/*
 * Index is 1 bit.  There are two color entries in the palette table.
 */
#define DDPCAPS_1BIT                    0x00000100l

/*
 * Index is 2 bit.  There are four color entries in the palette table.
 */
#define DDPCAPS_2BIT                    0x00000200l

/*
 * The peFlags member of PALETTEENTRY denotes an 8 bit alpha value
 */
#define DDPCAPS_ALPHA                   0x00000400l


/****************************************************************************
 *
 * DIRECTDRAWPALETTE SETENTRY CONSTANTS
 *
 ****************************************************************************/


/****************************************************************************
 *
 * DIRECTDRAWPALETTE GETENTRY CONSTANTS
 *
 ****************************************************************************/

/* 0 is the only legal value */

/****************************************************************************
 *
 * DIRECTDRAWSURFACE SETPRIVATEDATA CONSTANTS
 *
 ****************************************************************************/

/*
 * The passed pointer is an IUnknown ptr. The cbData argument to SetPrivateData
 * must be set to sizeof(IUnknown*). DirectDraw will call AddRef through this
 * pointer and Release when the private data is destroyed. This includes when
 * the surface or palette is destroyed before such priovate data is destroyed.
 */
#define DDSPD_IUNKNOWNPOINTER           0x00000001L

/*
 * Private data is only valid for the current state of the object,
 * as determined by the uniqueness value.
 */
#define DDSPD_VOLATILE                  0x00000002L


/****************************************************************************
 *
 * DIRECTDRAWSURFACE SETPALETTE CONSTANTS
 *
 ****************************************************************************/


/****************************************************************************
 *
 * DIRECTDRAW BITDEPTH CONSTANTS
 *
 * NOTE:  These are only used to indicate supported bit depths.   These
 * are flags only, they are not to be used as an actual bit depth.   The
 * absolute numbers 1, 2, 4, 8, 16, 24 and 32 are used to indicate actual
 * bit depths in a surface or for changing the display mode.
 *
 ****************************************************************************/

/*
 * 1 bit per pixel.
 */
#define DDBD_1                  0x00004000l

/*
 * 2 bits per pixel.
 */
#define DDBD_2                  0x00002000l

/*
 * 4 bits per pixel.
 */
#define DDBD_4                  0x00001000l

/*
 * 8 bits per pixel.
 */
#define DDBD_8                  0x00000800l

/*
 * 16 bits per pixel.
 */
#define DDBD_16                 0x00000400l

/*
 * 24 bits per pixel.
 */
#define DDBD_24                 0X00000200l

/*
 * 32 bits per pixel.
 */
#define DDBD_32                 0x00000100l

/****************************************************************************
 *
 * DIRECTDRAWSURFACE SET/GET COLOR KEY FLAGS
 *
 ****************************************************************************/

/*
 * Set if the structure contains a color space.  Not set if the structure
 * contains a single color key.
 */
#define DDCKEY_COLORSPACE       0x00000001l

/*
 * Set if the structure specifies a color key or color space which is to be
 * used as a destination color key for blt operations.
 */
#define DDCKEY_DESTBLT          0x00000002l

/*
 * Set if the structure specifies a color key or color space which is to be
 * used as a destination color key for overlay operations.
 */
#define DDCKEY_DESTOVERLAY      0x00000004l

/*
 * Set if the structure specifies a color key or color space which is to be
 * used as a source color key for blt operations.
 */
#define DDCKEY_SRCBLT           0x00000008l

/*
 * Set if the structure specifies a color key or color space which is to be
 * used as a source color key for overlay operations.
 */
#define DDCKEY_SRCOVERLAY       0x00000010l


/****************************************************************************
 *
 * DIRECTDRAW COLOR KEY CAPABILITY FLAGS
 *
 ****************************************************************************/

/*
 * Supports transparent blting using a color key to identify the replaceable
 * bits of the destination surface for RGB colors.
 */
#define DDCKEYCAPS_DESTBLT                      0x00000001l

/*
 * Supports transparent blting using a color space to identify the replaceable
 * bits of the destination surface for RGB colors.
 */
#define DDCKEYCAPS_DESTBLTCLRSPACE              0x00000002l

/*
 * Supports transparent blting using a color space to identify the replaceable
 * bits of the destination surface for YUV colors.
 */
#define DDCKEYCAPS_DESTBLTCLRSPACEYUV           0x00000004l

/*
 * Supports transparent blting using a color key to identify the replaceable
 * bits of the destination surface for YUV colors.
 */
#define DDCKEYCAPS_DESTBLTYUV                   0x00000008l

/*
 * Supports overlaying using colorkeying of the replaceable bits of the surface
 * being overlayed for RGB colors.
 */
#define DDCKEYCAPS_DESTOVERLAY                  0x00000010l

/*
 * Supports a color space as the color key for the destination for RGB colors.
 */
#define DDCKEYCAPS_DESTOVERLAYCLRSPACE          0x00000020l

/*
 * Supports a color space as the color key for the destination for YUV colors.
 */
#define DDCKEYCAPS_DESTOVERLAYCLRSPACEYUV       0x00000040l

/*
 * Supports only one active destination color key value for visible overlay
 * surfaces.
 */
#define DDCKEYCAPS_DESTOVERLAYONEACTIVE         0x00000080l

/*
 * Supports overlaying using colorkeying of the replaceable bits of the
 * surface being overlayed for YUV colors.
 */
#define DDCKEYCAPS_DESTOVERLAYYUV               0x00000100l

/*
 * Supports transparent blting using the color key for the source with
 * this surface for RGB colors.
 */
#define DDCKEYCAPS_SRCBLT                       0x00000200l

/*
 * Supports transparent blting using a color space for the source with
 * this surface for RGB colors.
 */
#define DDCKEYCAPS_SRCBLTCLRSPACE               0x00000400l

/*
 * Supports transparent blting using a color space for the source with
 * this surface for YUV colors.
 */
#define DDCKEYCAPS_SRCBLTCLRSPACEYUV            0x00000800l

/*
 * Supports transparent blting using the color key for the source with
 * this surface for YUV colors.
 */
#define DDCKEYCAPS_SRCBLTYUV                    0x00001000l

/*
 * Supports overlays using the color key for the source with this
 * overlay surface for RGB colors.
 */
#define DDCKEYCAPS_SRCOVERLAY                   0x00002000l

/*
 * Supports overlays using a color space as the source color key for
 * the overlay surface for RGB colors.
 */
#define DDCKEYCAPS_SRCOVERLAYCLRSPACE           0x00004000l

/*
 * Supports overlays using a color space as the source color key for
 * the overlay surface for YUV colors.
 */
#define DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV        0x00008000l

/*
 * Supports only one active source color key value for visible
 * overlay surfaces.
 */
#define DDCKEYCAPS_SRCOVERLAYONEACTIVE          0x00010000l

/*
 * Supports overlays using the color key for the source with this
 * overlay surface for YUV colors.
 */
#define DDCKEYCAPS_SRCOVERLAYYUV                0x00020000l

/*
 * there are no bandwidth trade-offs for using colorkey with an overlay
 */
#define DDCKEYCAPS_NOCOSTOVERLAY                0x00040000l


/****************************************************************************
 *
 * DIRECTDRAW PIXELFORMAT FLAGS
 *
 ****************************************************************************/

/*
 * The surface has alpha channel information in the pixel format.
 */
#define DDPF_ALPHAPIXELS                        0x00000001l

/*
 * The pixel format contains alpha only information
 */
#define DDPF_ALPHA                              0x00000002l

/*
 * The FourCC code is valid.
 */
#define DDPF_FOURCC                             0x00000004l

/*
 * The surface is 4-bit color indexed.
 */
#define DDPF_PALETTEINDEXED4                    0x00000008l

/*
 * The surface is indexed into a palette which stores indices
 * into the destination surface's 8-bit palette.
 */
#define DDPF_PALETTEINDEXEDTO8                  0x00000010l

/*
 * The surface is 8-bit color indexed.
 */
#define DDPF_PALETTEINDEXED8                    0x00000020l

/*
 * The RGB data in the pixel format structure is valid.
 */
#define DDPF_RGB                                0x00000040l

/*
 * The surface will accept pixel data in the format specified
 * and compress it during the write.
 */
#define DDPF_COMPRESSED                         0x00000080l

/*
 * The surface will accept RGB data and translate it during
 * the write to YUV data.  The format of the data to be written
 * will be contained in the pixel format structure.  The DDPF_RGB
 * flag will be set.
 */
#define DDPF_RGBTOYUV                           0x00000100l

/*
 * pixel format is YUV - YUV data in pixel format struct is valid
 */
#define DDPF_YUV                                0x00000200l

/*
 * pixel format is a z buffer only surface
 */
#define DDPF_ZBUFFER                            0x00000400l

/*
 * The surface is 1-bit color indexed.
 */
#define DDPF_PALETTEINDEXED1                    0x00000800l

/*
 * The surface is 2-bit color indexed.
 */
#define DDPF_PALETTEINDEXED2                    0x00001000l

/*
 * The surface contains Z information in the pixels
 */
#define DDPF_ZPIXELS                            0x00002000l

/*
 * The surface contains stencil information along with Z
 */
#define DDPF_STENCILBUFFER                      0x00004000l

/*
 * Premultiplied alpha format -- the color components have been
 * premultiplied by the alpha component.
 */
#define DDPF_ALPHAPREMULT                       0x00008000l


/*
 * Luminance data in the pixel format is valid.
 * Use this flag for luminance-only or luminance+alpha surfaces,
 * the bit depth is then ddpf.dwLuminanceBitCount.
 */
#define DDPF_LUMINANCE                          0x00020000l

/*
 * Luminance data in the pixel format is valid.
 * Use this flag when hanging luminance off bumpmap surfaces,
 * the bit mask for the luminance portion of the pixel is then
 * ddpf.dwBumpLuminanceBitMask
 */
#define DDPF_BUMPLUMINANCE                      0x00040000l

/*
 * Bump map dUdV data in the pixel format is valid.
 */
#define DDPF_BUMPDUDV                           0x00080000l


/*===========================================================================
 *
 *
 * DIRECTDRAW CALLBACK FLAGS
 *
 *
 *==========================================================================*/

/****************************************************************************
 *
 * DIRECTDRAW ENUMSURFACES FLAGS
 *
 ****************************************************************************/

/*
 * Enumerate all of the surfaces that meet the search criterion.
 */
#define DDENUMSURFACES_ALL                      0x00000001l

/*
 * A search hit is a surface that matches the surface description.
 */
#define DDENUMSURFACES_MATCH                    0x00000002l

/*
 * A search hit is a surface that does not match the surface description.
 */
#define DDENUMSURFACES_NOMATCH                  0x00000004l

/*
 * Enumerate the first surface that can be created which meets the search criterion.
 */
#define DDENUMSURFACES_CANBECREATED             0x00000008l

/*
 * Enumerate the surfaces that already exist that meet the search criterion.
 */
#define DDENUMSURFACES_DOESEXIST                0x00000010l


/****************************************************************************
 *
 * DIRECTDRAW SETDISPLAYMODE FLAGS
 *
 ****************************************************************************/

/*
 * The desired mode is a standard VGA mode
 */
#define DDSDM_STANDARDVGAMODE                   0x00000001l


/****************************************************************************
 *
 * DIRECTDRAW ENUMDISPLAYMODES FLAGS
 *
 ****************************************************************************/

/*
 * Enumerate Modes with different refresh rates.  EnumDisplayModes guarantees
 * that a particular mode will be enumerated only once.  This flag specifies whether
 * the refresh rate is taken into account when determining if a mode is unique.
 */
#define DDEDM_REFRESHRATES                      0x00000001l

/*
 * Enumerate VGA modes. Specify this flag if you wish to enumerate supported VGA
 * modes such as mode 0x13 in addition to the usual ModeX modes (which are always
 * enumerated if the application has previously called SetCooperativeLevel with the
 * DDSCL_ALLOWMODEX flag set).
 */
#define DDEDM_STANDARDVGAMODES                  0x00000002L


/****************************************************************************
 *
 * DIRECTDRAW SETCOOPERATIVELEVEL FLAGS
 *
 ****************************************************************************/

/*
 * Exclusive mode owner will be responsible for the entire primary surface.
 * GDI can be ignored. used with DD
 */
#define DDSCL_FULLSCREEN                        0x00000001l

/*
 * allow CTRL_ALT_DEL to work while in fullscreen exclusive mode
 */
#define DDSCL_ALLOWREBOOT                       0x00000002l

/*
 * prevents DDRAW from modifying the application window.
 * prevents DDRAW from minimize/restore the application window on activation.
 */
#define DDSCL_NOWINDOWCHANGES                   0x00000004l

/*
 * app wants to work as a regular Windows application
 */
#define DDSCL_NORMAL                            0x00000008l

/*
 * app wants exclusive access
 */
#define DDSCL_EXCLUSIVE                         0x00000010l


/*
 * app can deal with non-windows display modes
 */
#define DDSCL_ALLOWMODEX                        0x00000040l

/*
 * this window will receive the focus messages
 */
#define DDSCL_SETFOCUSWINDOW                    0x00000080l

/*
 * this window is associated with the DDRAW object and will
 * cover the screen in fullscreen mode
 */
#define DDSCL_SETDEVICEWINDOW                   0x00000100l

/*
 * app wants DDRAW to create a window to be associated with the
 * DDRAW object
 */
#define DDSCL_CREATEDEVICEWINDOW                0x00000200l

/*
 * App explicitly asks DDRAW/D3D to be multithread safe. This makes D3D
 * take the global crtisec more frequently.
 */
#define DDSCL_MULTITHREADED                     0x00000400l

/*
 * App specifies that it would like to keep the FPU set up for optimal Direct3D
 * performance (single precision and exceptions disabled) so Direct3D
 * does not need to explicitly set the FPU each time. This is assumed by
 * default in DirectX 7. See also DDSCL_FPUPRESERVE
 */
#define DDSCL_FPUSETUP                          0x00000800l

/*
 * App specifies that it needs either double precision FPU or FPU exceptions
 * enabled. This makes Direct3D explicitly set the FPU state eah time it is
 * called. Setting the flag will reduce Direct3D performance. The flag is
 * assumed by default in DirectX 6 and earlier. See also DDSCL_FPUSETUP
 */
#define DDSCL_FPUPRESERVE                          0x00001000l


/****************************************************************************
 *
 * DIRECTDRAW BLT FLAGS
 *
 ****************************************************************************/

/*
 * Use the alpha information in the pixel format or the alpha channel surface
 * attached to the destination surface as the alpha channel for this blt.
 */
#define DDBLT_ALPHADEST                         0x00000001l

/*
 * Use the dwConstAlphaDest field in the DDBLTFX structure as the alpha channel
 * for the destination surface for this blt.
 */
#define DDBLT_ALPHADESTCONSTOVERRIDE            0x00000002l

/*
 * The NEG suffix indicates that the destination surface becomes more
 * transparent as the alpha value increases. (0 is opaque)
 */
#define DDBLT_ALPHADESTNEG                      0x00000004l

/*
 * Use the lpDDSAlphaDest field in the DDBLTFX structure as the alpha
 * channel for the destination for this blt.
 */
#define DDBLT_ALPHADESTSURFACEOVERRIDE          0x00000008l

/*
 * Use the dwAlphaEdgeBlend field in the DDBLTFX structure as the alpha channel
 * for the edges of the image that border the color key colors.
 */
#define DDBLT_ALPHAEDGEBLEND                    0x00000010l

/*
 * Use the alpha information in the pixel format or the alpha channel surface
 * attached to the source surface as the alpha channel for this blt.
 */
#define DDBLT_ALPHASRC                          0x00000020l

/*
 * Use the dwConstAlphaSrc field in the DDBLTFX structure as the alpha channel
 * for the source for this blt.
 */
#define DDBLT_ALPHASRCCONSTOVERRIDE             0x00000040l

/*
 * The NEG suffix indicates that the source surface becomes more transparent
 * as the alpha value increases. (0 is opaque)
 */
#define DDBLT_ALPHASRCNEG                       0x00000080l

/*
 * Use the lpDDSAlphaSrc field in the DDBLTFX structure as the alpha channel
 * for the source for this blt.
 */
#define DDBLT_ALPHASRCSURFACEOVERRIDE           0x00000100l

/*
 * Do this blt asynchronously through the FIFO in the order received.  If
 * there is no room in the hardware FIFO fail the call.
 */
#define DDBLT_ASYNC                             0x00000200l

/*
 * Uses the dwFillColor field in the DDBLTFX structure as the RGB color
 * to fill the destination rectangle on the destination surface with.
 */
#define DDBLT_COLORFILL                         0x00000400l

/*
 * Uses the dwDDFX field in the DDBLTFX structure to specify the effects
 * to use for the blt.
 */
#define DDBLT_DDFX                              0x00000800l

/*
 * Uses the dwDDROPS field in the DDBLTFX structure to specify the ROPS
 * that are not part of the Win32 API.
 */
#define DDBLT_DDROPS                            0x00001000l

/*
 * Use the color key associated with the destination surface.
 */
#define DDBLT_KEYDEST                           0x00002000l

/*
 * Use the dckDestColorkey field in the DDBLTFX structure as the color key
 * for the destination surface.
 */
#define DDBLT_KEYDESTOVERRIDE                   0x00004000l

/*
 * Use the color key associated with the source surface.
 */
#define DDBLT_KEYSRC                            0x00008000l

/*
 * Use the dckSrcColorkey field in the DDBLTFX structure as the color key
 * for the source surface.
 */
#define DDBLT_KEYSRCOVERRIDE                    0x00010000l

/*
 * Use the dwROP field in the DDBLTFX structure for the raster operation
 * for this blt.  These ROPs are the same as the ones defined in the Win32 API.
 */
#define DDBLT_ROP                               0x00020000l

/*
 * Use the dwRotationAngle field in the DDBLTFX structure as the angle
 * (specified in 1/100th of a degree) to rotate the surface.
 */
#define DDBLT_ROTATIONANGLE                     0x00040000l

/*
 * Z-buffered blt using the z-buffers attached to the source and destination
 * surfaces and the dwZBufferOpCode field in the DDBLTFX structure as the
 * z-buffer opcode.
 */
#define DDBLT_ZBUFFER                           0x00080000l

/*
 * Z-buffered blt using the dwConstDest Zfield and the dwZBufferOpCode field
 * in the DDBLTFX structure as the z-buffer and z-buffer opcode respectively
 * for the destination.
 */
#define DDBLT_ZBUFFERDESTCONSTOVERRIDE          0x00100000l

/*
 * Z-buffered blt using the lpDDSDestZBuffer field and the dwZBufferOpCode
 * field in the DDBLTFX structure as the z-buffer and z-buffer opcode
 * respectively for the destination.
 */
#define DDBLT_ZBUFFERDESTOVERRIDE               0x00200000l

/*
 * Z-buffered blt using the dwConstSrcZ field and the dwZBufferOpCode field
 * in the DDBLTFX structure as the z-buffer and z-buffer opcode respectively
 * for the source.
 */
#define DDBLT_ZBUFFERSRCCONSTOVERRIDE           0x00400000l

/*
 * Z-buffered blt using the lpDDSSrcZBuffer field and the dwZBufferOpCode
 * field in the DDBLTFX structure as the z-buffer and z-buffer opcode
 * respectively for the source.
 */
#define DDBLT_ZBUFFERSRCOVERRIDE                0x00800000l

/*
 * wait until the device is ready to handle the blt
 * this will cause blt to not return DDERR_WASSTILLDRAWING
 */
#define DDBLT_WAIT                              0x01000000l

/*
 * Uses the dwFillDepth field in the DDBLTFX structure as the depth value
 * to fill the destination rectangle on the destination Z-buffer surface
 * with.
 */
#define DDBLT_DEPTHFILL                         0x02000000l


/*
 * Return immediately (with DDERR_WASSTILLDRAWING) if the device is not
 * ready to schedule the blt at the time Blt() is called.
 */
#define DDBLT_DONOTWAIT                         0x08000000l

/*
 * These flags indicate a presentation blt (i.e. a blt
 * that moves surface contents from an offscreen back buffer to the primary
 * surface). The driver is not allowed to "queue"  more than three such blts.
 * The "end" of the presentation blt is indicated, since the
 * blt may be clipped, in which case the runtime will call the driver with 
 * several blts. All blts (even if not clipped) are tagged with DDBLT_PRESENTATION
 * and the last (even if not clipped) additionally with DDBLT_LAST_PRESENTATION.
 * Thus the true rule is that the driver must not schedule a DDBLT_PRESENTATION
 * blt if there are 3 or more DDBLT_PRESENTLAST blts in the hardware pipe.
 * If there are such blts in the pipe, the driver should return DDERR_WASSTILLDRAWING
 * until the oldest queued DDBLT_LAST_PRESENTATION blts has been retired (i.e. the
 * pixels have been actually written to the primary surface). Once the oldest blt
 * has been retired, the driver is free to schedule the current blt.
 * The goal is to provide a mechanism whereby the device's hardware queue never
 * gets more than 3 frames ahead of the frames being generated by the application.
 * When excessive queueing occurs, applications become unusable because the application
 * visibly lags user input, and such problems make windowed interactive applications impossible.
 * Some drivers may not have sufficient knowledge of their hardware's FIFO to know
 * when a certain blt has been retired. Such drivers should code cautiously, and 
 * simply not allow any frames to be queued at all. DDBLT_LAST_PRESENTATION should cause
 * such drivers to return DDERR_WASSTILLDRAWING until the accelerator is completely
 * finished- exactly as if the application had called Lock on the source surface
 * before calling Blt. 
 * In other words, the driver is allowed and encouraged to 
 * generate as much latency as it can, but never more than 3 frames worth.
 * Implementation detail: Drivers should count blts against the SOURCE surface, not
 * against the primary surface. This enables multiple parallel windowed application
 * to function more optimally.
 * This flag is passed only to DX8 or higher drivers.
 *
 * APPLICATIONS DO NOT SET THESE FLAGS. THEY ARE SET BY THE DIRECTDRAW RUNTIME.
 * 
 */
#define DDBLT_PRESENTATION                      0x10000000l
#define DDBLT_LAST_PRESENTATION                 0x20000000l

/*
 * If DDBLT_EXTENDED_FLAGS is set, then the driver should re-interpret
 * other flags according to the definitions that follow.
 * For example, bit 0 (0x00000001L) means DDBLT_ALPHADEST, unless
 * DDBLT_EXTENDED_FLAGS is also set, in which case bit 0 means
 * DDBLT_EXTENDED_LINEAR_CONTENT.
 * Only DirectX9 and higher drivers will be given extended blt flags.
 * Only flags explicitly mentioned here should be re-interpreted.
 * All other flags retain their original meanings.
 *
 * List of re-interpreted flags:
 *
 * Bit Hex value   New meaning                                  old meaning
 * ---------------------------------------------------------------
 *  2  0x00000004  DDBLT_EXTENDED_LINEAR_CONTENT                DDBLT_ALPHADESTNEG
 *  4  0x00000010  DDBLT_EXTENDED_PRESENTATION_STRETCHFACTOR    DDBLT_ALPHAEDGEBLEND
 *
 *
 * NOTE: APPLICATIONS SHOULD NOT SET THIS FLAG. THIS FLAG IS INTENDED
 * FOR USE BY THE DIRECT3D RUNTIME.
 */
#define DDBLT_EXTENDED_FLAGS                    0x40000000l

/*
 * EXTENDED FLAG. SEE DEFINITION OF DDBLT_EXTENDED_FLAGS.
 * This flag indidcates that the source surface contains content in a
 * linear color space. The driver may perform gamma correction to the
 * desktop color space (i.e. sRGB, gamma 2.2) as part of this blt.
 * If the device can perform such a conversion as part of the copy,
 * the driver should also set D3DCAPS3_LINEAR_TO_SRGB_PRESENTATION
 *
 * NOTE: APPLICATIONS SHOULD NOT SET THIS FLAG. THIS FLAG IS INTENDED
 * FOR USE BY THE DIRECT3D RUNTIME. Use IDirect3DSwapChain9::Present
 * and specify D3DPRESENT_LINEAR_CONTENT in order to use this functionality.
 */ 
#define DDBLT_EXTENDED_LINEAR_CONTENT           0x00000004l


/****************************************************************************
 *
 * BLTFAST FLAGS
 *
 ****************************************************************************/

#define DDBLTFAST_NOCOLORKEY                    0x00000000
#define DDBLTFAST_SRCCOLORKEY                   0x00000001
#define DDBLTFAST_DESTCOLORKEY                  0x00000002
#define DDBLTFAST_WAIT                          0x00000010
#define DDBLTFAST_DONOTWAIT                     0x00000020

/****************************************************************************
 *
 * FLIP FLAGS
 *
 ****************************************************************************/

#define DDFLIP_WAIT                          0x00000001L

/*
 * Indicates that the target surface contains the even field of video data.
 * This flag is only valid with an overlay surface.
 */
#define DDFLIP_EVEN                          0x00000002L

/*
 * Indicates that the target surface contains the odd field of video data.
 * This flag is only valid with an overlay surface.
 */
#define DDFLIP_ODD                           0x00000004L

/*
 * Causes DirectDraw to perform the physical flip immediately and return
 * to the application. Typically, what was the front buffer but is now the back
 * buffer will still be visible (depending on timing) until the next vertical
 * retrace. Subsequent operations involving the two flipped surfaces will
 * not check to see if the physical flip has finished (i.e. will not return
 * DDERR_WASSTILLDRAWING for that reason (but may for other reasons)).
 * This allows an application to perform Flips at a higher frequency than the
 * monitor refresh rate, but may introduce visible artifacts.
 * Only effective if DDCAPS2_FLIPNOVSYNC is set. If that bit is not set,
 * DDFLIP_NOVSYNC has no effect.
 */
#define DDFLIP_NOVSYNC                       0x00000008L


/*
 * Flip Interval Flags. These flags indicate how many vertical retraces to wait between
 * each flip. The default is one. DirectDraw will return DDERR_WASSTILLDRAWING for each
 * surface involved in the flip until the specified number of vertical retraces has
 * ocurred. Only effective if DDCAPS2_FLIPINTERVAL is set. If that bit is not set,
 * DDFLIP_INTERVALn has no effect.
 */

/*
 * DirectDraw will flip on every other vertical sync
 */
#define DDFLIP_INTERVAL2                     0x02000000L


/*
 * DirectDraw will flip on every third vertical sync
 */
#define DDFLIP_INTERVAL3                     0x03000000L


/*
 * DirectDraw will flip on every fourth vertical sync
 */
#define DDFLIP_INTERVAL4                     0x04000000L

/*
 * DirectDraw will flip and display a main stereo surface
 */
#define DDFLIP_STEREO                        0x00000010L

/*
 * On IDirectDrawSurface7 and higher interfaces, the default is DDFLIP_WAIT. If you wish
 * to override the default and use time when the accelerator is busy (as denoted by
 * the DDERR_WASSTILLDRAWING return code) then use DDFLIP_DONOTWAIT.
 */
#define DDFLIP_DONOTWAIT                     0x00000020L


/****************************************************************************
 *
 * DIRECTDRAW SURFACE OVERLAY FLAGS
 *
 ****************************************************************************/

/*
 * Use the alpha information in the pixel format or the alpha channel surface
 * attached to the destination surface as the alpha channel for the
 * destination overlay.
 */
#define DDOVER_ALPHADEST                        0x00000001l

/*
 * Use the dwConstAlphaDest field in the DDOVERLAYFX structure as the
 * destination alpha channel for this overlay.
 */
#define DDOVER_ALPHADESTCONSTOVERRIDE           0x00000002l

/*
 * The NEG suffix indicates that the destination surface becomes more
 * transparent as the alpha value increases.
 */
#define DDOVER_ALPHADESTNEG                     0x00000004l

/*
 * Use the lpDDSAlphaDest field in the DDOVERLAYFX structure as the alpha
 * channel destination for this overlay.
 */
#define DDOVER_ALPHADESTSURFACEOVERRIDE         0x00000008l

/*
 * Use the dwAlphaEdgeBlend field in the DDOVERLAYFX structure as the alpha
 * channel for the edges of the image that border the color key colors.
 */
#define DDOVER_ALPHAEDGEBLEND                   0x00000010l

/*
 * Use the alpha information in the pixel format or the alpha channel surface
 * attached to the source surface as the source alpha channel for this overlay.
 */
#define DDOVER_ALPHASRC                         0x00000020l

/*
 * Use the dwConstAlphaSrc field in the DDOVERLAYFX structure as the source
 * alpha channel for this overlay.
 */
#define DDOVER_ALPHASRCCONSTOVERRIDE            0x00000040l

/*
 * The NEG suffix indicates that the source surface becomes more transparent
 * as the alpha value increases.
 */
#define DDOVER_ALPHASRCNEG                      0x00000080l

/*
 * Use the lpDDSAlphaSrc field in the DDOVERLAYFX structure as the alpha channel
 * source for this overlay.
 */
#define DDOVER_ALPHASRCSURFACEOVERRIDE          0x00000100l

/*
 * Turn this overlay off.
 */
#define DDOVER_HIDE                             0x00000200l

/*
 * Use the color key associated with the destination surface.
 */
#define DDOVER_KEYDEST                          0x00000400l

/*
 * Use the dckDestColorkey field in the DDOVERLAYFX structure as the color key
 * for the destination surface
 */
#define DDOVER_KEYDESTOVERRIDE                  0x00000800l

/*
 * Use the color key associated with the source surface.
 */
#define DDOVER_KEYSRC                           0x00001000l

/*
 * Use the dckSrcColorkey field in the DDOVERLAYFX structure as the color key
 * for the source surface.
 */
#define DDOVER_KEYSRCOVERRIDE                   0x00002000l

/*
 * Turn this overlay on.
 */
#define DDOVER_SHOW                             0x00004000l

/*
 * Add a dirty rect to an emulated overlayed surface.
 */
#define DDOVER_ADDDIRTYRECT                     0x00008000l

/*
 * Redraw all dirty rects on an emulated overlayed surface.
 */
#define DDOVER_REFRESHDIRTYRECTS                0x00010000l

/*
 * Redraw the entire surface on an emulated overlayed surface.
 */
#define DDOVER_REFRESHALL                      0x00020000l


/*
 * Use the overlay FX flags to define special overlay FX
 */
#define DDOVER_DDFX                             0x00080000l

/*
 * Autoflip the overlay when ever the video port autoflips
 */
#define DDOVER_AUTOFLIP                         0x00100000l

/*
 * Display each field of video port data individually without
 * causing any jittery artifacts
 */
#define DDOVER_BOB                              0x00200000l

/*
 * Indicates that bob/weave decisions should not be overridden by other
 * interfaces.
 */
#define DDOVER_OVERRIDEBOBWEAVE                 0x00400000l

/*
 * Indicates that the surface memory is composed of interleaved fields.
 */
#define DDOVER_INTERLEAVED                      0x00800000l

/*
 * Indicates that bob will be performed using hardware rather than
 * software or emulated.
 */
#define DDOVER_BOBHARDWARE                      0x01000000l

/*
 * Indicates that overlay FX structure contains valid ARGB scaling factors.
 */
#define DDOVER_ARGBSCALEFACTORS                 0x02000000l

/*
 * Indicates that ARGB scaling factors can be degraded to fit driver capabilities.
 */
#define DDOVER_DEGRADEARGBSCALING               0x04000000l


/****************************************************************************
 *
 * DIRECTDRAWSURFACE LOCK FLAGS
 *
 ****************************************************************************/

/*
 * The default.  Set to indicate that Lock should return a valid memory pointer
 * to the top of the specified rectangle.  If no rectangle is specified then a
 * pointer to the top of the surface is returned.
 */
#define DDLOCK_SURFACEMEMORYPTR                 0x00000000L     // default

/*
 * Set to indicate that Lock should wait until it can obtain a valid memory
 * pointer before returning.  If this bit is set, Lock will never return
 * DDERR_WASSTILLDRAWING.
 */
#define DDLOCK_WAIT                             0x00000001L

/*
 * Set if an event handle is being passed to Lock.  Lock will trigger the event
 * when it can return the surface memory pointer requested.
 */
#define DDLOCK_EVENT                            0x00000002L

/*
 * Indicates that the surface being locked will only be read from.
 */
#define DDLOCK_READONLY                         0x00000010L

/*
 * Indicates that the surface being locked will only be written to
 */
#define DDLOCK_WRITEONLY                        0x00000020L


/*
 * Indicates that a system wide lock should not be taken when this surface
 * is locked. This has several advantages (cursor responsiveness, ability
 * to call more Windows functions, easier debugging) when locking video
 * memory surfaces. However, an application specifying this flag must
 * comply with a number of conditions documented in the help file.
 * Furthermore, this flag cannot be specified when locking the primary.
 */
#define DDLOCK_NOSYSLOCK                        0x00000800L

/*
 * Used only with Direct3D Vertex Buffer Locks. Indicates that no vertices
 * that were referred to in Draw*PrimtiveVB calls since the start of the
 * frame (or the last lock without this flag) will be modified during the
 * lock. This can be useful when one is only appending data to the vertex
 * buffer
 */
#define DDLOCK_NOOVERWRITE                      0x00001000L

/*
 * Indicates that no assumptions will be made about the contents of the
 * surface or vertex buffer during this lock.
 * This enables two things:
 * -    Direct3D or the driver may provide an alternative memory
 *      area as the vertex buffer. This is useful when one plans to clear the
 *      contents of the vertex buffer and fill in new data.
 * -    Drivers sometimes store surface data in a re-ordered format.
 *      When the application locks the surface, the driver is forced to un-re-order
 *      the surface data before allowing the application to see the surface contents.
 *      This flag is a hint to the driver that it can skip the un-re-ordering process
 *      since the application plans to overwrite every single pixel in the surface
 *      or locked rectangle (and so erase any un-re-ordered pixels anyway).
 *      Applications should always set this flag when they intend to overwrite the entire
 *      surface or locked rectangle.
 */
#define DDLOCK_DISCARDCONTENTS                  0x00002000L
 /*
  * DDLOCK_OKTOSWAP is an older, less informative name for DDLOCK_DISCARDCONTENTS
  */
#define DDLOCK_OKTOSWAP                         0x00002000L

/*
 * On IDirectDrawSurface7 and higher interfaces, the default is DDLOCK_WAIT. If you wish
 * to override the default and use time when the accelerator is busy (as denoted by
 * the DDERR_WASSTILLDRAWING return code) then use DDLOCK_DONOTWAIT.
 */
#define DDLOCK_DONOTWAIT                        0x00004000L

/*
 * This indicates volume texture lock with front and back specified.
 */
#define DDLOCK_HASVOLUMETEXTUREBOXRECT          0x00008000L

/*
 * This indicates that the driver should not update dirty rect information for this lock.
 */
#define DDLOCK_NODIRTYUPDATE                    0x00010000L


/****************************************************************************
 *
 * DIRECTDRAWSURFACE PAGELOCK FLAGS
 *
 ****************************************************************************/

/*
 * No flags defined at present
 */


/****************************************************************************
 *
 * DIRECTDRAWSURFACE PAGEUNLOCK FLAGS
 *
 ****************************************************************************/

/*
 * No flags defined at present
 */


/****************************************************************************
 *
 * DIRECTDRAWSURFACE BLT FX FLAGS
 *
 ****************************************************************************/

/*
 * If stretching, use arithmetic stretching along the Y axis for this blt.
 */
#define DDBLTFX_ARITHSTRETCHY                   0x00000001l

/*
 * Do this blt mirroring the surface left to right.  Spin the
 * surface around its y-axis.
 */
#define DDBLTFX_MIRRORLEFTRIGHT                 0x00000002l

/*
 * Do this blt mirroring the surface up and down.  Spin the surface
 * around its x-axis.
 */
#define DDBLTFX_MIRRORUPDOWN                    0x00000004l

/*
 * Schedule this blt to avoid tearing.
 */
#define DDBLTFX_NOTEARING                       0x00000008l

/*
 * Do this blt rotating the surface one hundred and eighty degrees.
 */
#define DDBLTFX_ROTATE180                       0x00000010l

/*
 * Do this blt rotating the surface two hundred and seventy degrees.
 */
#define DDBLTFX_ROTATE270                       0x00000020l

/*
 * Do this blt rotating the surface ninety degrees.
 */
#define DDBLTFX_ROTATE90                        0x00000040l

/*
 * Do this z blt using dwZBufferLow and dwZBufferHigh as  range values
 * specified to limit the bits copied from the source surface.
 */
#define DDBLTFX_ZBUFFERRANGE                    0x00000080l

/*
 * Do this z blt adding the dwZBufferBaseDest to each of the sources z values
 * before comparing it with the desting z values.
 */
#define DDBLTFX_ZBUFFERBASEDEST                 0x00000100l

/****************************************************************************
 *
 * DIRECTDRAWSURFACE OVERLAY FX FLAGS
 *
 ****************************************************************************/

/*
 * If stretching, use arithmetic stretching along the Y axis for this overlay.
 */
#define DDOVERFX_ARITHSTRETCHY                  0x00000001l

/*
 * Mirror the overlay across the vertical axis
 */
#define DDOVERFX_MIRRORLEFTRIGHT                0x00000002l

/*
 * Mirror the overlay across the horizontal axis
 */
#define DDOVERFX_MIRRORUPDOWN                   0x00000004l

/*
 * Deinterlace the overlay, if possible
 */
#define DDOVERFX_DEINTERLACE                    0x00000008l


/****************************************************************************
 *
 * DIRECTDRAW WAITFORVERTICALBLANK FLAGS
 *
 ****************************************************************************/

/*
 * return when the vertical blank interval begins
 */
#define DDWAITVB_BLOCKBEGIN                     0x00000001l

/*
 * set up an event to trigger when the vertical blank begins
 */
#define DDWAITVB_BLOCKBEGINEVENT                0x00000002l

/*
 * return when the vertical blank interval ends and display begins
 */
#define DDWAITVB_BLOCKEND                       0x00000004l

/****************************************************************************
 *
 * DIRECTDRAW GETFLIPSTATUS FLAGS
 *
 ****************************************************************************/

/*
 * is it OK to flip now?
 */
#define DDGFS_CANFLIP                   0x00000001l

/*
 * is the last flip finished?
 */
#define DDGFS_ISFLIPDONE                0x00000002l

/****************************************************************************
 *
 * DIRECTDRAW GETBLTSTATUS FLAGS
 *
 ****************************************************************************/

/*
 * is it OK to blt now?
 */
#define DDGBS_CANBLT                    0x00000001l

/*
 * is the blt to the surface finished?
 */
#define DDGBS_ISBLTDONE                 0x00000002l


/****************************************************************************
 *
 * DIRECTDRAW ENUMOVERLAYZORDER FLAGS
 *
 ****************************************************************************/

/*
 * Enumerate overlays back to front.
 */
#define DDENUMOVERLAYZ_BACKTOFRONT      0x00000000l

/*
 * Enumerate overlays front to back
 */
#define DDENUMOVERLAYZ_FRONTTOBACK      0x00000001l

/****************************************************************************
 *
 * DIRECTDRAW UPDATEOVERLAYZORDER FLAGS
 *
 ****************************************************************************/

/*
 * Send overlay to front
 */
#define DDOVERZ_SENDTOFRONT             0x00000000l

/*
 * Send overlay to back
 */
#define DDOVERZ_SENDTOBACK              0x00000001l

/*
 * Move Overlay forward
 */
#define DDOVERZ_MOVEFORWARD             0x00000002l

/*
 * Move Overlay backward
 */
#define DDOVERZ_MOVEBACKWARD            0x00000003l

/*
 * Move Overlay in front of relative surface
 */
#define DDOVERZ_INSERTINFRONTOF         0x00000004l

/*
 * Move Overlay in back of relative surface
 */
#define DDOVERZ_INSERTINBACKOF          0x00000005l


/****************************************************************************
 *
 * DIRECTDRAW SETGAMMARAMP FLAGS
 *
 ****************************************************************************/

/*
 * Request calibrator to adjust the gamma ramp according to the physical
 * properties of the display so that the result should appear identical
 * on all systems.
 */
#define DDSGR_CALIBRATE                        0x00000001L


/****************************************************************************
 *
 * DIRECTDRAW STARTMODETEST FLAGS
 *
 ****************************************************************************/

/*
 * Indicates that the mode being tested has passed
 */
#define DDSMT_ISTESTREQUIRED                   0x00000001L


/****************************************************************************
 *
 * DIRECTDRAW EVALUATEMODE FLAGS
 *
 ****************************************************************************/

/*
 * Indicates that the mode being tested has passed
 */
#define DDEM_MODEPASSED                        0x00000001L

/*
 * Indicates that the mode being tested has failed
 */
#define DDEM_MODEFAILED                        0x00000002L


/*===========================================================================
 *
 *
 * DIRECTDRAW RETURN CODES
 *
 * The return values from DirectDraw Commands and Surface that return an HRESULT
 * are codes from DirectDraw concerning the results of the action
 * requested by DirectDraw.
 *
 *==========================================================================*/

/*
 * Status is OK
 *
 * Issued by: DirectDraw Commands and all callbacks
 */
#define DD_OK                                   S_OK
#define DD_FALSE                                S_FALSE

/****************************************************************************
 *
 * DIRECTDRAW ENUMCALLBACK RETURN VALUES
 *
 * EnumCallback returns are used to control the flow of the DIRECTDRAW and
 * DIRECTDRAWSURFACE object enumerations.   They can only be returned by
 * enumeration callback routines.
 *
 ****************************************************************************/

/*
 * stop the enumeration
 */
#define DDENUMRET_CANCEL                        0

/*
 * continue the enumeration
 */
#define DDENUMRET_OK                            1

/****************************************************************************
 *
 * DIRECTDRAW ERRORS
 *
 * Errors are represented by negative values and cannot be combined.
 *
 ****************************************************************************/

/*
 * This object is already initialized
 */
#define DDERR_ALREADYINITIALIZED                MAKE_DDHRESULT( 5 )

/*
 * This surface can not be attached to the requested surface.
 */
#define DDERR_CANNOTATTACHSURFACE               MAKE_DDHRESULT( 10 )

/*
 * This surface can not be detached from the requested surface.
 */
#define DDERR_CANNOTDETACHSURFACE               MAKE_DDHRESULT( 20 )

/*
 * Support is currently not available.
 */
#define DDERR_CURRENTLYNOTAVAIL                 MAKE_DDHRESULT( 40 )

/*
 * An exception was encountered while performing the requested operation
 */
#define DDERR_EXCEPTION                         MAKE_DDHRESULT( 55 )

/*
 * Generic failure.
 */
#define DDERR_GENERIC                           E_FAIL

/*
 * Height of rectangle provided is not a multiple of reqd alignment
 */
#define DDERR_HEIGHTALIGN                       MAKE_DDHRESULT( 90 )

/*
 * Unable to match primary surface creation request with existing
 * primary surface.
 */
#define DDERR_INCOMPATIBLEPRIMARY               MAKE_DDHRESULT( 95 )

/*
 * One or more of the caps bits passed to the callback are incorrect.
 */
#define DDERR_INVALIDCAPS                       MAKE_DDHRESULT( 100 )

/*
 * DirectDraw does not support provided Cliplist.
 */
#define DDERR_INVALIDCLIPLIST                   MAKE_DDHRESULT( 110 )

/*
 * DirectDraw does not support the requested mode
 */
#define DDERR_INVALIDMODE                       MAKE_DDHRESULT( 120 )

/*
 * DirectDraw received a pointer that was an invalid DIRECTDRAW object.
 */
#define DDERR_INVALIDOBJECT                     MAKE_DDHRESULT( 130 )

/*
 * One or more of the parameters passed to the callback function are
 * incorrect.
 */
#define DDERR_INVALIDPARAMS                     E_INVALIDARG

/*
 * pixel format was invalid as specified
 */
#define DDERR_INVALIDPIXELFORMAT                MAKE_DDHRESULT( 145 )

/*
 * Rectangle provided was invalid.
 */
#define DDERR_INVALIDRECT                       MAKE_DDHRESULT( 150 )

/*
 * Operation could not be carried out because one or more surfaces are locked
 */
#define DDERR_LOCKEDSURFACES                    MAKE_DDHRESULT( 160 )

/*
 * There is no 3D present.
 */
#define DDERR_NO3D                              MAKE_DDHRESULT( 170 )

/*
 * Operation could not be carried out because there is no alpha accleration
 * hardware present or available.
 */
#define DDERR_NOALPHAHW                         MAKE_DDHRESULT( 180 )

/*
 * Operation could not be carried out because there is no stereo
 * hardware present or available.
 */
#define DDERR_NOSTEREOHARDWARE          MAKE_DDHRESULT( 181 )

/*
 * Operation could not be carried out because there is no hardware
 * present which supports stereo surfaces
 */
#define DDERR_NOSURFACELEFT                             MAKE_DDHRESULT( 182 )



/*
 * no clip list available
 */
#define DDERR_NOCLIPLIST                        MAKE_DDHRESULT( 205 )

/*
 * Operation could not be carried out because there is no color conversion
 * hardware present or available.
 */
#define DDERR_NOCOLORCONVHW                     MAKE_DDHRESULT( 210 )

/*
 * Create function called without DirectDraw object method SetCooperativeLevel
 * being called.
 */
#define DDERR_NOCOOPERATIVELEVELSET             MAKE_DDHRESULT( 212 )

/*
 * Surface doesn't currently have a color key
 */
#define DDERR_NOCOLORKEY                        MAKE_DDHRESULT( 215 )

/*
 * Operation could not be carried out because there is no hardware support
 * of the dest color key.
 */
#define DDERR_NOCOLORKEYHW                      MAKE_DDHRESULT( 220 )

/*
 * No DirectDraw support possible with current display driver
 */
#define DDERR_NODIRECTDRAWSUPPORT               MAKE_DDHRESULT( 222 )

/*
 * Operation requires the application to have exclusive mode but the
 * application does not have exclusive mode.
 */
#define DDERR_NOEXCLUSIVEMODE                   MAKE_DDHRESULT( 225 )

/*
 * Flipping visible surfaces is not supported.
 */
#define DDERR_NOFLIPHW                          MAKE_DDHRESULT( 230 )

/*
 * There is no GDI present.
 */
#define DDERR_NOGDI                             MAKE_DDHRESULT( 240 )

/*
 * Operation could not be carried out because there is no hardware present
 * or available.
 */
#define DDERR_NOMIRRORHW                        MAKE_DDHRESULT( 250 )

/*
 * Requested item was not found
 */
#define DDERR_NOTFOUND                          MAKE_DDHRESULT( 255 )

/*
 * Operation could not be carried out because there is no overlay hardware
 * present or available.
 */
#define DDERR_NOOVERLAYHW                       MAKE_DDHRESULT( 260 )

/*
 * Operation could not be carried out because the source and destination
 * rectangles are on the same surface and overlap each other.
 */
#define DDERR_OVERLAPPINGRECTS                  MAKE_DDHRESULT( 270 )

/*
 * Operation could not be carried out because there is no appropriate raster
 * op hardware present or available.
 */
#define DDERR_NORASTEROPHW                      MAKE_DDHRESULT( 280 )

/*
 * Operation could not be carried out because there is no rotation hardware
 * present or available.
 */
#define DDERR_NOROTATIONHW                      MAKE_DDHRESULT( 290 )

/*
 * Operation could not be carried out because there is no hardware support
 * for stretching
 */
#define DDERR_NOSTRETCHHW                       MAKE_DDHRESULT( 310 )

/*
 * DirectDrawSurface is not in 4 bit color palette and the requested operation
 * requires 4 bit color palette.
 */
#define DDERR_NOT4BITCOLOR                      MAKE_DDHRESULT( 316 )

/*
 * DirectDrawSurface is not in 4 bit color index palette and the requested
 * operation requires 4 bit color index palette.
 */
#define DDERR_NOT4BITCOLORINDEX                 MAKE_DDHRESULT( 317 )

/*
 * DirectDraw Surface is not in 8 bit color mode and the requested operation
 * requires 8 bit color.
 */
#define DDERR_NOT8BITCOLOR                      MAKE_DDHRESULT( 320 )

/*
 * Operation could not be carried out because there is no texture mapping
 * hardware present or available.
 */
#define DDERR_NOTEXTUREHW                       MAKE_DDHRESULT( 330 )

/*
 * Operation could not be carried out because there is no hardware support
 * for vertical blank synchronized operations.
 */
#define DDERR_NOVSYNCHW                         MAKE_DDHRESULT( 335 )

/*
 * Operation could not be carried out because there is no hardware support
 * for zbuffer blting.
 */
#define DDERR_NOZBUFFERHW                       MAKE_DDHRESULT( 340 )

/*
 * Overlay surfaces could not be z layered based on their BltOrder because
 * the hardware does not support z layering of overlays.
 */
#define DDERR_NOZOVERLAYHW                      MAKE_DDHRESULT( 350 )

/*
 * The hardware needed for the requested operation has already been
 * allocated.
 */
#define DDERR_OUTOFCAPS                         MAKE_DDHRESULT( 360 )

/*
 * DirectDraw does not have enough memory to perform the operation.
 */
#define DDERR_OUTOFMEMORY                       E_OUTOFMEMORY

/*
 * DirectDraw does not have enough memory to perform the operation.
 */
#define DDERR_OUTOFVIDEOMEMORY                  MAKE_DDHRESULT( 380 )

/*
 * hardware does not support clipped overlays
 */
#define DDERR_OVERLAYCANTCLIP                   MAKE_DDHRESULT( 382 )

/*
 * Can only have ony color key active at one time for overlays
 */
#define DDERR_OVERLAYCOLORKEYONLYONEACTIVE      MAKE_DDHRESULT( 384 )

/*
 * Access to this palette is being refused because the palette is already
 * locked by another thread.
 */
#define DDERR_PALETTEBUSY                       MAKE_DDHRESULT( 387 )

/*
 * No src color key specified for this operation.
 */
#define DDERR_COLORKEYNOTSET                    MAKE_DDHRESULT( 400 )

/*
 * This surface is already attached to the surface it is being attached to.
 */
#define DDERR_SURFACEALREADYATTACHED            MAKE_DDHRESULT( 410 )

/*
 * This surface is already a dependency of the surface it is being made a
 * dependency of.
 */
#define DDERR_SURFACEALREADYDEPENDENT           MAKE_DDHRESULT( 420 )

/*
 * Access to this surface is being refused because the surface is already
 * locked by another thread.
 */
#define DDERR_SURFACEBUSY                       MAKE_DDHRESULT( 430 )

/*
 * Access to this surface is being refused because no driver exists
 * which can supply a pointer to the surface.
 * This is most likely to happen when attempting to lock the primary
 * surface when no DCI provider is present.
 * Will also happen on attempts to lock an optimized surface.
 */
#define DDERR_CANTLOCKSURFACE                   MAKE_DDHRESULT( 435 )

/*
 * Access to Surface refused because Surface is obscured.
 */
#define DDERR_SURFACEISOBSCURED                 MAKE_DDHRESULT( 440 )

/*
 * Access to this surface is being refused because the surface is gone.
 * The DIRECTDRAWSURFACE object representing this surface should
 * have Restore called on it.
 */
#define DDERR_SURFACELOST                       MAKE_DDHRESULT( 450 )

/*
 * The requested surface is not attached.
 */
#define DDERR_SURFACENOTATTACHED                MAKE_DDHRESULT( 460 )

/*
 * Height requested by DirectDraw is too large.
 */
#define DDERR_TOOBIGHEIGHT                      MAKE_DDHRESULT( 470 )

/*
 * Size requested by DirectDraw is too large --  The individual height and
 * width are OK.
 */
#define DDERR_TOOBIGSIZE                        MAKE_DDHRESULT( 480 )

/*
 * Width requested by DirectDraw is too large.
 */
#define DDERR_TOOBIGWIDTH                       MAKE_DDHRESULT( 490 )

/*
 * Action not supported.
 */
#define DDERR_UNSUPPORTED                       E_NOTIMPL

/*
 * Pixel format requested is unsupported by DirectDraw
 */
#define DDERR_UNSUPPORTEDFORMAT                 MAKE_DDHRESULT( 510 )

/*
 * Bitmask in the pixel format requested is unsupported by DirectDraw
 */
#define DDERR_UNSUPPORTEDMASK                   MAKE_DDHRESULT( 520 )

/*
 * The specified stream contains invalid data
 */
#define DDERR_INVALIDSTREAM                     MAKE_DDHRESULT( 521 )

/*
 * vertical blank is in progress
 */
#define DDERR_VERTICALBLANKINPROGRESS           MAKE_DDHRESULT( 537 )

/*
 * Informs DirectDraw that the previous Blt which is transfering information
 * to or from this Surface is incomplete.
 */
#define DDERR_WASSTILLDRAWING                   MAKE_DDHRESULT( 540 )


/*
 * The specified surface type requires specification of the COMPLEX flag
 */
#define DDERR_DDSCAPSCOMPLEXREQUIRED            MAKE_DDHRESULT( 542 )


/*
 * Rectangle provided was not horizontally aligned on reqd. boundary
 */
#define DDERR_XALIGN                            MAKE_DDHRESULT( 560 )

/*
 * The GUID passed to DirectDrawCreate is not a valid DirectDraw driver
 * identifier.
 */
#define DDERR_INVALIDDIRECTDRAWGUID             MAKE_DDHRESULT( 561 )

/*
 * A DirectDraw object representing this driver has already been created
 * for this process.
 */
#define DDERR_DIRECTDRAWALREADYCREATED          MAKE_DDHRESULT( 562 )

/*
 * A hardware only DirectDraw object creation was attempted but the driver
 * did not support any hardware.
 */
#define DDERR_NODIRECTDRAWHW                    MAKE_DDHRESULT( 563 )

/*
 * this process already has created a primary surface
 */
#define DDERR_PRIMARYSURFACEALREADYEXISTS       MAKE_DDHRESULT( 564 )

/*
 * software emulation not available.
 */
#define DDERR_NOEMULATION                       MAKE_DDHRESULT( 565 )

/*
 * region passed to Clipper::GetClipList is too small.
 */
#define DDERR_REGIONTOOSMALL                    MAKE_DDHRESULT( 566 )

/*
 * an attempt was made to set a clip list for a clipper objec that
 * is already monitoring an hwnd.
 */
#define DDERR_CLIPPERISUSINGHWND                MAKE_DDHRESULT( 567 )

/*
 * No clipper object attached to surface object
 */
#define DDERR_NOCLIPPERATTACHED                 MAKE_DDHRESULT( 568 )

/*
 * Clipper notification requires an HWND or
 * no HWND has previously been set as the CooperativeLevel HWND.
 */
#define DDERR_NOHWND                            MAKE_DDHRESULT( 569 )

/*
 * HWND used by DirectDraw CooperativeLevel has been subclassed,
 * this prevents DirectDraw from restoring state.
 */
#define DDERR_HWNDSUBCLASSED                    MAKE_DDHRESULT( 570 )

/*
 * The CooperativeLevel HWND has already been set.
 * It can not be reset while the process has surfaces or palettes created.
 */
#define DDERR_HWNDALREADYSET                    MAKE_DDHRESULT( 571 )

/*
 * No palette object attached to this surface.
 */
#define DDERR_NOPALETTEATTACHED                 MAKE_DDHRESULT( 572 )

/*
 * No hardware support for 16 or 256 color palettes.
 */
#define DDERR_NOPALETTEHW                       MAKE_DDHRESULT( 573 )

/*
 * If a clipper object is attached to the source surface passed into a
 * BltFast call.
 */
#define DDERR_BLTFASTCANTCLIP                   MAKE_DDHRESULT( 574 )

/*
 * No blter.
 */
#define DDERR_NOBLTHW                           MAKE_DDHRESULT( 575 )

/*
 * No DirectDraw ROP hardware.
 */
#define DDERR_NODDROPSHW                        MAKE_DDHRESULT( 576 )

/*
 * returned when GetOverlayPosition is called on a hidden overlay
 */
#define DDERR_OVERLAYNOTVISIBLE                 MAKE_DDHRESULT( 577 )

/*
 * returned when GetOverlayPosition is called on a overlay that UpdateOverlay
 * has never been called on to establish a destionation.
 */
#define DDERR_NOOVERLAYDEST                     MAKE_DDHRESULT( 578 )

/*
 * returned when the position of the overlay on the destionation is no longer
 * legal for that destionation.
 */
#define DDERR_INVALIDPOSITION                   MAKE_DDHRESULT( 579 )

/*
 * returned when an overlay member is called for a non-overlay surface
 */
#define DDERR_NOTAOVERLAYSURFACE                MAKE_DDHRESULT( 580 )

/*
 * An attempt was made to set the cooperative level when it was already
 * set to exclusive.
 */
#define DDERR_EXCLUSIVEMODEALREADYSET           MAKE_DDHRESULT( 581 )

/*
 * An attempt has been made to flip a surface that is not flippable.
 */
#define DDERR_NOTFLIPPABLE                      MAKE_DDHRESULT( 582 )

/*
 * Can't duplicate primary & 3D surfaces, or surfaces that are implicitly
 * created.
 */
#define DDERR_CANTDUPLICATE                     MAKE_DDHRESULT( 583 )

/*
 * Surface was not locked.  An attempt to unlock a surface that was not
 * locked at all, or by this process, has been attempted.
 */
#define DDERR_NOTLOCKED                         MAKE_DDHRESULT( 584 )

/*
 * Windows can not create any more DCs, or a DC was requested for a paltte-indexed
 * surface when the surface had no palette AND the display mode was not palette-indexed
 * (in this case DirectDraw cannot select a proper palette into the DC)
 */
#define DDERR_CANTCREATEDC                      MAKE_DDHRESULT( 585 )

/*
 * No DC was ever created for this surface.
 */
#define DDERR_NODC                              MAKE_DDHRESULT( 586 )

/*
 * This surface can not be restored because it was created in a different
 * mode.
 */
#define DDERR_WRONGMODE                         MAKE_DDHRESULT( 587 )

/*
 * This surface can not be restored because it is an implicitly created
 * surface.
 */
#define DDERR_IMPLICITLYCREATED                 MAKE_DDHRESULT( 588 )

/*
 * The surface being used is not a palette-based surface
 */
#define DDERR_NOTPALETTIZED                     MAKE_DDHRESULT( 589 )


/*
 * The display is currently in an unsupported mode
 */
#define DDERR_UNSUPPORTEDMODE                   MAKE_DDHRESULT( 590 )

/*
 * Operation could not be carried out because there is no mip-map
 * texture mapping hardware present or available.
 */
#define DDERR_NOMIPMAPHW                        MAKE_DDHRESULT( 591 )

/*
 * The requested action could not be performed because the surface was of
 * the wrong type.
 */
#define DDERR_INVALIDSURFACETYPE                MAKE_DDHRESULT( 592 )


/*
 * Device does not support optimized surfaces, therefore no video memory optimized surfaces
 */
#define DDERR_NOOPTIMIZEHW                      MAKE_DDHRESULT( 600 )

/*
 * Surface is an optimized surface, but has not yet been allocated any memory
 */
#define DDERR_NOTLOADED                         MAKE_DDHRESULT( 601 )

/*
 * Attempt was made to create or set a device window without first setting
 * the focus window
 */
#define DDERR_NOFOCUSWINDOW                     MAKE_DDHRESULT( 602 )

/*
 * Attempt was made to set a palette on a mipmap sublevel
 */
#define DDERR_NOTONMIPMAPSUBLEVEL               MAKE_DDHRESULT( 603 )

/*
 * A DC has already been returned for this surface. Only one DC can be
 * retrieved per surface.
 */
#define DDERR_DCALREADYCREATED                  MAKE_DDHRESULT( 620 )

/*
 * An attempt was made to allocate non-local video memory from a device
 * that does not support non-local video memory.
 */
#define DDERR_NONONLOCALVIDMEM                  MAKE_DDHRESULT( 630 )

/*
 * The attempt to page lock a surface failed.
 */
#define DDERR_CANTPAGELOCK                      MAKE_DDHRESULT( 640 )


/*
 * The attempt to page unlock a surface failed.
 */
#define DDERR_CANTPAGEUNLOCK                    MAKE_DDHRESULT( 660 )

/*
 * An attempt was made to page unlock a surface with no outstanding page locks.
 */
#define DDERR_NOTPAGELOCKED                     MAKE_DDHRESULT( 680 )

/*
 * There is more data available than the specified buffer size could hold
 */
#define DDERR_MOREDATA                          MAKE_DDHRESULT( 690 )

/*
 * The data has expired and is therefore no longer valid.
 */
#define DDERR_EXPIRED                           MAKE_DDHRESULT( 691 )

/*
 * The mode test has finished executing.
 */
#define DDERR_TESTFINISHED                      MAKE_DDHRESULT( 692 )

/*
 * The mode test has switched to a new mode.
 */
#define DDERR_NEWMODE                           MAKE_DDHRESULT( 693 )

/*
 * D3D has not yet been initialized.
 */
#define DDERR_D3DNOTINITIALIZED                 MAKE_DDHRESULT( 694 )

/*
 * The video port is not active
 */
#define DDERR_VIDEONOTACTIVE                    MAKE_DDHRESULT( 695 )

/*
 * The monitor does not have EDID data.
 */
#define DDERR_NOMONITORINFORMATION              MAKE_DDHRESULT( 696 )

/*
 * The driver does not enumerate display mode refresh rates.
 */
#define DDERR_NODRIVERSUPPORT                   MAKE_DDHRESULT( 697 )

/*
 * Surfaces created by one direct draw device cannot be used directly by
 * another direct draw device.
 */
#define DDERR_DEVICEDOESNTOWNSURFACE            MAKE_DDHRESULT( 699 )



/*
 * An attempt was made to invoke an interface member of a DirectDraw object
 * created by CoCreateInstance() before it was initialized.
 */
#define DDERR_NOTINITIALIZED                    CO_E_NOTINITIALIZED


/* Alpha bit depth constants */


#ifdef __cplusplus
};
#endif

#ifdef ENABLE_NAMELESS_UNION_PRAGMA
#pragma warning(default:4201)
#endif

#endif //__DDRAW_INCLUDED__


