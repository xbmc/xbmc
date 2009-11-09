/*==========================================================================;
 *
 *  Copyright (c) Microsoft Corporation.  All rights reserved.
 *
 *  File:       d3drm.h
 *  Content:    Direct3DRM include file
 *
 ***************************************************************************/

#ifndef _D3DRMOBJ_H_
#define _D3DRMOBJ_H_

#include <objbase.h> /* Use Windows header files */
#define VIRTUAL
#include "d3drmdef.h"

#include "d3d.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The methods for IUnknown
 */
#define IUNKNOWN_METHODS(kind) \
    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, LPVOID *ppvObj) kind; \
    STDMETHOD_(ULONG, AddRef)           (THIS) kind; \
    STDMETHOD_(ULONG, Release)          (THIS) kind

/*
 * The methods for IDirect3DRMObject
 */
#define IDIRECT3DRMOBJECT_METHODS(kind) \
    STDMETHOD(Clone)                    (THIS_ LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObj) kind; \
    STDMETHOD(AddDestroyCallback)       (THIS_ D3DRMOBJECTCALLBACK, LPVOID argument) kind; \
    STDMETHOD(DeleteDestroyCallback)    (THIS_ D3DRMOBJECTCALLBACK, LPVOID argument) kind; \
    STDMETHOD(SetAppData)               (THIS_ DWORD data) kind; \
    STDMETHOD_(DWORD, GetAppData)       (THIS) kind; \
    STDMETHOD(SetName)                  (THIS_ LPCSTR) kind; \
    STDMETHOD(GetName)                  (THIS_ LPDWORD lpdwSize, LPSTR lpName) kind; \
    STDMETHOD(GetClassName)             (THIS_ LPDWORD lpdwSize, LPSTR lpName) kind


#define WIN_TYPES(itype, ptype) \
    typedef interface itype FAR *LP##ptype, FAR **LPLP##ptype

WIN_TYPES(IDirect3DRMObject, DIRECT3DRMOBJECT);
WIN_TYPES(IDirect3DRMObject2, DIRECT3DRMOBJECT2);
WIN_TYPES(IDirect3DRMDevice, DIRECT3DRMDEVICE);
WIN_TYPES(IDirect3DRMDevice2, DIRECT3DRMDEVICE2);
WIN_TYPES(IDirect3DRMDevice3, DIRECT3DRMDEVICE3);
WIN_TYPES(IDirect3DRMViewport, DIRECT3DRMVIEWPORT);
WIN_TYPES(IDirect3DRMViewport2, DIRECT3DRMVIEWPORT2);
WIN_TYPES(IDirect3DRMFrame, DIRECT3DRMFRAME);
WIN_TYPES(IDirect3DRMFrame2, DIRECT3DRMFRAME2);
WIN_TYPES(IDirect3DRMFrame3, DIRECT3DRMFRAME3);
WIN_TYPES(IDirect3DRMVisual, DIRECT3DRMVISUAL);
WIN_TYPES(IDirect3DRMMesh, DIRECT3DRMMESH);
WIN_TYPES(IDirect3DRMMeshBuilder, DIRECT3DRMMESHBUILDER);
WIN_TYPES(IDirect3DRMMeshBuilder2, DIRECT3DRMMESHBUILDER2);
WIN_TYPES(IDirect3DRMMeshBuilder3, DIRECT3DRMMESHBUILDER3);
WIN_TYPES(IDirect3DRMFace, DIRECT3DRMFACE);
WIN_TYPES(IDirect3DRMFace2, DIRECT3DRMFACE2);
WIN_TYPES(IDirect3DRMLight, DIRECT3DRMLIGHT);
WIN_TYPES(IDirect3DRMTexture, DIRECT3DRMTEXTURE);
WIN_TYPES(IDirect3DRMTexture2, DIRECT3DRMTEXTURE2);
WIN_TYPES(IDirect3DRMTexture3, DIRECT3DRMTEXTURE3);
WIN_TYPES(IDirect3DRMWrap, DIRECT3DRMWRAP);
WIN_TYPES(IDirect3DRMMaterial, DIRECT3DRMMATERIAL);
WIN_TYPES(IDirect3DRMMaterial2, DIRECT3DRMMATERIAL2);
WIN_TYPES(IDirect3DRMInterpolator, DIRECT3DRMINTERPOLATOR);
WIN_TYPES(IDirect3DRMAnimation, DIRECT3DRMANIMATION);
WIN_TYPES(IDirect3DRMAnimation2, DIRECT3DRMANIMATION2);
WIN_TYPES(IDirect3DRMAnimationSet, DIRECT3DRMANIMATIONSET);
WIN_TYPES(IDirect3DRMAnimationSet2, DIRECT3DRMANIMATIONSET2);
WIN_TYPES(IDirect3DRMUserVisual, DIRECT3DRMUSERVISUAL);
WIN_TYPES(IDirect3DRMShadow, DIRECT3DRMSHADOW);
WIN_TYPES(IDirect3DRMShadow2, DIRECT3DRMSHADOW2);
WIN_TYPES(IDirect3DRMArray, DIRECT3DRMARRAY);
WIN_TYPES(IDirect3DRMObjectArray, DIRECT3DRMOBJECTARRAY);
WIN_TYPES(IDirect3DRMDeviceArray, DIRECT3DRMDEVICEARRAY);
WIN_TYPES(IDirect3DRMFaceArray, DIRECT3DRMFACEARRAY);
WIN_TYPES(IDirect3DRMViewportArray, DIRECT3DRMVIEWPORTARRAY);
WIN_TYPES(IDirect3DRMFrameArray, DIRECT3DRMFRAMEARRAY);
WIN_TYPES(IDirect3DRMAnimationArray, DIRECT3DRMANIMATIONARRAY);
WIN_TYPES(IDirect3DRMVisualArray, DIRECT3DRMVISUALARRAY);
WIN_TYPES(IDirect3DRMPickedArray, DIRECT3DRMPICKEDARRAY);
WIN_TYPES(IDirect3DRMPicked2Array, DIRECT3DRMPICKED2ARRAY);
WIN_TYPES(IDirect3DRMLightArray, DIRECT3DRMLIGHTARRAY);
WIN_TYPES(IDirect3DRMProgressiveMesh, DIRECT3DRMPROGRESSIVEMESH);
WIN_TYPES(IDirect3DRMClippedVisual, DIRECT3DRMCLIPPEDVISUAL);

/*
 * Direct3DRM Object classes
 */
DEFINE_GUID(CLSID_CDirect3DRMDevice,        0x4fa3568e, 0x623f, 0x11cf, 0xac, 0x4a, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(CLSID_CDirect3DRMViewport,      0x4fa3568f, 0x623f, 0x11cf, 0xac, 0x4a, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(CLSID_CDirect3DRMFrame,         0x4fa35690, 0x623f, 0x11cf, 0xac, 0x4a, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(CLSID_CDirect3DRMMesh,          0x4fa35691, 0x623f, 0x11cf, 0xac, 0x4a, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(CLSID_CDirect3DRMMeshBuilder,   0x4fa35692, 0x623f, 0x11cf, 0xac, 0x4a, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(CLSID_CDirect3DRMFace,          0x4fa35693, 0x623f, 0x11cf, 0xac, 0x4a, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(CLSID_CDirect3DRMLight,         0x4fa35694, 0x623f, 0x11cf, 0xac, 0x4a, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(CLSID_CDirect3DRMTexture,       0x4fa35695, 0x623f, 0x11cf, 0xac, 0x4a, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(CLSID_CDirect3DRMWrap,          0x4fa35696, 0x623f, 0x11cf, 0xac, 0x4a, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(CLSID_CDirect3DRMMaterial,      0x4fa35697, 0x623f, 0x11cf, 0xac, 0x4a, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(CLSID_CDirect3DRMAnimation,     0x4fa35698, 0x623f, 0x11cf, 0xac, 0x4a, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(CLSID_CDirect3DRMAnimationSet,  0x4fa35699, 0x623f, 0x11cf, 0xac, 0x4a, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(CLSID_CDirect3DRMUserVisual,    0x4fa3569a, 0x623f, 0x11cf, 0xac, 0x4a, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(CLSID_CDirect3DRMShadow,        0x4fa3569b, 0x623f, 0x11cf, 0xac, 0x4a, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(CLSID_CDirect3DRMViewportInterpolator,
0xde9eaa1, 0x3b84, 0x11d0, 0x9b, 0x6d, 0x0, 0x0, 0xc0, 0x78, 0x1b, 0xc3);
DEFINE_GUID(CLSID_CDirect3DRMFrameInterpolator,
0xde9eaa2, 0x3b84, 0x11d0, 0x9b, 0x6d, 0x0, 0x0, 0xc0, 0x78, 0x1b, 0xc3);
DEFINE_GUID(CLSID_CDirect3DRMMeshInterpolator,
0xde9eaa3, 0x3b84, 0x11d0, 0x9b, 0x6d, 0x0, 0x0, 0xc0, 0x78, 0x1b, 0xc3);
DEFINE_GUID(CLSID_CDirect3DRMLightInterpolator,
0xde9eaa6, 0x3b84, 0x11d0, 0x9b, 0x6d, 0x0, 0x0, 0xc0, 0x78, 0x1b, 0xc3);
DEFINE_GUID(CLSID_CDirect3DRMMaterialInterpolator,
0xde9eaa7, 0x3b84, 0x11d0, 0x9b, 0x6d, 0x0, 0x0, 0xc0, 0x78, 0x1b, 0xc3);
DEFINE_GUID(CLSID_CDirect3DRMTextureInterpolator,
0xde9eaa8, 0x3b84, 0x11d0, 0x9b, 0x6d, 0x0, 0x0, 0xc0, 0x78, 0x1b, 0xc3);
DEFINE_GUID(CLSID_CDirect3DRMProgressiveMesh, 0x4516ec40, 0x8f20, 0x11d0, 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3);
DEFINE_GUID(CLSID_CDirect3DRMClippedVisual,   0x5434e72d, 0x6d66, 0x11d1, 0xbb, 0xb, 0x0, 0x0, 0xf8, 0x75, 0x86, 0x5a);


/*
 * Direct3DRM Object interfaces
 */
DEFINE_GUID(IID_IDirect3DRMObject,          0xeb16cb00, 0xd271, 0x11ce, 0xac, 0x48, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMObject2,         0x4516ec7c, 0x8f20, 0x11d0, 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3);
DEFINE_GUID(IID_IDirect3DRMDevice,          0xe9e19280, 0x6e05, 0x11cf, 0xac, 0x4a, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMDevice2,         0x4516ec78, 0x8f20, 0x11d0, 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3);
DEFINE_GUID(IID_IDirect3DRMDevice3,     0x549f498b, 0xbfeb, 0x11d1, 0x8e, 0xd8, 0x0, 0xa0, 0xc9, 0x67, 0xa4, 0x82);
DEFINE_GUID(IID_IDirect3DRMViewport,        0xeb16cb02, 0xd271, 0x11ce, 0xac, 0x48, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMViewport2,   0x4a1b1be6, 0xbfed, 0x11d1, 0x8e, 0xd8, 0x0, 0xa0, 0xc9, 0x67, 0xa4, 0x82);
DEFINE_GUID(IID_IDirect3DRMFrame,           0xeb16cb03, 0xd271, 0x11ce, 0xac, 0x48, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMFrame2,          0xc3dfbd60, 0x3988, 0x11d0, 0x9e, 0xc2, 0x0, 0x0, 0xc0, 0x29, 0x1a, 0xc3);
DEFINE_GUID(IID_IDirect3DRMFrame3,              0xff6b7f70, 0xa40e, 0x11d1, 0x91, 0xf9, 0x0, 0x0, 0xf8, 0x75, 0x8e, 0x66);
DEFINE_GUID(IID_IDirect3DRMVisual,          0xeb16cb04, 0xd271, 0x11ce, 0xac, 0x48, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMMesh,            0xa3a80d01, 0x6e12, 0x11cf, 0xac, 0x4a, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMMeshBuilder,     0xa3a80d02, 0x6e12, 0x11cf, 0xac, 0x4a, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMMeshBuilder2,    0x4516ec77, 0x8f20, 0x11d0, 0x9b, 0x6d, 0x0, 0x0, 0xc0, 0x78, 0x1b, 0xc3);
DEFINE_GUID(IID_IDirect3DRMMeshBuilder3,    0x4516ec82, 0x8f20, 0x11d0, 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3);
DEFINE_GUID(IID_IDirect3DRMFace,            0xeb16cb07, 0xd271, 0x11ce, 0xac, 0x48, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMFace2,           0x4516ec81, 0x8f20, 0x11d0, 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3);
DEFINE_GUID(IID_IDirect3DRMLight,           0xeb16cb08, 0xd271, 0x11ce, 0xac, 0x48, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMTexture,         0xeb16cb09, 0xd271, 0x11ce, 0xac, 0x48, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMTexture2,        0x120f30c0, 0x1629, 0x11d0, 0x94, 0x1c, 0x0, 0x80, 0xc8, 0xc, 0xfa, 0x7b);
DEFINE_GUID(IID_IDirect3DRMTexture3,        0xff6b7f73, 0xa40e, 0x11d1, 0x91, 0xf9, 0x0, 0x0, 0xf8, 0x75, 0x8e, 0x66);
DEFINE_GUID(IID_IDirect3DRMWrap,            0xeb16cb0a, 0xd271, 0x11ce, 0xac, 0x48, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMMaterial,        0xeb16cb0b, 0xd271, 0x11ce, 0xac, 0x48, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMMaterial2,       0xff6b7f75, 0xa40e, 0x11d1, 0x91, 0xf9, 0x0, 0x0, 0xf8, 0x75, 0x8e, 0x66);
DEFINE_GUID(IID_IDirect3DRMAnimation,       0xeb16cb0d, 0xd271, 0x11ce, 0xac, 0x48, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMAnimation2,      0xff6b7f77, 0xa40e, 0x11d1, 0x91, 0xf9, 0x0, 0x0, 0xf8, 0x75, 0x8e, 0x66);
DEFINE_GUID(IID_IDirect3DRMAnimationSet,    0xeb16cb0e, 0xd271, 0x11ce, 0xac, 0x48, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMAnimationSet2,   0xff6b7f79, 0xa40e, 0x11d1, 0x91, 0xf9, 0x0, 0x0, 0xf8, 0x75, 0x8e, 0x66);
DEFINE_GUID(IID_IDirect3DRMObjectArray,     0x242f6bc2, 0x3849, 0x11d0, 0x9b, 0x6d, 0x0, 0x0, 0xc0, 0x78, 0x1b, 0xc3);
DEFINE_GUID(IID_IDirect3DRMDeviceArray,     0xeb16cb10, 0xd271, 0x11ce, 0xac, 0x48, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMViewportArray,   0xeb16cb11, 0xd271, 0x11ce, 0xac, 0x48, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMFrameArray,      0xeb16cb12, 0xd271, 0x11ce, 0xac, 0x48, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMVisualArray,     0xeb16cb13, 0xd271, 0x11ce, 0xac, 0x48, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMLightArray,      0xeb16cb14, 0xd271, 0x11ce, 0xac, 0x48, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMPickedArray,     0xeb16cb16, 0xd271, 0x11ce, 0xac, 0x48, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMFaceArray,       0xeb16cb17, 0xd271, 0x11ce, 0xac, 0x48, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMAnimationArray,
0xd5f1cae0, 0x4bd7, 0x11d1, 0xb9, 0x74, 0x0, 0x60, 0x8, 0x3e, 0x45, 0xf3);
DEFINE_GUID(IID_IDirect3DRMUserVisual,      0x59163de0, 0x6d43, 0x11cf, 0xac, 0x4a, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMShadow,          0xaf359780, 0x6ba3, 0x11cf, 0xac, 0x4a, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMShadow2,         0x86b44e25, 0x9c82, 0x11d1, 0xbb, 0xb, 0x0, 0xa0, 0xc9, 0x81, 0xa0, 0xa6);
DEFINE_GUID(IID_IDirect3DRMInterpolator,    0x242f6bc1, 0x3849, 0x11d0, 0x9b, 0x6d, 0x0, 0x0, 0xc0, 0x78, 0x1b, 0xc3);
DEFINE_GUID(IID_IDirect3DRMProgressiveMesh, 0x4516ec79, 0x8f20, 0x11d0, 0x9b, 0x6d, 0x0, 0x0, 0xc0, 0x78, 0x1b, 0xc3);
DEFINE_GUID(IID_IDirect3DRMPicked2Array,    0x4516ec7b, 0x8f20, 0x11d0, 0x9b, 0x6d, 0x0, 0x0, 0xc0, 0x78, 0x1b, 0xc3);
DEFINE_GUID(IID_IDirect3DRMClippedVisual,   0x5434e733, 0x6d66, 0x11d1, 0xbb, 0xb, 0x0, 0x0, 0xf8, 0x75, 0x86, 0x5a);

typedef void (__cdecl *D3DRMOBJECTCALLBACK)(LPDIRECT3DRMOBJECT obj, LPVOID arg);
typedef void (__cdecl *D3DRMFRAMEMOVECALLBACK)(LPDIRECT3DRMFRAME obj, LPVOID arg, D3DVALUE delta);
typedef void (__cdecl *D3DRMFRAME3MOVECALLBACK)(LPDIRECT3DRMFRAME3 obj, LPVOID arg, D3DVALUE delta);
typedef void (__cdecl *D3DRMUPDATECALLBACK)(LPDIRECT3DRMDEVICE obj, LPVOID arg, int, LPD3DRECT);
typedef void (__cdecl *D3DRMDEVICE3UPDATECALLBACK)(LPDIRECT3DRMDEVICE3 obj, LPVOID arg, int, LPD3DRECT);
typedef int (__cdecl *D3DRMUSERVISUALCALLBACK)
    (   LPDIRECT3DRMUSERVISUAL obj, LPVOID arg, D3DRMUSERVISUALREASON reason,
        LPDIRECT3DRMDEVICE dev, LPDIRECT3DRMVIEWPORT view
    );
typedef HRESULT (__cdecl *D3DRMLOADTEXTURECALLBACK)
    (char *tex_name, void *arg, LPDIRECT3DRMTEXTURE *);
typedef HRESULT (__cdecl *D3DRMLOADTEXTURE3CALLBACK)
    (char *tex_name, void *arg, LPDIRECT3DRMTEXTURE3 *);
typedef void (__cdecl *D3DRMLOADCALLBACK)
    (LPDIRECT3DRMOBJECT object, REFIID objectguid, LPVOID arg);

typedef HRESULT (__cdecl *D3DRMDOWNSAMPLECALLBACK)
    (LPDIRECT3DRMTEXTURE3 lpDirect3DRMTexture, LPVOID pArg,
     LPDIRECTDRAWSURFACE pDDSSrc, LPDIRECTDRAWSURFACE pDDSDst);
typedef HRESULT (__cdecl *D3DRMVALIDATIONCALLBACK)
    (LPDIRECT3DRMTEXTURE3 lpDirect3DRMTexture, LPVOID pArg,
     DWORD dwFlags, DWORD dwcRects, LPRECT pRects);


typedef struct _D3DRMPICKDESC
{
    ULONG       ulFaceIdx;
    LONG        lGroupIdx;
    D3DVECTOR   vPosition;

} D3DRMPICKDESC, *LPD3DRMPICKDESC;

typedef struct _D3DRMPICKDESC2
{
    ULONG       ulFaceIdx;
    LONG        lGroupIdx;
    D3DVECTOR   dvPosition;
    D3DVALUE    tu;
    D3DVALUE    tv;
    D3DVECTOR   dvNormal;
    D3DCOLOR    dcColor;

} D3DRMPICKDESC2, *LPD3DRMPICKDESC2;

#undef INTERFACE
#define INTERFACE IDirect3DRMObject

/*
 * Base class
 */
DECLARE_INTERFACE_(IDirect3DRMObject, IUnknown)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);
};

#undef INTERFACE
#define INTERFACE IDirect3DRMObject2

DECLARE_INTERFACE_(IDirect3DRMObject2, IUnknown)
{
    IUNKNOWN_METHODS(PURE);

    /*
     * IDirect3DRMObject2 methods
     */
    STDMETHOD(AddDestroyCallback)(THIS_ D3DRMOBJECTCALLBACK lpFunc, LPVOID pvArg) PURE;
    STDMETHOD(Clone)(THIS_ LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObj) PURE; \
    STDMETHOD(DeleteDestroyCallback)(THIS_ D3DRMOBJECTCALLBACK lpFunc, LPVOID pvArg) PURE; \
    STDMETHOD(GetClientData)(THIS_ DWORD dwID, LPVOID* lplpvData) PURE;
    STDMETHOD(GetDirect3DRM)(THIS_ LPDIRECT3DRM* lplpDirect3DRM) PURE;
    STDMETHOD(GetName)(THIS_ LPDWORD lpdwSize, LPSTR lpName) PURE;
    STDMETHOD(SetClientData)(THIS_ DWORD dwID, LPVOID lpvData, DWORD dwFlags) PURE;
    STDMETHOD(SetName)(THIS_ LPCSTR lpName) PURE;
    STDMETHOD(GetAge)(THIS_ DWORD dwFlags, LPDWORD pdwAge) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMVisual

DECLARE_INTERFACE_(IDirect3DRMVisual, IDirect3DRMObject)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);
};

#undef INTERFACE
#define INTERFACE IDirect3DRMDevice

DECLARE_INTERFACE_(IDirect3DRMDevice, IDirect3DRMObject)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMDevice methods
     */
    STDMETHOD(Init)(THIS_ ULONG width, ULONG height) PURE;
    STDMETHOD(InitFromD3D)(THIS_ LPDIRECT3D lpD3D, LPDIRECT3DDEVICE lpD3DDev) PURE;
    STDMETHOD(InitFromClipper)(THIS_ LPDIRECTDRAWCLIPPER lpDDClipper, LPGUID lpGUID, int width, int height) PURE;

    STDMETHOD(Update)(THIS) PURE;
    STDMETHOD(AddUpdateCallback)(THIS_ D3DRMUPDATECALLBACK, LPVOID arg) PURE;
    STDMETHOD(DeleteUpdateCallback)(THIS_ D3DRMUPDATECALLBACK, LPVOID arg) PURE;
    STDMETHOD(SetBufferCount)(THIS_ DWORD) PURE;
    STDMETHOD_(DWORD, GetBufferCount)(THIS) PURE;

    STDMETHOD(SetDither)(THIS_ BOOL) PURE;
    STDMETHOD(SetShades)(THIS_ DWORD) PURE;
    STDMETHOD(SetQuality)(THIS_ D3DRMRENDERQUALITY) PURE;
    STDMETHOD(SetTextureQuality)(THIS_ D3DRMTEXTUREQUALITY) PURE;

    STDMETHOD(GetViewports)(THIS_ LPDIRECT3DRMVIEWPORTARRAY *return_views) PURE;

    STDMETHOD_(BOOL, GetDither)(THIS) PURE;
    STDMETHOD_(DWORD, GetShades)(THIS) PURE;
    STDMETHOD_(DWORD, GetHeight)(THIS) PURE;
    STDMETHOD_(DWORD, GetWidth)(THIS) PURE;
    STDMETHOD_(DWORD, GetTrianglesDrawn)(THIS) PURE;
    STDMETHOD_(DWORD, GetWireframeOptions)(THIS) PURE;
    STDMETHOD_(D3DRMRENDERQUALITY, GetQuality)(THIS) PURE;
    STDMETHOD_(D3DCOLORMODEL, GetColorModel)(THIS) PURE;
    STDMETHOD_(D3DRMTEXTUREQUALITY, GetTextureQuality)(THIS) PURE;
    STDMETHOD(GetDirect3DDevice)(THIS_ LPDIRECT3DDEVICE *) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMDevice2

DECLARE_INTERFACE_(IDirect3DRMDevice2, IDirect3DRMDevice)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMDevice methods
     */
    STDMETHOD(Init)(THIS_ ULONG width, ULONG height) PURE;
    STDMETHOD(InitFromD3D)(THIS_ LPDIRECT3D lpD3D, LPDIRECT3DDEVICE lpD3DDev) PURE;
    STDMETHOD(InitFromClipper)(THIS_ LPDIRECTDRAWCLIPPER lpDDClipper, LPGUID lpGUID, int width, int height) PURE;

    STDMETHOD(Update)(THIS) PURE;
    STDMETHOD(AddUpdateCallback)(THIS_ D3DRMUPDATECALLBACK, LPVOID arg) PURE;
    STDMETHOD(DeleteUpdateCallback)(THIS_ D3DRMUPDATECALLBACK, LPVOID arg) PURE;
    STDMETHOD(SetBufferCount)(THIS_ DWORD) PURE;
    STDMETHOD_(DWORD, GetBufferCount)(THIS) PURE;

    STDMETHOD(SetDither)(THIS_ BOOL) PURE;
    STDMETHOD(SetShades)(THIS_ DWORD) PURE;
    STDMETHOD(SetQuality)(THIS_ D3DRMRENDERQUALITY) PURE;
    STDMETHOD(SetTextureQuality)(THIS_ D3DRMTEXTUREQUALITY) PURE;

    STDMETHOD(GetViewports)(THIS_ LPDIRECT3DRMVIEWPORTARRAY *return_views) PURE;

    STDMETHOD_(BOOL, GetDither)(THIS) PURE;
    STDMETHOD_(DWORD, GetShades)(THIS) PURE;
    STDMETHOD_(DWORD, GetHeight)(THIS) PURE;
    STDMETHOD_(DWORD, GetWidth)(THIS) PURE;
    STDMETHOD_(DWORD, GetTrianglesDrawn)(THIS) PURE;
    STDMETHOD_(DWORD, GetWireframeOptions)(THIS) PURE;
    STDMETHOD_(D3DRMRENDERQUALITY, GetQuality)(THIS) PURE;
    STDMETHOD_(D3DCOLORMODEL, GetColorModel)(THIS) PURE;
    STDMETHOD_(D3DRMTEXTUREQUALITY, GetTextureQuality)(THIS) PURE;
    STDMETHOD(GetDirect3DDevice)(THIS_ LPDIRECT3DDEVICE *) PURE;

    /*
     * IDirect3DRMDevice2 methods
     */
    STDMETHOD(InitFromD3D2)(THIS_ LPDIRECT3D2 lpD3D, LPDIRECT3DDEVICE2 lpD3DDev) PURE;
    STDMETHOD(InitFromSurface)(THIS_ LPGUID lpGUID, LPDIRECTDRAW lpDD, LPDIRECTDRAWSURFACE lpDDSBack) PURE;
    STDMETHOD(SetRenderMode)(THIS_ DWORD dwFlags) PURE;
    STDMETHOD_(DWORD, GetRenderMode)(THIS) PURE;
    STDMETHOD(GetDirect3DDevice2)(THIS_ LPDIRECT3DDEVICE2 *) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMDevice3

DECLARE_INTERFACE_(IDirect3DRMDevice3, IDirect3DRMObject)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMDevice methods
     */
    STDMETHOD(Init)(THIS_ ULONG width, ULONG height) PURE;
    STDMETHOD(InitFromD3D)(THIS_ LPDIRECT3D lpD3D, LPDIRECT3DDEVICE lpD3DDev) PURE;
    STDMETHOD(InitFromClipper)(THIS_ LPDIRECTDRAWCLIPPER lpDDClipper, LPGUID lpGUID, int width, int height) PURE;

    STDMETHOD(Update)(THIS) PURE;
    STDMETHOD(AddUpdateCallback)(THIS_ D3DRMDEVICE3UPDATECALLBACK, LPVOID arg) PURE;
    STDMETHOD(DeleteUpdateCallback)(THIS_ D3DRMDEVICE3UPDATECALLBACK, LPVOID arg) PURE;
    STDMETHOD(SetBufferCount)(THIS_ DWORD) PURE;
    STDMETHOD_(DWORD, GetBufferCount)(THIS) PURE;

    STDMETHOD(SetDither)(THIS_ BOOL) PURE;
    STDMETHOD(SetShades)(THIS_ DWORD) PURE;
    STDMETHOD(SetQuality)(THIS_ D3DRMRENDERQUALITY) PURE;
    STDMETHOD(SetTextureQuality)(THIS_ D3DRMTEXTUREQUALITY) PURE;

    STDMETHOD(GetViewports)(THIS_ LPDIRECT3DRMVIEWPORTARRAY *return_views) PURE;

    STDMETHOD_(BOOL, GetDither)(THIS) PURE;
    STDMETHOD_(DWORD, GetShades)(THIS) PURE;
    STDMETHOD_(DWORD, GetHeight)(THIS) PURE;
    STDMETHOD_(DWORD, GetWidth)(THIS) PURE;
    STDMETHOD_(DWORD, GetTrianglesDrawn)(THIS) PURE;
    STDMETHOD_(DWORD, GetWireframeOptions)(THIS) PURE;
    STDMETHOD_(D3DRMRENDERQUALITY, GetQuality)(THIS) PURE;
    STDMETHOD_(D3DCOLORMODEL, GetColorModel)(THIS) PURE;
    STDMETHOD_(D3DRMTEXTUREQUALITY, GetTextureQuality)(THIS) PURE;
    STDMETHOD(GetDirect3DDevice)(THIS_ LPDIRECT3DDEVICE *) PURE;

    /*
     * IDirect3DRMDevice2 methods
     */
    STDMETHOD(InitFromD3D2)(THIS_ LPDIRECT3D2 lpD3D, LPDIRECT3DDEVICE2 lpD3DDev) PURE;
    STDMETHOD(InitFromSurface)(THIS_ LPGUID lpGUID, LPDIRECTDRAW lpDD, LPDIRECTDRAWSURFACE lpDDSBack, DWORD dwFlags) PURE;
    STDMETHOD(SetRenderMode)(THIS_ DWORD dwFlags) PURE;
    STDMETHOD_(DWORD, GetRenderMode)(THIS) PURE;
    STDMETHOD(GetDirect3DDevice2)(THIS_ LPDIRECT3DDEVICE2 *) PURE;

    /*
     * IDirect3DRMDevice3 methods
     */
    STDMETHOD(FindPreferredTextureFormat)(THIS_ DWORD dwBitDepths, DWORD dwFlags, LPDDPIXELFORMAT lpDDPF) PURE;
    STDMETHOD(RenderStateChange)(THIS_ D3DRENDERSTATETYPE drsType, DWORD dwVal, DWORD dwFlags) PURE;
    STDMETHOD(LightStateChange)(THIS_ D3DLIGHTSTATETYPE drsType, DWORD dwVal, DWORD dwFlags) PURE;
    STDMETHOD(GetStateChangeOptions)(THIS_ DWORD dwStateClass, DWORD dwStateNum, LPDWORD pdwFlags) PURE;
    STDMETHOD(SetStateChangeOptions)(THIS_ DWORD dwStateClass, DWORD dwStateNum, DWORD dwFlags) PURE;
};


#undef INTERFACE
#define INTERFACE IDirect3DRMViewport

DECLARE_INTERFACE_(IDirect3DRMViewport, IDirect3DRMObject)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMViewport methods
     */
    STDMETHOD(Init)
    (   THIS_ LPDIRECT3DRMDEVICE dev, LPDIRECT3DRMFRAME camera,
        DWORD xpos, DWORD ypos, DWORD width, DWORD height
    ) PURE;
    STDMETHOD(Clear)(THIS) PURE;
    STDMETHOD(Render)(THIS_ LPDIRECT3DRMFRAME) PURE;

    STDMETHOD(SetFront)(THIS_ D3DVALUE) PURE;
    STDMETHOD(SetBack)(THIS_ D3DVALUE) PURE;
    STDMETHOD(SetField)(THIS_ D3DVALUE) PURE;
    STDMETHOD(SetUniformScaling)(THIS_ BOOL) PURE;
    STDMETHOD(SetCamera)(THIS_ LPDIRECT3DRMFRAME) PURE;
    STDMETHOD(SetProjection)(THIS_ D3DRMPROJECTIONTYPE) PURE;
    STDMETHOD(Transform)(THIS_ D3DRMVECTOR4D *d, D3DVECTOR *s) PURE;
    STDMETHOD(InverseTransform)(THIS_ D3DVECTOR *d, D3DRMVECTOR4D *s) PURE;
    STDMETHOD(Configure)(THIS_ LONG x, LONG y, DWORD width, DWORD height) PURE;
    STDMETHOD(ForceUpdate)(THIS_ DWORD x1, DWORD y1, DWORD x2, DWORD y2) PURE;
    STDMETHOD(SetPlane)(THIS_ D3DVALUE left, D3DVALUE right, D3DVALUE bottom, D3DVALUE top) PURE;

    STDMETHOD(GetCamera)(THIS_ LPDIRECT3DRMFRAME *) PURE;
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DRMDEVICE *) PURE;
    STDMETHOD(GetPlane)(THIS_ D3DVALUE *left, D3DVALUE *right, D3DVALUE *bottom, D3DVALUE *top) PURE;
    STDMETHOD(Pick)(THIS_ LONG x, LONG y, LPDIRECT3DRMPICKEDARRAY *return_visuals) PURE;

    STDMETHOD_(BOOL, GetUniformScaling)(THIS) PURE;
    STDMETHOD_(LONG, GetX)(THIS) PURE;
    STDMETHOD_(LONG, GetY)(THIS) PURE;
    STDMETHOD_(DWORD, GetWidth)(THIS) PURE;
    STDMETHOD_(DWORD, GetHeight)(THIS) PURE;
    STDMETHOD_(D3DVALUE, GetField)(THIS) PURE;
    STDMETHOD_(D3DVALUE, GetBack)(THIS) PURE;
    STDMETHOD_(D3DVALUE, GetFront)(THIS) PURE;
    STDMETHOD_(D3DRMPROJECTIONTYPE, GetProjection)(THIS) PURE;
    STDMETHOD(GetDirect3DViewport)(THIS_ LPDIRECT3DVIEWPORT *) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMViewport2
DECLARE_INTERFACE_(IDirect3DRMViewport2, IDirect3DRMObject)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMViewport2 methods
     */
    STDMETHOD(Init)
    (   THIS_ LPDIRECT3DRMDEVICE3 dev, LPDIRECT3DRMFRAME3 camera,
        DWORD xpos, DWORD ypos, DWORD width, DWORD height
    ) PURE;
    STDMETHOD(Clear)(THIS_ DWORD dwFlags) PURE;
    STDMETHOD(Render)(THIS_ LPDIRECT3DRMFRAME3) PURE;

    STDMETHOD(SetFront)(THIS_ D3DVALUE) PURE;
    STDMETHOD(SetBack)(THIS_ D3DVALUE) PURE;
    STDMETHOD(SetField)(THIS_ D3DVALUE) PURE;
    STDMETHOD(SetUniformScaling)(THIS_ BOOL) PURE;
    STDMETHOD(SetCamera)(THIS_ LPDIRECT3DRMFRAME3) PURE;
    STDMETHOD(SetProjection)(THIS_ D3DRMPROJECTIONTYPE) PURE;
    STDMETHOD(Transform)(THIS_ D3DRMVECTOR4D *d, D3DVECTOR *s) PURE;
    STDMETHOD(InverseTransform)(THIS_ D3DVECTOR *d, D3DRMVECTOR4D *s) PURE;
    STDMETHOD(Configure)(THIS_ LONG x, LONG y, DWORD width, DWORD height) PURE;
    STDMETHOD(ForceUpdate)(THIS_ DWORD x1, DWORD y1, DWORD x2, DWORD y2) PURE;
    STDMETHOD(SetPlane)(THIS_ D3DVALUE left, D3DVALUE right, D3DVALUE bottom, D3DVALUE top) PURE;

    STDMETHOD(GetCamera)(THIS_ LPDIRECT3DRMFRAME3 *) PURE;
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DRMDEVICE3 *) PURE;
    STDMETHOD(GetPlane)(THIS_ D3DVALUE *left, D3DVALUE *right, D3DVALUE *bottom, D3DVALUE *top) PURE;
    STDMETHOD(Pick)(THIS_ LONG x, LONG y, LPDIRECT3DRMPICKEDARRAY *return_visuals) PURE;

    STDMETHOD_(BOOL, GetUniformScaling)(THIS) PURE;
    STDMETHOD_(LONG, GetX)(THIS) PURE;
    STDMETHOD_(LONG, GetY)(THIS) PURE;
    STDMETHOD_(DWORD, GetWidth)(THIS) PURE;
    STDMETHOD_(DWORD, GetHeight)(THIS) PURE;
    STDMETHOD_(D3DVALUE, GetField)(THIS) PURE;
    STDMETHOD_(D3DVALUE, GetBack)(THIS) PURE;
    STDMETHOD_(D3DVALUE, GetFront)(THIS) PURE;
    STDMETHOD_(D3DRMPROJECTIONTYPE, GetProjection)(THIS) PURE;
    STDMETHOD(GetDirect3DViewport)(THIS_ LPDIRECT3DVIEWPORT *) PURE;
    STDMETHOD(TransformVectors)(THIS_ DWORD dwNumVectors,
                                LPD3DRMVECTOR4D lpDstVectors,
                                LPD3DVECTOR lpSrcVectors) PURE;
    STDMETHOD(InverseTransformVectors)(THIS_ DWORD dwNumVectors,
                                       LPD3DVECTOR lpDstVectors,
                                       LPD3DRMVECTOR4D lpSrcVectors) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMFrame

DECLARE_INTERFACE_(IDirect3DRMFrame, IDirect3DRMVisual)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMFrame methods
     */
    STDMETHOD(AddChild)(THIS_ LPDIRECT3DRMFRAME child) PURE;
    STDMETHOD(AddLight)(THIS_ LPDIRECT3DRMLIGHT) PURE;
    STDMETHOD(AddMoveCallback)(THIS_ D3DRMFRAMEMOVECALLBACK, VOID *arg) PURE;
    STDMETHOD(AddTransform)(THIS_ D3DRMCOMBINETYPE, D3DRMMATRIX4D) PURE;
    STDMETHOD(AddTranslation)(THIS_ D3DRMCOMBINETYPE, D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD(AddScale)(THIS_ D3DRMCOMBINETYPE, D3DVALUE sx, D3DVALUE sy, D3DVALUE sz) PURE;
    STDMETHOD(AddRotation)(THIS_ D3DRMCOMBINETYPE, D3DVALUE x, D3DVALUE y, D3DVALUE z, D3DVALUE theta) PURE;
    STDMETHOD(AddVisual)(THIS_ LPDIRECT3DRMVISUAL) PURE;
    STDMETHOD(GetChildren)(THIS_ LPDIRECT3DRMFRAMEARRAY *children) PURE;
    STDMETHOD_(D3DCOLOR, GetColor)(THIS) PURE;
    STDMETHOD(GetLights)(THIS_ LPDIRECT3DRMLIGHTARRAY *lights) PURE;
    STDMETHOD_(D3DRMMATERIALMODE, GetMaterialMode)(THIS) PURE;
    STDMETHOD(GetParent)(THIS_ LPDIRECT3DRMFRAME *) PURE;
    STDMETHOD(GetPosition)(THIS_ LPDIRECT3DRMFRAME reference, LPD3DVECTOR return_position) PURE;
    STDMETHOD(GetRotation)(THIS_ LPDIRECT3DRMFRAME reference, LPD3DVECTOR axis, LPD3DVALUE return_theta) PURE;
    STDMETHOD(GetScene)(THIS_ LPDIRECT3DRMFRAME *) PURE;
    STDMETHOD_(D3DRMSORTMODE, GetSortMode)(THIS) PURE;
    STDMETHOD(GetTexture)(THIS_ LPDIRECT3DRMTEXTURE *) PURE;
    STDMETHOD(GetTransform)(THIS_ D3DRMMATRIX4D return_matrix) PURE;
    STDMETHOD(GetVelocity)(THIS_ LPDIRECT3DRMFRAME reference, LPD3DVECTOR return_velocity, BOOL with_rotation) PURE;
    STDMETHOD(GetOrientation)(THIS_ LPDIRECT3DRMFRAME reference, LPD3DVECTOR dir, LPD3DVECTOR up) PURE;
    STDMETHOD(GetVisuals)(THIS_ LPDIRECT3DRMVISUALARRAY *visuals) PURE;
    STDMETHOD(GetTextureTopology)(THIS_ BOOL *wrap_u, BOOL *wrap_v) PURE;
    STDMETHOD(InverseTransform)(THIS_ D3DVECTOR *d, D3DVECTOR *s) PURE;
    STDMETHOD(Load)(THIS_ LPVOID filename, LPVOID name, D3DRMLOADOPTIONS loadflags, D3DRMLOADTEXTURECALLBACK, LPVOID lpArg)PURE;
    STDMETHOD(LookAt)(THIS_ LPDIRECT3DRMFRAME target, LPDIRECT3DRMFRAME reference, D3DRMFRAMECONSTRAINT) PURE;
    STDMETHOD(Move)(THIS_ D3DVALUE delta) PURE;
    STDMETHOD(DeleteChild)(THIS_ LPDIRECT3DRMFRAME) PURE;
    STDMETHOD(DeleteLight)(THIS_ LPDIRECT3DRMLIGHT) PURE;
    STDMETHOD(DeleteMoveCallback)(THIS_ D3DRMFRAMEMOVECALLBACK, VOID *arg) PURE;
    STDMETHOD(DeleteVisual)(THIS_ LPDIRECT3DRMVISUAL) PURE;
    STDMETHOD_(D3DCOLOR, GetSceneBackground)(THIS) PURE;
    STDMETHOD(GetSceneBackgroundDepth)(THIS_ LPDIRECTDRAWSURFACE *) PURE;
    STDMETHOD_(D3DCOLOR, GetSceneFogColor)(THIS) PURE;
    STDMETHOD_(BOOL, GetSceneFogEnable)(THIS) PURE;
    STDMETHOD_(D3DRMFOGMODE, GetSceneFogMode)(THIS) PURE;
    STDMETHOD(GetSceneFogParams)(THIS_ D3DVALUE *return_start, D3DVALUE *return_end, D3DVALUE *return_density) PURE;
    STDMETHOD(SetSceneBackground)(THIS_ D3DCOLOR) PURE;
    STDMETHOD(SetSceneBackgroundRGB)(THIS_ D3DVALUE red, D3DVALUE green, D3DVALUE blue) PURE;
    STDMETHOD(SetSceneBackgroundDepth)(THIS_ LPDIRECTDRAWSURFACE) PURE;
    STDMETHOD(SetSceneBackgroundImage)(THIS_ LPDIRECT3DRMTEXTURE) PURE;
    STDMETHOD(SetSceneFogEnable)(THIS_ BOOL) PURE;
    STDMETHOD(SetSceneFogColor)(THIS_ D3DCOLOR) PURE;
    STDMETHOD(SetSceneFogMode)(THIS_ D3DRMFOGMODE) PURE;
    STDMETHOD(SetSceneFogParams)(THIS_ D3DVALUE start, D3DVALUE end, D3DVALUE density) PURE;
    STDMETHOD(SetColor)(THIS_ D3DCOLOR) PURE;
    STDMETHOD(SetColorRGB)(THIS_ D3DVALUE red, D3DVALUE green, D3DVALUE blue) PURE;
    STDMETHOD_(D3DRMZBUFFERMODE, GetZbufferMode)(THIS) PURE;
    STDMETHOD(SetMaterialMode)(THIS_ D3DRMMATERIALMODE) PURE;
    STDMETHOD(SetOrientation)
    (   THIS_ LPDIRECT3DRMFRAME reference,
        D3DVALUE dx, D3DVALUE dy, D3DVALUE dz,
        D3DVALUE ux, D3DVALUE uy, D3DVALUE uz
    ) PURE;
    STDMETHOD(SetPosition)(THIS_ LPDIRECT3DRMFRAME reference, D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD(SetRotation)(THIS_ LPDIRECT3DRMFRAME reference, D3DVALUE x, D3DVALUE y, D3DVALUE z, D3DVALUE theta) PURE;
    STDMETHOD(SetSortMode)(THIS_ D3DRMSORTMODE) PURE;
    STDMETHOD(SetTexture)(THIS_ LPDIRECT3DRMTEXTURE) PURE;
    STDMETHOD(SetTextureTopology)(THIS_ BOOL wrap_u, BOOL wrap_v) PURE;
    STDMETHOD(SetVelocity)(THIS_ LPDIRECT3DRMFRAME reference, D3DVALUE x, D3DVALUE y, D3DVALUE z, BOOL with_rotation) PURE;
    STDMETHOD(SetZbufferMode)(THIS_ D3DRMZBUFFERMODE) PURE;
    STDMETHOD(Transform)(THIS_ D3DVECTOR *d, D3DVECTOR *s) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMFrame2

DECLARE_INTERFACE_(IDirect3DRMFrame2, IDirect3DRMFrame)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMFrame methods
     */
    STDMETHOD(AddChild)(THIS_ LPDIRECT3DRMFRAME child) PURE;
    STDMETHOD(AddLight)(THIS_ LPDIRECT3DRMLIGHT) PURE;
    STDMETHOD(AddMoveCallback)(THIS_ D3DRMFRAMEMOVECALLBACK, VOID *arg) PURE;
    STDMETHOD(AddTransform)(THIS_ D3DRMCOMBINETYPE, D3DRMMATRIX4D) PURE;
    STDMETHOD(AddTranslation)(THIS_ D3DRMCOMBINETYPE, D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD(AddScale)(THIS_ D3DRMCOMBINETYPE, D3DVALUE sx, D3DVALUE sy, D3DVALUE sz) PURE;
    STDMETHOD(AddRotation)(THIS_ D3DRMCOMBINETYPE, D3DVALUE x, D3DVALUE y, D3DVALUE z, D3DVALUE theta) PURE;
    STDMETHOD(AddVisual)(THIS_ LPDIRECT3DRMVISUAL) PURE;
    STDMETHOD(GetChildren)(THIS_ LPDIRECT3DRMFRAMEARRAY *children) PURE;
    STDMETHOD_(D3DCOLOR, GetColor)(THIS) PURE;
    STDMETHOD(GetLights)(THIS_ LPDIRECT3DRMLIGHTARRAY *lights) PURE;
    STDMETHOD_(D3DRMMATERIALMODE, GetMaterialMode)(THIS) PURE;
    STDMETHOD(GetParent)(THIS_ LPDIRECT3DRMFRAME *) PURE;
    STDMETHOD(GetPosition)(THIS_ LPDIRECT3DRMFRAME reference, LPD3DVECTOR return_position) PURE;
    STDMETHOD(GetRotation)(THIS_ LPDIRECT3DRMFRAME reference, LPD3DVECTOR axis, LPD3DVALUE return_theta) PURE;
    STDMETHOD(GetScene)(THIS_ LPDIRECT3DRMFRAME *) PURE;
    STDMETHOD_(D3DRMSORTMODE, GetSortMode)(THIS) PURE;
    STDMETHOD(GetTexture)(THIS_ LPDIRECT3DRMTEXTURE *) PURE;
    STDMETHOD(GetTransform)(THIS_ D3DRMMATRIX4D return_matrix) PURE;
    STDMETHOD(GetVelocity)(THIS_ LPDIRECT3DRMFRAME reference, LPD3DVECTOR return_velocity, BOOL with_rotation) PURE;
    STDMETHOD(GetOrientation)(THIS_ LPDIRECT3DRMFRAME reference, LPD3DVECTOR dir, LPD3DVECTOR up) PURE;
    STDMETHOD(GetVisuals)(THIS_ LPDIRECT3DRMVISUALARRAY *visuals) PURE;
    STDMETHOD(GetTextureTopology)(THIS_ BOOL *wrap_u, BOOL *wrap_v) PURE;
    STDMETHOD(InverseTransform)(THIS_ D3DVECTOR *d, D3DVECTOR *s) PURE;
    STDMETHOD(Load)(THIS_ LPVOID filename, LPVOID name, D3DRMLOADOPTIONS loadflags, D3DRMLOADTEXTURECALLBACK, LPVOID lpArg)PURE;
    STDMETHOD(LookAt)(THIS_ LPDIRECT3DRMFRAME target, LPDIRECT3DRMFRAME reference, D3DRMFRAMECONSTRAINT) PURE;
    STDMETHOD(Move)(THIS_ D3DVALUE delta) PURE;
    STDMETHOD(DeleteChild)(THIS_ LPDIRECT3DRMFRAME) PURE;
    STDMETHOD(DeleteLight)(THIS_ LPDIRECT3DRMLIGHT) PURE;
    STDMETHOD(DeleteMoveCallback)(THIS_ D3DRMFRAMEMOVECALLBACK, VOID *arg) PURE;
    STDMETHOD(DeleteVisual)(THIS_ LPDIRECT3DRMVISUAL) PURE;
    STDMETHOD_(D3DCOLOR, GetSceneBackground)(THIS) PURE;
    STDMETHOD(GetSceneBackgroundDepth)(THIS_ LPDIRECTDRAWSURFACE *) PURE;
    STDMETHOD_(D3DCOLOR, GetSceneFogColor)(THIS) PURE;
    STDMETHOD_(BOOL, GetSceneFogEnable)(THIS) PURE;
    STDMETHOD_(D3DRMFOGMODE, GetSceneFogMode)(THIS) PURE;
    STDMETHOD(GetSceneFogParams)(THIS_ D3DVALUE *return_start, D3DVALUE *return_end, D3DVALUE *return_density) PURE;
    STDMETHOD(SetSceneBackground)(THIS_ D3DCOLOR) PURE;
    STDMETHOD(SetSceneBackgroundRGB)(THIS_ D3DVALUE red, D3DVALUE green, D3DVALUE blue) PURE;
    STDMETHOD(SetSceneBackgroundDepth)(THIS_ LPDIRECTDRAWSURFACE) PURE;
    STDMETHOD(SetSceneBackgroundImage)(THIS_ LPDIRECT3DRMTEXTURE) PURE;
    STDMETHOD(SetSceneFogEnable)(THIS_ BOOL) PURE;
    STDMETHOD(SetSceneFogColor)(THIS_ D3DCOLOR) PURE;
    STDMETHOD(SetSceneFogMode)(THIS_ D3DRMFOGMODE) PURE;
    STDMETHOD(SetSceneFogParams)(THIS_ D3DVALUE start, D3DVALUE end, D3DVALUE density) PURE;
    STDMETHOD(SetColor)(THIS_ D3DCOLOR) PURE;
    STDMETHOD(SetColorRGB)(THIS_ D3DVALUE red, D3DVALUE green, D3DVALUE blue) PURE;
    STDMETHOD_(D3DRMZBUFFERMODE, GetZbufferMode)(THIS) PURE;
    STDMETHOD(SetMaterialMode)(THIS_ D3DRMMATERIALMODE) PURE;
    STDMETHOD(SetOrientation)
    (   THIS_ LPDIRECT3DRMFRAME reference,
        D3DVALUE dx, D3DVALUE dy, D3DVALUE dz,
        D3DVALUE ux, D3DVALUE uy, D3DVALUE uz
    ) PURE;
    STDMETHOD(SetPosition)(THIS_ LPDIRECT3DRMFRAME reference, D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD(SetRotation)(THIS_ LPDIRECT3DRMFRAME reference, D3DVALUE x, D3DVALUE y, D3DVALUE z, D3DVALUE theta) PURE;
    STDMETHOD(SetSortMode)(THIS_ D3DRMSORTMODE) PURE;
    STDMETHOD(SetTexture)(THIS_ LPDIRECT3DRMTEXTURE) PURE;
    STDMETHOD(SetTextureTopology)(THIS_ BOOL wrap_u, BOOL wrap_v) PURE;
    STDMETHOD(SetVelocity)(THIS_ LPDIRECT3DRMFRAME reference, D3DVALUE x, D3DVALUE y, D3DVALUE z, BOOL with_rotation) PURE;
    STDMETHOD(SetZbufferMode)(THIS_ D3DRMZBUFFERMODE) PURE;
    STDMETHOD(Transform)(THIS_ D3DVECTOR *d, D3DVECTOR *s) PURE;

    /*
     * IDirect3DRMFrame2 methods
     */
    STDMETHOD(AddMoveCallback2)(THIS_ D3DRMFRAMEMOVECALLBACK, VOID *arg, DWORD dwFlags) PURE;
    STDMETHOD(GetBox)(THIS_ LPD3DRMBOX) PURE;
    STDMETHOD_(BOOL, GetBoxEnable)(THIS) PURE;
    STDMETHOD(GetAxes)(THIS_ LPD3DVECTOR dir, LPD3DVECTOR up);
    STDMETHOD(GetMaterial)(THIS_ LPDIRECT3DRMMATERIAL *) PURE;
    STDMETHOD_(BOOL, GetInheritAxes)(THIS);
    STDMETHOD(GetHierarchyBox)(THIS_ LPD3DRMBOX) PURE;

    STDMETHOD(SetBox)(THIS_ LPD3DRMBOX) PURE;
    STDMETHOD(SetBoxEnable)(THIS_ BOOL) PURE;
    STDMETHOD(SetAxes)(THIS_ D3DVALUE dx, D3DVALUE dy, D3DVALUE dz,
                       D3DVALUE ux, D3DVALUE uy, D3DVALUE uz);
    STDMETHOD(SetInheritAxes)(THIS_ BOOL inherit_from_parent);
    STDMETHOD(SetMaterial)(THIS_ LPDIRECT3DRMMATERIAL) PURE;
    STDMETHOD(SetQuaternion)(THIS_ LPDIRECT3DRMFRAME reference, D3DRMQUATERNION *q) PURE;

    STDMETHOD(RayPick)(THIS_ LPDIRECT3DRMFRAME reference, LPD3DRMRAY ray, DWORD dwFlags, LPDIRECT3DRMPICKED2ARRAY *return_visuals) PURE;
    STDMETHOD(Save)(THIS_ LPCSTR filename, D3DRMXOFFORMAT d3dFormat,
                    D3DRMSAVEOPTIONS d3dSaveFlags);
};

#undef INTERFACE
#define INTERFACE IDirect3DRMFrame3

DECLARE_INTERFACE_(IDirect3DRMFrame3, IDirect3DRMVisual)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMFrame3 methods
     */
    STDMETHOD(AddChild)(THIS_ LPDIRECT3DRMFRAME3 child) PURE;
    STDMETHOD(AddLight)(THIS_ LPDIRECT3DRMLIGHT) PURE;
    STDMETHOD(AddMoveCallback)(THIS_ D3DRMFRAME3MOVECALLBACK, VOID *arg, DWORD dwFlags) PURE;
    STDMETHOD(AddTransform)(THIS_ D3DRMCOMBINETYPE, D3DRMMATRIX4D) PURE;
    STDMETHOD(AddTranslation)(THIS_ D3DRMCOMBINETYPE, D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD(AddScale)(THIS_ D3DRMCOMBINETYPE, D3DVALUE sx, D3DVALUE sy, D3DVALUE sz) PURE;
    STDMETHOD(AddRotation)(THIS_ D3DRMCOMBINETYPE, D3DVALUE x, D3DVALUE y, D3DVALUE z, D3DVALUE theta) PURE;
    STDMETHOD(AddVisual)(THIS_ LPUNKNOWN) PURE;
    STDMETHOD(GetChildren)(THIS_ LPDIRECT3DRMFRAMEARRAY *children) PURE;
    STDMETHOD_(D3DCOLOR, GetColor)(THIS) PURE;
    STDMETHOD(GetLights)(THIS_ LPDIRECT3DRMLIGHTARRAY *lights) PURE;
    STDMETHOD_(D3DRMMATERIALMODE, GetMaterialMode)(THIS) PURE;
    STDMETHOD(GetParent)(THIS_ LPDIRECT3DRMFRAME3 *) PURE;
    STDMETHOD(GetPosition)(THIS_ LPDIRECT3DRMFRAME3 reference, LPD3DVECTOR return_position) PURE;
    STDMETHOD(GetRotation)(THIS_ LPDIRECT3DRMFRAME3 reference, LPD3DVECTOR axis, LPD3DVALUE return_theta) PURE;
    STDMETHOD(GetScene)(THIS_ LPDIRECT3DRMFRAME3 *) PURE;
    STDMETHOD_(D3DRMSORTMODE, GetSortMode)(THIS) PURE;
    STDMETHOD(GetTexture)(THIS_ LPDIRECT3DRMTEXTURE3 *) PURE;
    STDMETHOD(GetTransform)(THIS_ LPDIRECT3DRMFRAME3 reference,
                             D3DRMMATRIX4D rmMatrix) PURE;
    STDMETHOD(GetVelocity)(THIS_ LPDIRECT3DRMFRAME3 reference, LPD3DVECTOR return_velocity, BOOL with_rotation) PURE;
    STDMETHOD(GetOrientation)(THIS_ LPDIRECT3DRMFRAME3 reference, LPD3DVECTOR dir, LPD3DVECTOR up) PURE;
    STDMETHOD(GetVisuals)(THIS_ LPDWORD lpdwCount, LPUNKNOWN *) PURE;
    STDMETHOD(InverseTransform)(THIS_ D3DVECTOR *d, D3DVECTOR *s) PURE;
    STDMETHOD(Load)(THIS_ LPVOID filename, LPVOID name, D3DRMLOADOPTIONS loadflags, D3DRMLOADTEXTURE3CALLBACK, LPVOID lpArg)PURE;
    STDMETHOD(LookAt)(THIS_ LPDIRECT3DRMFRAME3 target, LPDIRECT3DRMFRAME3 reference, D3DRMFRAMECONSTRAINT) PURE;
    STDMETHOD(Move)(THIS_ D3DVALUE delta) PURE;
    STDMETHOD(DeleteChild)(THIS_ LPDIRECT3DRMFRAME3) PURE;
    STDMETHOD(DeleteLight)(THIS_ LPDIRECT3DRMLIGHT) PURE;
    STDMETHOD(DeleteMoveCallback)(THIS_ D3DRMFRAME3MOVECALLBACK, VOID *arg) PURE;
    STDMETHOD(DeleteVisual)(THIS_ LPUNKNOWN) PURE;
    STDMETHOD_(D3DCOLOR, GetSceneBackground)(THIS) PURE;
    STDMETHOD(GetSceneBackgroundDepth)(THIS_ LPDIRECTDRAWSURFACE *) PURE;
    STDMETHOD_(D3DCOLOR, GetSceneFogColor)(THIS) PURE;
    STDMETHOD_(BOOL, GetSceneFogEnable)(THIS) PURE;
    STDMETHOD_(D3DRMFOGMODE, GetSceneFogMode)(THIS) PURE;
    STDMETHOD(GetSceneFogParams)(THIS_ D3DVALUE *return_start, D3DVALUE *return_end, D3DVALUE *return_density) PURE;
    STDMETHOD(SetSceneBackground)(THIS_ D3DCOLOR) PURE;
    STDMETHOD(SetSceneBackgroundRGB)(THIS_ D3DVALUE red, D3DVALUE green, D3DVALUE blue) PURE;
    STDMETHOD(SetSceneBackgroundDepth)(THIS_ LPDIRECTDRAWSURFACE) PURE;
    STDMETHOD(SetSceneBackgroundImage)(THIS_ LPDIRECT3DRMTEXTURE3) PURE;
    STDMETHOD(SetSceneFogEnable)(THIS_ BOOL) PURE;
    STDMETHOD(SetSceneFogColor)(THIS_ D3DCOLOR) PURE;
    STDMETHOD(SetSceneFogMode)(THIS_ D3DRMFOGMODE) PURE;
    STDMETHOD(SetSceneFogParams)(THIS_ D3DVALUE start, D3DVALUE end, D3DVALUE density) PURE;
    STDMETHOD(SetColor)(THIS_ D3DCOLOR) PURE;
    STDMETHOD(SetColorRGB)(THIS_ D3DVALUE red, D3DVALUE green, D3DVALUE blue) PURE;
    STDMETHOD_(D3DRMZBUFFERMODE, GetZbufferMode)(THIS) PURE;
    STDMETHOD(SetMaterialMode)(THIS_ D3DRMMATERIALMODE) PURE;
    STDMETHOD(SetOrientation)
    (   THIS_ LPDIRECT3DRMFRAME3 reference,
        D3DVALUE dx, D3DVALUE dy, D3DVALUE dz,
        D3DVALUE ux, D3DVALUE uy, D3DVALUE uz
    ) PURE;
    STDMETHOD(SetPosition)(THIS_ LPDIRECT3DRMFRAME3 reference, D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD(SetRotation)(THIS_ LPDIRECT3DRMFRAME3 reference, D3DVALUE x, D3DVALUE y, D3DVALUE z, D3DVALUE theta) PURE;
    STDMETHOD(SetSortMode)(THIS_ D3DRMSORTMODE) PURE;
    STDMETHOD(SetTexture)(THIS_ LPDIRECT3DRMTEXTURE3) PURE;
    STDMETHOD(SetVelocity)(THIS_ LPDIRECT3DRMFRAME3 reference, D3DVALUE x, D3DVALUE y, D3DVALUE z, BOOL with_rotation) PURE;
    STDMETHOD(SetZbufferMode)(THIS_ D3DRMZBUFFERMODE) PURE;
    STDMETHOD(Transform)(THIS_ D3DVECTOR *d, D3DVECTOR *s) PURE;
    STDMETHOD(GetBox)(THIS_ LPD3DRMBOX) PURE;
    STDMETHOD_(BOOL, GetBoxEnable)(THIS) PURE;
    STDMETHOD(GetAxes)(THIS_ LPD3DVECTOR dir, LPD3DVECTOR up);
    STDMETHOD(GetMaterial)(THIS_ LPDIRECT3DRMMATERIAL2 *) PURE;
    STDMETHOD_(BOOL, GetInheritAxes)(THIS);
    STDMETHOD(GetHierarchyBox)(THIS_ LPD3DRMBOX) PURE;

    STDMETHOD(SetBox)(THIS_ LPD3DRMBOX) PURE;
    STDMETHOD(SetBoxEnable)(THIS_ BOOL) PURE;
    STDMETHOD(SetAxes)(THIS_ D3DVALUE dx, D3DVALUE dy, D3DVALUE dz,
                       D3DVALUE ux, D3DVALUE uy, D3DVALUE uz);
    STDMETHOD(SetInheritAxes)(THIS_ BOOL inherit_from_parent);
    STDMETHOD(SetMaterial)(THIS_ LPDIRECT3DRMMATERIAL2) PURE;
    STDMETHOD(SetQuaternion)(THIS_ LPDIRECT3DRMFRAME3 reference, D3DRMQUATERNION *q) PURE;

    STDMETHOD(RayPick)(THIS_ LPDIRECT3DRMFRAME3 reference, LPD3DRMRAY ray, DWORD dwFlags, LPDIRECT3DRMPICKED2ARRAY *return_visuals) PURE;
    STDMETHOD(Save)(THIS_ LPCSTR filename, D3DRMXOFFORMAT d3dFormat,
                    D3DRMSAVEOPTIONS d3dSaveFlags);
    STDMETHOD(TransformVectors)(THIS_ LPDIRECT3DRMFRAME3 reference,
                                DWORD dwNumVectors,
                                LPD3DVECTOR lpDstVectors,
                                LPD3DVECTOR lpSrcVectors) PURE;
    STDMETHOD(InverseTransformVectors)(THIS_ LPDIRECT3DRMFRAME3 reference,
                                       DWORD dwNumVectors,
                                       LPD3DVECTOR lpDstVectors,
                                       LPD3DVECTOR lpSrcVectors) PURE;
    STDMETHOD(SetTraversalOptions)(THIS_ DWORD dwFlags) PURE;
    STDMETHOD(GetTraversalOptions)(THIS_ LPDWORD lpdwFlags) PURE;
    STDMETHOD(SetSceneFogMethod)(THIS_ DWORD dwFlags) PURE;
    STDMETHOD(GetSceneFogMethod)(THIS_ LPDWORD lpdwFlags) PURE;
    STDMETHOD(SetMaterialOverride)(THIS_ LPD3DRMMATERIALOVERRIDE) PURE;
    STDMETHOD(GetMaterialOverride)(THIS_ LPD3DRMMATERIALOVERRIDE) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMMesh

DECLARE_INTERFACE_(IDirect3DRMMesh, IDirect3DRMVisual)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMMesh methods
     */
    STDMETHOD(Scale)(THIS_ D3DVALUE sx, D3DVALUE sy, D3DVALUE sz) PURE;
    STDMETHOD(Translate)(THIS_ D3DVALUE tx, D3DVALUE ty, D3DVALUE tz) PURE;
    STDMETHOD(GetBox)(THIS_ D3DRMBOX *) PURE;
    STDMETHOD(AddGroup)(THIS_ unsigned vCount, unsigned fCount, unsigned vPerFace, unsigned *fData, D3DRMGROUPINDEX *returnId) PURE;
    STDMETHOD(SetVertices)(THIS_ D3DRMGROUPINDEX id, unsigned index, unsigned count, D3DRMVERTEX *values) PURE;
    STDMETHOD(SetGroupColor)(THIS_ D3DRMGROUPINDEX id, D3DCOLOR value) PURE;
    STDMETHOD(SetGroupColorRGB)(THIS_ D3DRMGROUPINDEX id, D3DVALUE red, D3DVALUE green, D3DVALUE blue) PURE;
    STDMETHOD(SetGroupMapping)(THIS_ D3DRMGROUPINDEX id, D3DRMMAPPING value) PURE;
    STDMETHOD(SetGroupQuality)(THIS_ D3DRMGROUPINDEX id, D3DRMRENDERQUALITY value) PURE;
    STDMETHOD(SetGroupMaterial)(THIS_ D3DRMGROUPINDEX id, LPDIRECT3DRMMATERIAL value) PURE;
    STDMETHOD(SetGroupTexture)(THIS_ D3DRMGROUPINDEX id, LPDIRECT3DRMTEXTURE value) PURE;

    STDMETHOD_(unsigned, GetGroupCount)(THIS) PURE;
    STDMETHOD(GetGroup)(THIS_ D3DRMGROUPINDEX id, unsigned *vCount, unsigned *fCount, unsigned *vPerFace, DWORD *fDataSize, unsigned *fData) PURE;
    STDMETHOD(GetVertices)(THIS_ D3DRMGROUPINDEX id, DWORD index, DWORD count, D3DRMVERTEX *returnPtr) PURE;
    STDMETHOD_(D3DCOLOR, GetGroupColor)(THIS_ D3DRMGROUPINDEX id) PURE;
    STDMETHOD_(D3DRMMAPPING, GetGroupMapping)(THIS_ D3DRMGROUPINDEX id) PURE;
    STDMETHOD_(D3DRMRENDERQUALITY, GetGroupQuality)(THIS_ D3DRMGROUPINDEX id) PURE;
    STDMETHOD(GetGroupMaterial)(THIS_ D3DRMGROUPINDEX id, LPDIRECT3DRMMATERIAL *returnPtr) PURE;
    STDMETHOD(GetGroupTexture)(THIS_ D3DRMGROUPINDEX id, LPDIRECT3DRMTEXTURE *returnPtr) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMProgressiveMesh

DECLARE_INTERFACE_(IDirect3DRMProgressiveMesh, IDirect3DRMVisual)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMProgressiveMesh methods
     */
    STDMETHOD(Load) (THIS_ LPVOID lpObjLocation, LPVOID lpObjId,
                     D3DRMLOADOPTIONS dloLoadflags, D3DRMLOADTEXTURECALLBACK lpCallback,
                     LPVOID lpArg) PURE;
    STDMETHOD(GetLoadStatus) (THIS_ LPD3DRMPMESHLOADSTATUS lpStatus) PURE;
    STDMETHOD(SetMinRenderDetail) (THIS_ D3DVALUE d3dVal) PURE;
    STDMETHOD(Abort) (THIS_ DWORD dwFlags) PURE;

    STDMETHOD(GetFaceDetail) (THIS_ LPDWORD lpdwCount) PURE;
    STDMETHOD(GetVertexDetail) (THIS_ LPDWORD lpdwCount) PURE;
    STDMETHOD(SetFaceDetail) (THIS_ DWORD dwCount) PURE;
    STDMETHOD(SetVertexDetail) (THIS_ DWORD dwCount) PURE;
    STDMETHOD(GetFaceDetailRange) (THIS_ LPDWORD lpdwMin, LPDWORD lpdwMax) PURE;
    STDMETHOD(GetVertexDetailRange) (THIS_ LPDWORD lpdwMin, LPDWORD lpdwMax) PURE;
    STDMETHOD(GetDetail) (THIS_ D3DVALUE *lpdvVal) PURE;
    STDMETHOD(SetDetail) (THIS_ D3DVALUE d3dVal) PURE;

    STDMETHOD(RegisterEvents) (THIS_ HANDLE hEvent, DWORD dwFlags, DWORD dwReserved) PURE;
    STDMETHOD(CreateMesh) (THIS_ LPDIRECT3DRMMESH *lplpD3DRMMesh) PURE;
    STDMETHOD(Duplicate) (THIS_ LPDIRECT3DRMPROGRESSIVEMESH *lplpD3DRMPMesh) PURE;
    STDMETHOD(GetBox) (THIS_ LPD3DRMBOX lpBBox) PURE;
    STDMETHOD(SetQuality) (THIS_ D3DRMRENDERQUALITY) PURE;
    STDMETHOD(GetQuality) (THIS_ LPD3DRMRENDERQUALITY lpdwquality) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMShadow

DECLARE_INTERFACE_(IDirect3DRMShadow, IDirect3DRMVisual)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMShadow methods
     */
    STDMETHOD(Init)
    (   THIS_ LPDIRECT3DRMVISUAL visual, LPDIRECT3DRMLIGHT light,
        D3DVALUE px, D3DVALUE py, D3DVALUE pz,
        D3DVALUE nx, D3DVALUE ny, D3DVALUE nz
    ) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMShadow2

DECLARE_INTERFACE_(IDirect3DRMShadow2, IDirect3DRMVisual)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMShadow methods
     */
    STDMETHOD(Init)
    (   THIS_ LPUNKNOWN pUNK, LPDIRECT3DRMLIGHT light,
        D3DVALUE px, D3DVALUE py, D3DVALUE pz,
        D3DVALUE nx, D3DVALUE ny, D3DVALUE nz
    ) PURE;

    /*
     * IDirect3DRMShadow2 methods
     */
    STDMETHOD(GetVisual)(THIS_ LPDIRECT3DRMVISUAL *) PURE;
    STDMETHOD(SetVisual)(THIS_ LPUNKNOWN pUNK, DWORD) PURE;
    STDMETHOD(GetLight)(THIS_ LPDIRECT3DRMLIGHT *) PURE;
    STDMETHOD(SetLight)(THIS_ LPDIRECT3DRMLIGHT, DWORD) PURE;
    STDMETHOD(GetPlane)(THIS_ LPD3DVALUE px, LPD3DVALUE py, LPD3DVALUE pz,
                        LPD3DVALUE nx, LPD3DVALUE ny, LPD3DVALUE nz) PURE;
    STDMETHOD(SetPlane)(THIS_ D3DVALUE px, D3DVALUE py, D3DVALUE pz,
                        D3DVALUE nx, D3DVALUE ny, D3DVALUE nz, DWORD) PURE;
    STDMETHOD(GetOptions)(THIS_ LPDWORD) PURE;
    STDMETHOD(SetOptions)(THIS_ DWORD) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMFace

DECLARE_INTERFACE_(IDirect3DRMFace, IDirect3DRMObject)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMFace methods
     */
     STDMETHOD(AddVertex)(THIS_ D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
     STDMETHOD(AddVertexAndNormalIndexed)(THIS_ DWORD vertex, DWORD normal) PURE;
     STDMETHOD(SetColorRGB)(THIS_ D3DVALUE, D3DVALUE, D3DVALUE) PURE;
     STDMETHOD(SetColor)(THIS_ D3DCOLOR) PURE;
     STDMETHOD(SetTexture)(THIS_ LPDIRECT3DRMTEXTURE) PURE;
     STDMETHOD(SetTextureCoordinates)(THIS_ DWORD vertex, D3DVALUE u, D3DVALUE v) PURE;
     STDMETHOD(SetMaterial)(THIS_ LPDIRECT3DRMMATERIAL) PURE;
     STDMETHOD(SetTextureTopology)(THIS_ BOOL wrap_u, BOOL wrap_v) PURE;

     STDMETHOD(GetVertex)(THIS_ DWORD index, D3DVECTOR *vertex, D3DVECTOR *normal) PURE;
     STDMETHOD(GetVertices)(THIS_ DWORD *vertex_count, D3DVECTOR *coords, D3DVECTOR *normals);
     STDMETHOD(GetTextureCoordinates)(THIS_ DWORD vertex, D3DVALUE *u, D3DVALUE *v) PURE;
     STDMETHOD(GetTextureTopology)(THIS_ BOOL *wrap_u, BOOL *wrap_v) PURE;
     STDMETHOD(GetNormal)(THIS_ D3DVECTOR *) PURE;
     STDMETHOD(GetTexture)(THIS_ LPDIRECT3DRMTEXTURE *) PURE;
     STDMETHOD(GetMaterial)(THIS_ LPDIRECT3DRMMATERIAL *) PURE;

     STDMETHOD_(int, GetVertexCount)(THIS) PURE;
     STDMETHOD_(int, GetVertexIndex)(THIS_ DWORD which) PURE;
     STDMETHOD_(int, GetTextureCoordinateIndex)(THIS_ DWORD which) PURE;
     STDMETHOD_(D3DCOLOR, GetColor)(THIS) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMFace2

DECLARE_INTERFACE_(IDirect3DRMFace2, IDirect3DRMObject)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMFace methods
     */
     STDMETHOD(AddVertex)(THIS_ D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
     STDMETHOD(AddVertexAndNormalIndexed)(THIS_ DWORD vertex, DWORD normal) PURE;
     STDMETHOD(SetColorRGB)(THIS_ D3DVALUE, D3DVALUE, D3DVALUE) PURE;
     STDMETHOD(SetColor)(THIS_ D3DCOLOR) PURE;
     STDMETHOD(SetTexture)(THIS_ LPDIRECT3DRMTEXTURE3) PURE;
     STDMETHOD(SetTextureCoordinates)(THIS_ DWORD vertex, D3DVALUE u, D3DVALUE v) PURE;
     STDMETHOD(SetMaterial)(THIS_ LPDIRECT3DRMMATERIAL2) PURE;
     STDMETHOD(SetTextureTopology)(THIS_ BOOL wrap_u, BOOL wrap_v) PURE;

     STDMETHOD(GetVertex)(THIS_ DWORD index, D3DVECTOR *vertex, D3DVECTOR *normal) PURE;
     STDMETHOD(GetVertices)(THIS_ DWORD *vertex_count, D3DVECTOR *coords, D3DVECTOR *normals);
     STDMETHOD(GetTextureCoordinates)(THIS_ DWORD vertex, D3DVALUE *u, D3DVALUE *v) PURE;
     STDMETHOD(GetTextureTopology)(THIS_ BOOL *wrap_u, BOOL *wrap_v) PURE;
     STDMETHOD(GetNormal)(THIS_ D3DVECTOR *) PURE;
     STDMETHOD(GetTexture)(THIS_ LPDIRECT3DRMTEXTURE3 *) PURE;
     STDMETHOD(GetMaterial)(THIS_ LPDIRECT3DRMMATERIAL2 *) PURE;

     STDMETHOD_(int, GetVertexCount)(THIS) PURE;
     STDMETHOD_(int, GetVertexIndex)(THIS_ DWORD which) PURE;
     STDMETHOD_(int, GetTextureCoordinateIndex)(THIS_ DWORD which) PURE;
     STDMETHOD_(D3DCOLOR, GetColor)(THIS) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMMeshBuilder

DECLARE_INTERFACE_(IDirect3DRMMeshBuilder, IDirect3DRMVisual)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMMeshBuilder methods
     */
    STDMETHOD(Load)(THIS_ LPVOID filename, LPVOID name, D3DRMLOADOPTIONS loadflags, D3DRMLOADTEXTURECALLBACK, LPVOID lpArg) PURE;
    STDMETHOD(Save)(THIS_ const char *filename, D3DRMXOFFORMAT, D3DRMSAVEOPTIONS save) PURE;
    STDMETHOD(Scale)(THIS_ D3DVALUE sx, D3DVALUE sy, D3DVALUE sz) PURE;
    STDMETHOD(Translate)(THIS_ D3DVALUE tx, D3DVALUE ty, D3DVALUE tz) PURE;
    STDMETHOD(SetColorSource)(THIS_ D3DRMCOLORSOURCE) PURE;
    STDMETHOD(GetBox)(THIS_ D3DRMBOX *) PURE;
    STDMETHOD(GenerateNormals)(THIS) PURE;
    STDMETHOD_(D3DRMCOLORSOURCE, GetColorSource)(THIS) PURE;

    STDMETHOD(AddMesh)(THIS_ LPDIRECT3DRMMESH) PURE;
    STDMETHOD(AddMeshBuilder)(THIS_ LPDIRECT3DRMMESHBUILDER) PURE;
    STDMETHOD(AddFrame)(THIS_ LPDIRECT3DRMFRAME) PURE;
    STDMETHOD(AddFace)(THIS_ LPDIRECT3DRMFACE) PURE;
    STDMETHOD(AddFaces)
    (   THIS_ DWORD vcount, D3DVECTOR *vertices, DWORD ncount, D3DVECTOR *normals,
        DWORD *data, LPDIRECT3DRMFACEARRAY*
    ) PURE;
    STDMETHOD(ReserveSpace)(THIS_ DWORD vertex_Count, DWORD normal_count, DWORD face_count) PURE;
    STDMETHOD(SetColorRGB)(THIS_ D3DVALUE red, D3DVALUE green, D3DVALUE blue) PURE;
    STDMETHOD(SetColor)(THIS_ D3DCOLOR) PURE;
    STDMETHOD(SetTexture)(THIS_ LPDIRECT3DRMTEXTURE) PURE;
    STDMETHOD(SetMaterial)(THIS_ LPDIRECT3DRMMATERIAL) PURE;
    STDMETHOD(SetTextureTopology)(THIS_ BOOL wrap_u, BOOL wrap_v) PURE;
    STDMETHOD(SetQuality)(THIS_ D3DRMRENDERQUALITY) PURE;
    STDMETHOD(SetPerspective)(THIS_ BOOL) PURE;
    STDMETHOD(SetVertex)(THIS_ DWORD index, D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD(SetNormal)(THIS_ DWORD index, D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD(SetTextureCoordinates)(THIS_ DWORD index, D3DVALUE u, D3DVALUE v) PURE;
    STDMETHOD(SetVertexColor)(THIS_ DWORD index, D3DCOLOR) PURE;
    STDMETHOD(SetVertexColorRGB)(THIS_ DWORD index, D3DVALUE red, D3DVALUE green, D3DVALUE blue) PURE;

    STDMETHOD(GetFaces)(THIS_ LPDIRECT3DRMFACEARRAY*) PURE;
    STDMETHOD(GetVertices)
    (   THIS_ DWORD *vcount, D3DVECTOR *vertices, DWORD *ncount, D3DVECTOR *normals, DWORD *face_data_size, DWORD *face_data
    ) PURE;
    STDMETHOD(GetTextureCoordinates)(THIS_ DWORD index, D3DVALUE *u, D3DVALUE *v) PURE;

    STDMETHOD_(int, AddVertex)(THIS_ D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD_(int, AddNormal)(THIS_ D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD(CreateFace)(THIS_ LPDIRECT3DRMFACE*) PURE;
    STDMETHOD_(D3DRMRENDERQUALITY, GetQuality)(THIS) PURE;
    STDMETHOD_(BOOL, GetPerspective)(THIS) PURE;
    STDMETHOD_(int, GetFaceCount)(THIS) PURE;
    STDMETHOD_(int, GetVertexCount)(THIS) PURE;
    STDMETHOD_(D3DCOLOR, GetVertexColor)(THIS_ DWORD index) PURE;

    STDMETHOD(CreateMesh)(THIS_ LPDIRECT3DRMMESH*) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMMeshBuilder2

DECLARE_INTERFACE_(IDirect3DRMMeshBuilder2, IDirect3DRMMeshBuilder)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMMeshBuilder methods
     */
    STDMETHOD(Load)(THIS_ LPVOID filename, LPVOID name, D3DRMLOADOPTIONS loadflags, D3DRMLOADTEXTURECALLBACK, LPVOID lpArg) PURE;
    STDMETHOD(Save)(THIS_ const char *filename, D3DRMXOFFORMAT, D3DRMSAVEOPTIONS save) PURE;
    STDMETHOD(Scale)(THIS_ D3DVALUE sx, D3DVALUE sy, D3DVALUE sz) PURE;
    STDMETHOD(Translate)(THIS_ D3DVALUE tx, D3DVALUE ty, D3DVALUE tz) PURE;
    STDMETHOD(SetColorSource)(THIS_ D3DRMCOLORSOURCE) PURE;
    STDMETHOD(GetBox)(THIS_ D3DRMBOX *) PURE;
    STDMETHOD(GenerateNormals)(THIS) PURE;
    STDMETHOD_(D3DRMCOLORSOURCE, GetColorSource)(THIS) PURE;

    STDMETHOD(AddMesh)(THIS_ LPDIRECT3DRMMESH) PURE;
    STDMETHOD(AddMeshBuilder)(THIS_ LPDIRECT3DRMMESHBUILDER) PURE;
    STDMETHOD(AddFrame)(THIS_ LPDIRECT3DRMFRAME) PURE;
    STDMETHOD(AddFace)(THIS_ LPDIRECT3DRMFACE) PURE;
    STDMETHOD(AddFaces)
    (   THIS_ DWORD vcount, D3DVECTOR *vertices, DWORD ncount, D3DVECTOR *normals,
        DWORD *data, LPDIRECT3DRMFACEARRAY*
    ) PURE;
    STDMETHOD(ReserveSpace)(THIS_ DWORD vertex_Count, DWORD normal_count, DWORD face_count) PURE;
    STDMETHOD(SetColorRGB)(THIS_ D3DVALUE red, D3DVALUE green, D3DVALUE blue) PURE;
    STDMETHOD(SetColor)(THIS_ D3DCOLOR) PURE;
    STDMETHOD(SetTexture)(THIS_ LPDIRECT3DRMTEXTURE) PURE;
    STDMETHOD(SetMaterial)(THIS_ LPDIRECT3DRMMATERIAL) PURE;
    STDMETHOD(SetTextureTopology)(THIS_ BOOL wrap_u, BOOL wrap_v) PURE;
    STDMETHOD(SetQuality)(THIS_ D3DRMRENDERQUALITY) PURE;
    STDMETHOD(SetPerspective)(THIS_ BOOL) PURE;
    STDMETHOD(SetVertex)(THIS_ DWORD index, D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD(SetNormal)(THIS_ DWORD index, D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD(SetTextureCoordinates)(THIS_ DWORD index, D3DVALUE u, D3DVALUE v) PURE;
    STDMETHOD(SetVertexColor)(THIS_ DWORD index, D3DCOLOR) PURE;
    STDMETHOD(SetVertexColorRGB)(THIS_ DWORD index, D3DVALUE red, D3DVALUE green, D3DVALUE blue) PURE;

    STDMETHOD(GetFaces)(THIS_ LPDIRECT3DRMFACEARRAY*) PURE;
    STDMETHOD(GetVertices)
    (   THIS_ DWORD *vcount, D3DVECTOR *vertices, DWORD *ncount, D3DVECTOR *normals, DWORD *face_data_size, DWORD *face_data
    ) PURE;
    STDMETHOD(GetTextureCoordinates)(THIS_ DWORD index, D3DVALUE *u, D3DVALUE *v) PURE;

    STDMETHOD_(int, AddVertex)(THIS_ D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD_(int, AddNormal)(THIS_ D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD(CreateFace)(THIS_ LPDIRECT3DRMFACE*) PURE;
    STDMETHOD_(D3DRMRENDERQUALITY, GetQuality)(THIS) PURE;
    STDMETHOD_(BOOL, GetPerspective)(THIS) PURE;
    STDMETHOD_(int, GetFaceCount)(THIS) PURE;
    STDMETHOD_(int, GetVertexCount)(THIS) PURE;
    STDMETHOD_(D3DCOLOR, GetVertexColor)(THIS_ DWORD index) PURE;

    STDMETHOD(CreateMesh)(THIS_ LPDIRECT3DRMMESH*) PURE;

    /*
     * IDirect3DRMMeshBuilder2 methods
     */
    STDMETHOD(GenerateNormals2)(THIS_ D3DVALUE crease, DWORD dwFlags) PURE;
    STDMETHOD(GetFace)(THIS_ DWORD index, LPDIRECT3DRMFACE*) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMMeshBuilder3

DECLARE_INTERFACE_(IDirect3DRMMeshBuilder3, IDirect3DRMVisual)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMMeshBuilder3 methods
     */
    STDMETHOD(Load)(THIS_ LPVOID filename, LPVOID name, D3DRMLOADOPTIONS loadflags, D3DRMLOADTEXTURE3CALLBACK, LPVOID lpArg) PURE;
    STDMETHOD(Save)(THIS_ const char *filename, D3DRMXOFFORMAT, D3DRMSAVEOPTIONS save) PURE;
    STDMETHOD(Scale)(THIS_ D3DVALUE sx, D3DVALUE sy, D3DVALUE sz) PURE;
    STDMETHOD(Translate)(THIS_ D3DVALUE tx, D3DVALUE ty, D3DVALUE tz) PURE;
    STDMETHOD(SetColorSource)(THIS_ D3DRMCOLORSOURCE) PURE;
    STDMETHOD(GetBox)(THIS_ D3DRMBOX *) PURE;
    STDMETHOD(GenerateNormals)(THIS_ D3DVALUE crease, DWORD dwFlags) PURE;
    STDMETHOD_(D3DRMCOLORSOURCE, GetColorSource)(THIS) PURE;

    STDMETHOD(AddMesh)(THIS_ LPDIRECT3DRMMESH) PURE;
    STDMETHOD(AddMeshBuilder)(THIS_ LPDIRECT3DRMMESHBUILDER3, DWORD dwFlags) PURE;
    STDMETHOD(AddFrame)(THIS_ LPDIRECT3DRMFRAME3) PURE;
    STDMETHOD(AddFace)(THIS_ LPDIRECT3DRMFACE2) PURE;
    STDMETHOD(AddFaces)
    (   THIS_ DWORD vcount, D3DVECTOR *vertices, DWORD ncount, D3DVECTOR *normals,
        DWORD *data, LPDIRECT3DRMFACEARRAY*
    ) PURE;
    STDMETHOD(ReserveSpace)(THIS_ DWORD vertex_Count, DWORD normal_count, DWORD face_count) PURE;
    STDMETHOD(SetColorRGB)(THIS_ D3DVALUE red, D3DVALUE green, D3DVALUE blue) PURE;
    STDMETHOD(SetColor)(THIS_ D3DCOLOR) PURE;
    STDMETHOD(SetTexture)(THIS_ LPDIRECT3DRMTEXTURE3) PURE;
    STDMETHOD(SetMaterial)(THIS_ LPDIRECT3DRMMATERIAL2) PURE;
    STDMETHOD(SetTextureTopology)(THIS_ BOOL wrap_u, BOOL wrap_v) PURE;
    STDMETHOD(SetQuality)(THIS_ D3DRMRENDERQUALITY) PURE;
    STDMETHOD(SetPerspective)(THIS_ BOOL) PURE;
    STDMETHOD(SetVertex)(THIS_ DWORD index, D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD(SetNormal)(THIS_ DWORD index, D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD(SetTextureCoordinates)(THIS_ DWORD index, D3DVALUE u, D3DVALUE v) PURE;
    STDMETHOD(SetVertexColor)(THIS_ DWORD index, D3DCOLOR) PURE;
    STDMETHOD(SetVertexColorRGB)(THIS_ DWORD index, D3DVALUE red, D3DVALUE green, D3DVALUE blue) PURE;
    STDMETHOD(GetFaces)(THIS_ LPDIRECT3DRMFACEARRAY*) PURE;
    STDMETHOD(GetGeometry)
    (   THIS_ DWORD *vcount, D3DVECTOR *vertices, DWORD *ncount, D3DVECTOR *normals, DWORD *face_data_size, DWORD *face_data
    ) PURE;
    STDMETHOD(GetTextureCoordinates)(THIS_ DWORD index, D3DVALUE *u, D3DVALUE *v) PURE;
    STDMETHOD_(int, AddVertex)(THIS_ D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD_(int, AddNormal)(THIS_ D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD(CreateFace)(THIS_ LPDIRECT3DRMFACE2 *) PURE;
    STDMETHOD_(D3DRMRENDERQUALITY, GetQuality)(THIS) PURE;
    STDMETHOD_(BOOL, GetPerspective)(THIS) PURE;
    STDMETHOD_(int, GetFaceCount)(THIS) PURE;
    STDMETHOD_(int, GetVertexCount)(THIS) PURE;
    STDMETHOD_(D3DCOLOR, GetVertexColor)(THIS_ DWORD index) PURE;
    STDMETHOD(CreateMesh)(THIS_ LPDIRECT3DRMMESH*) PURE;
    STDMETHOD(GetFace)(THIS_ DWORD index, LPDIRECT3DRMFACE2 *) PURE;
    STDMETHOD(GetVertex)(THIS_ DWORD dwIndex, LPD3DVECTOR lpVector) PURE;
    STDMETHOD(GetNormal)(THIS_ DWORD dwIndex, LPD3DVECTOR lpVector) PURE;
    STDMETHOD(DeleteVertices)(THIS_ DWORD dwIndexFirst, DWORD dwCount) PURE;
    STDMETHOD(DeleteNormals)(THIS_ DWORD dwIndexFirst, DWORD dwCount) PURE;
    STDMETHOD(DeleteFace)(THIS_ LPDIRECT3DRMFACE2) PURE;
    STDMETHOD(Empty)(THIS_ DWORD dwFlags) PURE;
    STDMETHOD(Optimize)(THIS_ DWORD dwFlags) PURE;
    STDMETHOD(AddFacesIndexed)(THIS_ DWORD dwFlags, DWORD *lpdwvIndices, DWORD *dwIndexFirst, DWORD *dwCount) PURE;
    STDMETHOD(CreateSubMesh)(THIS_ LPUNKNOWN *) PURE;
    STDMETHOD(GetParentMesh)(THIS_ DWORD, LPUNKNOWN *) PURE;
    STDMETHOD(GetSubMeshes)(THIS_ LPDWORD lpdwCount, LPUNKNOWN *) PURE;
    STDMETHOD(DeleteSubMesh)(THIS_ LPUNKNOWN) PURE;
    STDMETHOD(Enable)(THIS_ DWORD) PURE;
    STDMETHOD(GetEnable)(THIS_ DWORD *) PURE;
    STDMETHOD(AddTriangles)(THIS_ DWORD dwFlags, DWORD dwFormat,
                            DWORD dwVertexCount, LPVOID lpvData) PURE;
    STDMETHOD(SetVertices)(THIS_ DWORD dwIndexFirst, DWORD dwCount, LPD3DVECTOR) PURE;
    STDMETHOD(GetVertices)(THIS_ DWORD dwIndexFirst, LPDWORD lpdwCount, LPD3DVECTOR) PURE;
    STDMETHOD(SetNormals)(THIS_ DWORD dwIndexFirst, DWORD dwCount, LPD3DVECTOR) PURE;
    STDMETHOD(GetNormals)(THIS_ DWORD dwIndexFirst, LPDWORD lpdwCount, LPD3DVECTOR) PURE;
    STDMETHOD_(int, GetNormalCount)(THIS) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMLight

DECLARE_INTERFACE_(IDirect3DRMLight, IDirect3DRMObject)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMLight methods
     */
    STDMETHOD(SetType)(THIS_ D3DRMLIGHTTYPE) PURE;
    STDMETHOD(SetColor)(THIS_ D3DCOLOR) PURE;
    STDMETHOD(SetColorRGB)(THIS_ D3DVALUE red, D3DVALUE green, D3DVALUE blue) PURE;
    STDMETHOD(SetRange)(THIS_ D3DVALUE) PURE;
    STDMETHOD(SetUmbra)(THIS_ D3DVALUE) PURE;
    STDMETHOD(SetPenumbra)(THIS_ D3DVALUE) PURE;
    STDMETHOD(SetConstantAttenuation)(THIS_ D3DVALUE) PURE;
    STDMETHOD(SetLinearAttenuation)(THIS_ D3DVALUE) PURE;
    STDMETHOD(SetQuadraticAttenuation)(THIS_ D3DVALUE) PURE;

    STDMETHOD_(D3DVALUE, GetRange)(THIS) PURE;
    STDMETHOD_(D3DVALUE, GetUmbra)(THIS) PURE;
    STDMETHOD_(D3DVALUE, GetPenumbra)(THIS) PURE;
    STDMETHOD_(D3DVALUE, GetConstantAttenuation)(THIS) PURE;
    STDMETHOD_(D3DVALUE, GetLinearAttenuation)(THIS) PURE;
    STDMETHOD_(D3DVALUE, GetQuadraticAttenuation)(THIS) PURE;
    STDMETHOD_(D3DCOLOR, GetColor)(THIS) PURE;
    STDMETHOD_(D3DRMLIGHTTYPE, GetType)(THIS) PURE;

    STDMETHOD(SetEnableFrame)(THIS_ LPDIRECT3DRMFRAME) PURE;
    STDMETHOD(GetEnableFrame)(THIS_ LPDIRECT3DRMFRAME*) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMTexture

DECLARE_INTERFACE_(IDirect3DRMTexture, IDirect3DRMVisual)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMTexture methods
     */
    STDMETHOD(InitFromFile)(THIS_ const char *filename) PURE;
    STDMETHOD(InitFromSurface)(THIS_ LPDIRECTDRAWSURFACE lpDDS) PURE;
    STDMETHOD(InitFromResource)(THIS_ HRSRC) PURE;
    STDMETHOD(Changed)(THIS_ BOOL pixels, BOOL palette) PURE;

    STDMETHOD(SetColors)(THIS_ DWORD) PURE;
    STDMETHOD(SetShades)(THIS_ DWORD) PURE;
    STDMETHOD(SetDecalSize)(THIS_ D3DVALUE width, D3DVALUE height) PURE;
    STDMETHOD(SetDecalOrigin)(THIS_ LONG x, LONG y) PURE;
    STDMETHOD(SetDecalScale)(THIS_ DWORD) PURE;
    STDMETHOD(SetDecalTransparency)(THIS_ BOOL) PURE;
    STDMETHOD(SetDecalTransparentColor)(THIS_ D3DCOLOR) PURE;

    STDMETHOD(GetDecalSize)(THIS_ D3DVALUE *width_return, D3DVALUE *height_return) PURE;
    STDMETHOD(GetDecalOrigin)(THIS_ LONG *x_return, LONG *y_return) PURE;

    STDMETHOD_(D3DRMIMAGE *, GetImage)(THIS) PURE;
    STDMETHOD_(DWORD, GetShades)(THIS) PURE;
    STDMETHOD_(DWORD, GetColors)(THIS) PURE;
    STDMETHOD_(DWORD, GetDecalScale)(THIS) PURE;
    STDMETHOD_(BOOL, GetDecalTransparency)(THIS) PURE;
    STDMETHOD_(D3DCOLOR, GetDecalTransparentColor)(THIS) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMTexture2

DECLARE_INTERFACE_(IDirect3DRMTexture2, IDirect3DRMTexture)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMTexture methods
     */
    STDMETHOD(InitFromFile)(THIS_ const char *filename) PURE;
    STDMETHOD(InitFromSurface)(THIS_ LPDIRECTDRAWSURFACE lpDDS) PURE;
    STDMETHOD(InitFromResource)(THIS_ HRSRC) PURE;
    STDMETHOD(Changed)(THIS_ BOOL pixels, BOOL palette) PURE;

    STDMETHOD(SetColors)(THIS_ DWORD) PURE;
    STDMETHOD(SetShades)(THIS_ DWORD) PURE;
    STDMETHOD(SetDecalSize)(THIS_ D3DVALUE width, D3DVALUE height) PURE;
    STDMETHOD(SetDecalOrigin)(THIS_ LONG x, LONG y) PURE;
    STDMETHOD(SetDecalScale)(THIS_ DWORD) PURE;
    STDMETHOD(SetDecalTransparency)(THIS_ BOOL) PURE;
    STDMETHOD(SetDecalTransparentColor)(THIS_ D3DCOLOR) PURE;

    STDMETHOD(GetDecalSize)(THIS_ D3DVALUE *width_return, D3DVALUE *height_return) PURE;
    STDMETHOD(GetDecalOrigin)(THIS_ LONG *x_return, LONG *y_return) PURE;

    STDMETHOD_(D3DRMIMAGE *, GetImage)(THIS) PURE;
    STDMETHOD_(DWORD, GetShades)(THIS) PURE;
    STDMETHOD_(DWORD, GetColors)(THIS) PURE;
    STDMETHOD_(DWORD, GetDecalScale)(THIS) PURE;
    STDMETHOD_(BOOL, GetDecalTransparency)(THIS) PURE;
    STDMETHOD_(D3DCOLOR, GetDecalTransparentColor)(THIS) PURE;

    /*
     * IDirect3DRMTexture2 methods
     */
    STDMETHOD(InitFromImage)(THIS_ LPD3DRMIMAGE) PURE;
    STDMETHOD(InitFromResource2)(THIS_ HMODULE hModule, LPCTSTR strName, LPCTSTR strType) PURE;
    STDMETHOD(GenerateMIPMap)(THIS_ DWORD) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMTexture3

DECLARE_INTERFACE_(IDirect3DRMTexture3, IDirect3DRMVisual)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMTexture3 methods
     */
    STDMETHOD(InitFromFile)(THIS_ const char *filename) PURE;
    STDMETHOD(InitFromSurface)(THIS_ LPDIRECTDRAWSURFACE lpDDS) PURE;
    STDMETHOD(InitFromResource)(THIS_ HRSRC) PURE;
    STDMETHOD(Changed)(THIS_ DWORD dwFlags, DWORD dwcRects, LPRECT pRects) PURE;
    STDMETHOD(SetColors)(THIS_ DWORD) PURE;
    STDMETHOD(SetShades)(THIS_ DWORD) PURE;
    STDMETHOD(SetDecalSize)(THIS_ D3DVALUE width, D3DVALUE height) PURE;
    STDMETHOD(SetDecalOrigin)(THIS_ LONG x, LONG y) PURE;
    STDMETHOD(SetDecalScale)(THIS_ DWORD) PURE;
    STDMETHOD(SetDecalTransparency)(THIS_ BOOL) PURE;
    STDMETHOD(SetDecalTransparentColor)(THIS_ D3DCOLOR) PURE;

    STDMETHOD(GetDecalSize)(THIS_ D3DVALUE *width_return, D3DVALUE *height_return) PURE;
    STDMETHOD(GetDecalOrigin)(THIS_ LONG *x_return, LONG *y_return) PURE;

    STDMETHOD_(D3DRMIMAGE *, GetImage)(THIS) PURE;
    STDMETHOD_(DWORD, GetShades)(THIS) PURE;
    STDMETHOD_(DWORD, GetColors)(THIS) PURE;
    STDMETHOD_(DWORD, GetDecalScale)(THIS) PURE;
    STDMETHOD_(BOOL, GetDecalTransparency)(THIS) PURE;
    STDMETHOD_(D3DCOLOR, GetDecalTransparentColor)(THIS) PURE;
    STDMETHOD(InitFromImage)(THIS_ LPD3DRMIMAGE) PURE;
    STDMETHOD(InitFromResource2)(THIS_ HMODULE hModule, LPCTSTR strName, LPCTSTR strType) PURE;
    STDMETHOD(GenerateMIPMap)(THIS_ DWORD) PURE;
    STDMETHOD(GetSurface)(THIS_ DWORD dwFlags, LPDIRECTDRAWSURFACE* lplpDDS) PURE;
    STDMETHOD(SetCacheOptions)(THIS_ LONG lImportance, DWORD dwFlags) PURE;
    STDMETHOD(GetCacheOptions)(THIS_ LPLONG lplImportance, LPDWORD lpdwFlags) PURE;
    STDMETHOD(SetDownsampleCallback)(THIS_ D3DRMDOWNSAMPLECALLBACK pCallback, LPVOID pArg) PURE;
    STDMETHOD(SetValidationCallback)(THIS_ D3DRMVALIDATIONCALLBACK pCallback, LPVOID pArg) PURE;
};


#undef INTERFACE
#define INTERFACE IDirect3DRMWrap

DECLARE_INTERFACE_(IDirect3DRMWrap, IDirect3DRMObject)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMWrap methods
     */
    STDMETHOD(Init)
    (   THIS_ D3DRMWRAPTYPE, LPDIRECT3DRMFRAME ref,
        D3DVALUE ox, D3DVALUE oy, D3DVALUE oz,
        D3DVALUE dx, D3DVALUE dy, D3DVALUE dz,
        D3DVALUE ux, D3DVALUE uy, D3DVALUE uz,
        D3DVALUE ou, D3DVALUE ov,
        D3DVALUE su, D3DVALUE sv
    ) PURE;
    STDMETHOD(Apply)(THIS_ LPDIRECT3DRMOBJECT) PURE;
    STDMETHOD(ApplyRelative)(THIS_ LPDIRECT3DRMFRAME frame, LPDIRECT3DRMOBJECT) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMMaterial

DECLARE_INTERFACE_(IDirect3DRMMaterial, IDirect3DRMObject)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMMaterial methods
     */
    STDMETHOD(SetPower)(THIS_ D3DVALUE power) PURE;
    STDMETHOD(SetSpecular)(THIS_ D3DVALUE r, D3DVALUE g, D3DVALUE b) PURE;
    STDMETHOD(SetEmissive)(THIS_ D3DVALUE r, D3DVALUE g, D3DVALUE b) PURE;

    STDMETHOD_(D3DVALUE, GetPower)(THIS) PURE;
    STDMETHOD(GetSpecular)(THIS_ D3DVALUE* r, D3DVALUE* g, D3DVALUE* b) PURE;
    STDMETHOD(GetEmissive)(THIS_ D3DVALUE* r, D3DVALUE* g, D3DVALUE* b) PURE;
};


#undef INTERFACE
#define INTERFACE IDirect3DRMMaterial2

DECLARE_INTERFACE_(IDirect3DRMMaterial2, IDirect3DRMObject)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMMaterial2 methods
     */
    STDMETHOD(SetPower)(THIS_ D3DVALUE power) PURE;
    STDMETHOD(SetSpecular)(THIS_ D3DVALUE r, D3DVALUE g, D3DVALUE b) PURE;
    STDMETHOD(SetEmissive)(THIS_ D3DVALUE r, D3DVALUE g, D3DVALUE b) PURE;
    STDMETHOD_(D3DVALUE, GetPower)(THIS) PURE;
    STDMETHOD(GetSpecular)(THIS_ D3DVALUE* r, D3DVALUE* g, D3DVALUE* b) PURE;
    STDMETHOD(GetEmissive)(THIS_ D3DVALUE* r, D3DVALUE* g, D3DVALUE* b) PURE;
    STDMETHOD(GetAmbient)(THIS_ D3DVALUE* r, D3DVALUE* g, D3DVALUE* b) PURE;
    STDMETHOD(SetAmbient)(THIS_ D3DVALUE r, D3DVALUE g, D3DVALUE b) PURE;
};


#undef INTERFACE
#define INTERFACE IDirect3DRMAnimation

DECLARE_INTERFACE_(IDirect3DRMAnimation, IDirect3DRMObject)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMAnimation methods
     */
    STDMETHOD(SetOptions)(THIS_ D3DRMANIMATIONOPTIONS flags) PURE;
    STDMETHOD(AddRotateKey)(THIS_ D3DVALUE time, D3DRMQUATERNION *q) PURE;
    STDMETHOD(AddPositionKey)(THIS_ D3DVALUE time, D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD(AddScaleKey)(THIS_ D3DVALUE time, D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD(DeleteKey)(THIS_ D3DVALUE time) PURE;
    STDMETHOD(SetFrame)(THIS_ LPDIRECT3DRMFRAME frame) PURE;
    STDMETHOD(SetTime)(THIS_ D3DVALUE time) PURE;

    STDMETHOD_(D3DRMANIMATIONOPTIONS, GetOptions)(THIS) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMAnimation2

DECLARE_INTERFACE_(IDirect3DRMAnimation2, IDirect3DRMObject)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMAnimation2 methods
     */
    STDMETHOD(SetOptions)(THIS_ D3DRMANIMATIONOPTIONS flags) PURE;
    STDMETHOD(AddRotateKey)(THIS_ D3DVALUE time, D3DRMQUATERNION *q) PURE;
    STDMETHOD(AddPositionKey)(THIS_ D3DVALUE time, D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD(AddScaleKey)(THIS_ D3DVALUE time, D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD(DeleteKey)(THIS_ D3DVALUE time) PURE;
    STDMETHOD(SetFrame)(THIS_ LPDIRECT3DRMFRAME3 frame) PURE;
    STDMETHOD(SetTime)(THIS_ D3DVALUE time) PURE;

    STDMETHOD_(D3DRMANIMATIONOPTIONS, GetOptions)(THIS) PURE;
    STDMETHOD(GetFrame)(THIS_ LPDIRECT3DRMFRAME3 *lpD3DFrame) PURE;
    STDMETHOD(DeleteKeyByID)(THIS_ DWORD dwID) PURE;
    STDMETHOD(AddKey)(THIS_ LPD3DRMANIMATIONKEY lpKey) PURE;
    STDMETHOD(ModifyKey)(THIS_ LPD3DRMANIMATIONKEY lpKey) PURE;
    STDMETHOD(GetKeys)(THIS_ D3DVALUE dvTimeMin,
                       D3DVALUE dvTimeMax, LPDWORD lpdwNumKeys,
                       LPD3DRMANIMATIONKEY lpKey);
};

#undef INTERFACE
#define INTERFACE IDirect3DRMAnimationSet

DECLARE_INTERFACE_(IDirect3DRMAnimationSet, IDirect3DRMObject)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMAnimationSet methods
     */
    STDMETHOD(AddAnimation)(THIS_ LPDIRECT3DRMANIMATION aid) PURE;
    STDMETHOD(Load)(THIS_ LPVOID filename, LPVOID name, D3DRMLOADOPTIONS loadflags, D3DRMLOADTEXTURECALLBACK, LPVOID lpArg, LPDIRECT3DRMFRAME parent)PURE;
    STDMETHOD(DeleteAnimation)(THIS_ LPDIRECT3DRMANIMATION aid) PURE;
    STDMETHOD(SetTime)(THIS_ D3DVALUE time) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMAnimationSet2

DECLARE_INTERFACE_(IDirect3DRMAnimationSet2, IDirect3DRMObject)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMAnimationSet2 methods
     */
    STDMETHOD(AddAnimation)(THIS_ LPDIRECT3DRMANIMATION2 aid) PURE;
    STDMETHOD(Load)(THIS_ LPVOID filename, LPVOID name, D3DRMLOADOPTIONS loadflags, D3DRMLOADTEXTURE3CALLBACK, LPVOID lpArg, LPDIRECT3DRMFRAME3 parent)PURE;
    STDMETHOD(DeleteAnimation)(THIS_ LPDIRECT3DRMANIMATION2 aid) PURE;
    STDMETHOD(SetTime)(THIS_ D3DVALUE time) PURE;
    STDMETHOD(GetAnimations)(THIS_ LPDIRECT3DRMANIMATIONARRAY *) PURE;
};


#undef INTERFACE
#define INTERFACE IDirect3DRMUserVisual

DECLARE_INTERFACE_(IDirect3DRMUserVisual, IDirect3DRMVisual)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMUserVisual methods
     */
    STDMETHOD(Init)(THIS_ D3DRMUSERVISUALCALLBACK fn, void *arg) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMArray

DECLARE_INTERFACE_(IDirect3DRMArray, IUnknown)
{
    IUNKNOWN_METHODS(PURE);

    STDMETHOD_(DWORD, GetSize)(THIS) PURE;
    /* No GetElement method as it would get overloaded
     * in derived classes, and overloading is
     * a no-no in COM
     */
};

#undef INTERFACE
#define INTERFACE IDirect3DRMObjectArray

DECLARE_INTERFACE_(IDirect3DRMObjectArray, IDirect3DRMArray)
{
    IUNKNOWN_METHODS(PURE);

    STDMETHOD_(DWORD, GetSize)(THIS) PURE;
    STDMETHOD(GetElement)(THIS_ DWORD index, LPDIRECT3DRMOBJECT *) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMDeviceArray

DECLARE_INTERFACE_(IDirect3DRMDeviceArray, IDirect3DRMArray)
{
    IUNKNOWN_METHODS(PURE);

    STDMETHOD_(DWORD, GetSize)(THIS) PURE;
    STDMETHOD(GetElement)(THIS_ DWORD index, LPDIRECT3DRMDEVICE *) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMFrameArray

DECLARE_INTERFACE_(IDirect3DRMFrameArray, IDirect3DRMArray)
{
    IUNKNOWN_METHODS(PURE);

    STDMETHOD_(DWORD, GetSize)(THIS) PURE;
    STDMETHOD(GetElement)(THIS_ DWORD index, LPDIRECT3DRMFRAME *) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMViewportArray

DECLARE_INTERFACE_(IDirect3DRMViewportArray, IDirect3DRMArray)
{
    IUNKNOWN_METHODS(PURE);

    STDMETHOD_(DWORD, GetSize)(THIS) PURE;
    STDMETHOD(GetElement)(THIS_ DWORD index, LPDIRECT3DRMVIEWPORT *) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMVisualArray

DECLARE_INTERFACE_(IDirect3DRMVisualArray, IDirect3DRMArray)
{
    IUNKNOWN_METHODS(PURE);

    STDMETHOD_(DWORD, GetSize)(THIS) PURE;
    STDMETHOD(GetElement)(THIS_ DWORD index, LPDIRECT3DRMVISUAL *) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMAnimationArray

DECLARE_INTERFACE_(IDirect3DRMAnimationArray, IDirect3DRMArray)
{
    IUNKNOWN_METHODS(PURE);

    STDMETHOD_(DWORD, GetSize)(THIS) PURE;
    STDMETHOD(GetElement)(THIS_ DWORD index, LPDIRECT3DRMANIMATION2 *) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMPickedArray

DECLARE_INTERFACE_(IDirect3DRMPickedArray, IDirect3DRMArray)
{
    IUNKNOWN_METHODS(PURE);

    STDMETHOD_(DWORD, GetSize)(THIS) PURE;
    STDMETHOD(GetPick)(THIS_ DWORD index, LPDIRECT3DRMVISUAL *, LPDIRECT3DRMFRAMEARRAY *, LPD3DRMPICKDESC) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMLightArray

DECLARE_INTERFACE_(IDirect3DRMLightArray, IDirect3DRMArray)
{
    IUNKNOWN_METHODS(PURE);

    STDMETHOD_(DWORD, GetSize)(THIS) PURE;
    STDMETHOD(GetElement)(THIS_ DWORD index, LPDIRECT3DRMLIGHT *) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMFaceArray

DECLARE_INTERFACE_(IDirect3DRMFaceArray, IDirect3DRMArray)
{
    IUNKNOWN_METHODS(PURE);

    STDMETHOD_(DWORD, GetSize)(THIS) PURE;
    STDMETHOD(GetElement)(THIS_ DWORD index, LPDIRECT3DRMFACE *) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMPicked2Array

DECLARE_INTERFACE_(IDirect3DRMPicked2Array, IDirect3DRMArray)
{
    IUNKNOWN_METHODS(PURE);

    STDMETHOD_(DWORD, GetSize)(THIS) PURE;
    STDMETHOD(GetPick)(THIS_ DWORD index, LPDIRECT3DRMVISUAL *, LPDIRECT3DRMFRAMEARRAY *, LPD3DRMPICKDESC2) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMInterpolator

DECLARE_INTERFACE_(IDirect3DRMInterpolator, IDirect3DRMObject)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMInterpolator methods
     */
    STDMETHOD(AttachObject)(THIS_ LPDIRECT3DRMOBJECT) PURE;
    STDMETHOD(GetAttachedObjects)(THIS_ LPDIRECT3DRMOBJECTARRAY *) PURE;
    STDMETHOD(DetachObject)(THIS_ LPDIRECT3DRMOBJECT) PURE;
    STDMETHOD(SetIndex)(THIS_ D3DVALUE) PURE;
    STDMETHOD_(D3DVALUE, GetIndex)(THIS) PURE;
    STDMETHOD(Interpolate)(THIS_ D3DVALUE, LPDIRECT3DRMOBJECT, D3DRMINTERPOLATIONOPTIONS) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DRMClippedVisual

DECLARE_INTERFACE_(IDirect3DRMClippedVisual, IDirect3DRMVisual)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMClippedVisual methods
     */
    STDMETHOD(Init) (THIS_ LPDIRECT3DRMVISUAL) PURE;
    STDMETHOD(AddPlane) (THIS_ LPDIRECT3DRMFRAME3, LPD3DVECTOR, LPD3DVECTOR, DWORD, LPDWORD) PURE;
    STDMETHOD(DeletePlane)(THIS_ DWORD, DWORD) PURE;
    STDMETHOD(GetPlaneIDs)(THIS_ LPDWORD, LPDWORD, DWORD) PURE;
    STDMETHOD(GetPlane) (THIS_ DWORD, LPDIRECT3DRMFRAME3, LPD3DVECTOR, LPD3DVECTOR, DWORD) PURE;
    STDMETHOD(SetPlane) (THIS_ DWORD, LPDIRECT3DRMFRAME3, LPD3DVECTOR, LPD3DVECTOR, DWORD) PURE;
};

#ifdef __cplusplus
};
#endif
#endif /* _D3DRMOBJ_H_ */

