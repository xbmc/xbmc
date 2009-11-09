
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0357 */
/* Compiler settings for dxtrans.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __dxtrans_h__
#define __dxtrans_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDXBaseObject_FWD_DEFINED__
#define __IDXBaseObject_FWD_DEFINED__
typedef interface IDXBaseObject IDXBaseObject;
#endif 	/* __IDXBaseObject_FWD_DEFINED__ */


#ifndef __IDXTransformFactory_FWD_DEFINED__
#define __IDXTransformFactory_FWD_DEFINED__
typedef interface IDXTransformFactory IDXTransformFactory;
#endif 	/* __IDXTransformFactory_FWD_DEFINED__ */


#ifndef __IDXTransform_FWD_DEFINED__
#define __IDXTransform_FWD_DEFINED__
typedef interface IDXTransform IDXTransform;
#endif 	/* __IDXTransform_FWD_DEFINED__ */


#ifndef __IDXSurfacePick_FWD_DEFINED__
#define __IDXSurfacePick_FWD_DEFINED__
typedef interface IDXSurfacePick IDXSurfacePick;
#endif 	/* __IDXSurfacePick_FWD_DEFINED__ */


#ifndef __IDXTBindHost_FWD_DEFINED__
#define __IDXTBindHost_FWD_DEFINED__
typedef interface IDXTBindHost IDXTBindHost;
#endif 	/* __IDXTBindHost_FWD_DEFINED__ */


#ifndef __IDXTaskManager_FWD_DEFINED__
#define __IDXTaskManager_FWD_DEFINED__
typedef interface IDXTaskManager IDXTaskManager;
#endif 	/* __IDXTaskManager_FWD_DEFINED__ */


#ifndef __IDXSurfaceFactory_FWD_DEFINED__
#define __IDXSurfaceFactory_FWD_DEFINED__
typedef interface IDXSurfaceFactory IDXSurfaceFactory;
#endif 	/* __IDXSurfaceFactory_FWD_DEFINED__ */


#ifndef __IDXSurfaceModifier_FWD_DEFINED__
#define __IDXSurfaceModifier_FWD_DEFINED__
typedef interface IDXSurfaceModifier IDXSurfaceModifier;
#endif 	/* __IDXSurfaceModifier_FWD_DEFINED__ */


#ifndef __IDXSurface_FWD_DEFINED__
#define __IDXSurface_FWD_DEFINED__
typedef interface IDXSurface IDXSurface;
#endif 	/* __IDXSurface_FWD_DEFINED__ */


#ifndef __IDXSurfaceInit_FWD_DEFINED__
#define __IDXSurfaceInit_FWD_DEFINED__
typedef interface IDXSurfaceInit IDXSurfaceInit;
#endif 	/* __IDXSurfaceInit_FWD_DEFINED__ */


#ifndef __IDXARGBSurfaceInit_FWD_DEFINED__
#define __IDXARGBSurfaceInit_FWD_DEFINED__
typedef interface IDXARGBSurfaceInit IDXARGBSurfaceInit;
#endif 	/* __IDXARGBSurfaceInit_FWD_DEFINED__ */


#ifndef __IDXARGBReadPtr_FWD_DEFINED__
#define __IDXARGBReadPtr_FWD_DEFINED__
typedef interface IDXARGBReadPtr IDXARGBReadPtr;
#endif 	/* __IDXARGBReadPtr_FWD_DEFINED__ */


#ifndef __IDXARGBReadWritePtr_FWD_DEFINED__
#define __IDXARGBReadWritePtr_FWD_DEFINED__
typedef interface IDXARGBReadWritePtr IDXARGBReadWritePtr;
#endif 	/* __IDXARGBReadWritePtr_FWD_DEFINED__ */


#ifndef __IDXDCLock_FWD_DEFINED__
#define __IDXDCLock_FWD_DEFINED__
typedef interface IDXDCLock IDXDCLock;
#endif 	/* __IDXDCLock_FWD_DEFINED__ */


#ifndef __IDXTScaleOutput_FWD_DEFINED__
#define __IDXTScaleOutput_FWD_DEFINED__
typedef interface IDXTScaleOutput IDXTScaleOutput;
#endif 	/* __IDXTScaleOutput_FWD_DEFINED__ */


#ifndef __IDXGradient_FWD_DEFINED__
#define __IDXGradient_FWD_DEFINED__
typedef interface IDXGradient IDXGradient;
#endif 	/* __IDXGradient_FWD_DEFINED__ */


#ifndef __IDXTScale_FWD_DEFINED__
#define __IDXTScale_FWD_DEFINED__
typedef interface IDXTScale IDXTScale;
#endif 	/* __IDXTScale_FWD_DEFINED__ */


#ifndef __IDXEffect_FWD_DEFINED__
#define __IDXEffect_FWD_DEFINED__
typedef interface IDXEffect IDXEffect;
#endif 	/* __IDXEffect_FWD_DEFINED__ */


#ifndef __IDXLookupTable_FWD_DEFINED__
#define __IDXLookupTable_FWD_DEFINED__
typedef interface IDXLookupTable IDXLookupTable;
#endif 	/* __IDXLookupTable_FWD_DEFINED__ */


#ifndef __IDXRawSurface_FWD_DEFINED__
#define __IDXRawSurface_FWD_DEFINED__
typedef interface IDXRawSurface IDXRawSurface;
#endif 	/* __IDXRawSurface_FWD_DEFINED__ */


#ifndef __IHTMLDXTransform_FWD_DEFINED__
#define __IHTMLDXTransform_FWD_DEFINED__
typedef interface IHTMLDXTransform IHTMLDXTransform;
#endif 	/* __IHTMLDXTransform_FWD_DEFINED__ */


#ifndef __ICSSFilterDispatch_FWD_DEFINED__
#define __ICSSFilterDispatch_FWD_DEFINED__
typedef interface ICSSFilterDispatch ICSSFilterDispatch;
#endif 	/* __ICSSFilterDispatch_FWD_DEFINED__ */


#ifndef __DXTransformFactory_FWD_DEFINED__
#define __DXTransformFactory_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTransformFactory DXTransformFactory;
#else
typedef struct DXTransformFactory DXTransformFactory;
#endif /* __cplusplus */

#endif 	/* __DXTransformFactory_FWD_DEFINED__ */


#ifndef __DXTaskManager_FWD_DEFINED__
#define __DXTaskManager_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTaskManager DXTaskManager;
#else
typedef struct DXTaskManager DXTaskManager;
#endif /* __cplusplus */

#endif 	/* __DXTaskManager_FWD_DEFINED__ */


#ifndef __DXTScale_FWD_DEFINED__
#define __DXTScale_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTScale DXTScale;
#else
typedef struct DXTScale DXTScale;
#endif /* __cplusplus */

#endif 	/* __DXTScale_FWD_DEFINED__ */


#ifndef __DXSurface_FWD_DEFINED__
#define __DXSurface_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXSurface DXSurface;
#else
typedef struct DXSurface DXSurface;
#endif /* __cplusplus */

#endif 	/* __DXSurface_FWD_DEFINED__ */


#ifndef __DXSurfaceModifier_FWD_DEFINED__
#define __DXSurfaceModifier_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXSurfaceModifier DXSurfaceModifier;
#else
typedef struct DXSurfaceModifier DXSurfaceModifier;
#endif /* __cplusplus */

#endif 	/* __DXSurfaceModifier_FWD_DEFINED__ */


#ifndef __DXGradient_FWD_DEFINED__
#define __DXGradient_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXGradient DXGradient;
#else
typedef struct DXGradient DXGradient;
#endif /* __cplusplus */

#endif 	/* __DXGradient_FWD_DEFINED__ */


#ifndef __DXTFilter_FWD_DEFINED__
#define __DXTFilter_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTFilter DXTFilter;
#else
typedef struct DXTFilter DXTFilter;
#endif /* __cplusplus */

#endif 	/* __DXTFilter_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "comcat.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_dxtrans_0000 */
/* [local] */ 

#include <servprov.h>
#include <ddraw.h>
#include <d3d.h>
#include <d3drm.h>
#include <urlmon.h>
#if 0
// Bogus definition used to make MIDL compiler happy
typedef void DDSURFACEDESC;

typedef void D3DRMBOX;

typedef void D3DVECTOR;

typedef void D3DRMMATRIX4D;

typedef void *LPSECURITY_ATTRIBUTES;

#endif
#ifdef _DXTRANSIMPL
    #define _DXTRANS_IMPL_EXT _declspec(dllexport)
#else
    #define _DXTRANS_IMPL_EXT _declspec(dllimport)
#endif
















//
//   All GUIDs for DXTransform are declared in DXTGUID.C in the SDK include directory
//
EXTERN_C const GUID DDPF_RGB1;
EXTERN_C const GUID DDPF_RGB2;
EXTERN_C const GUID DDPF_RGB4;
EXTERN_C const GUID DDPF_RGB8;
EXTERN_C const GUID DDPF_RGB332;
EXTERN_C const GUID DDPF_ARGB4444;
EXTERN_C const GUID DDPF_RGB565;
EXTERN_C const GUID DDPF_BGR565;
EXTERN_C const GUID DDPF_RGB555;
EXTERN_C const GUID DDPF_ARGB1555;
EXTERN_C const GUID DDPF_RGB24;
EXTERN_C const GUID DDPF_BGR24;
EXTERN_C const GUID DDPF_RGB32;
EXTERN_C const GUID DDPF_BGR32;
EXTERN_C const GUID DDPF_ABGR32;
EXTERN_C const GUID DDPF_ARGB32;
EXTERN_C const GUID DDPF_PMARGB32;
EXTERN_C const GUID DDPF_A1;
EXTERN_C const GUID DDPF_A2;
EXTERN_C const GUID DDPF_A4;
EXTERN_C const GUID DDPF_A8;
EXTERN_C const GUID DDPF_Z8;
EXTERN_C const GUID DDPF_Z16;
EXTERN_C const GUID DDPF_Z24;
EXTERN_C const GUID DDPF_Z32;
//
//   Component categories
//
EXTERN_C const GUID CATID_DXImageTransform;
EXTERN_C const GUID CATID_DX3DTransform;
EXTERN_C const GUID CATID_DXAuthoringTransform;
EXTERN_C const GUID CATID_DXSurface;
//
//   Service IDs
//
EXTERN_C const GUID SID_SDirectDraw;
EXTERN_C const GUID SID_SDirect3DRM;
#define SID_SDXTaskManager CLSID_DXTaskManager
#define SID_SDXSurfaceFactory IID_IDXSurfaceFactory
#define SID_SDXTransformFactory IID_IDXTransformFactory
//
//   DXTransforms Core Type Library Version Info
//
#define DXTRANS_TLB_MAJOR_VER 1
#define DXTRANS_TLB_MINOR_VER 1


extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0000_v0_0_s_ifspec;

#ifndef __IDXBaseObject_INTERFACE_DEFINED__
#define __IDXBaseObject_INTERFACE_DEFINED__

/* interface IDXBaseObject */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXBaseObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("17B59B2B-9CC8-11d1-9053-00C04FD9189D")
    IDXBaseObject : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetGenerationId( 
            /* [out] */ ULONG *pID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IncrementGenerationId( 
            /* [in] */ BOOL bRefresh) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetObjectSize( 
            /* [out] */ ULONG *pcbSize) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXBaseObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXBaseObject * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXBaseObject * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXBaseObject * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetGenerationId )( 
            IDXBaseObject * This,
            /* [out] */ ULONG *pID);
        
        HRESULT ( STDMETHODCALLTYPE *IncrementGenerationId )( 
            IDXBaseObject * This,
            /* [in] */ BOOL bRefresh);
        
        HRESULT ( STDMETHODCALLTYPE *GetObjectSize )( 
            IDXBaseObject * This,
            /* [out] */ ULONG *pcbSize);
        
        END_INTERFACE
    } IDXBaseObjectVtbl;

    interface IDXBaseObject
    {
        CONST_VTBL struct IDXBaseObjectVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXBaseObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXBaseObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXBaseObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXBaseObject_GetGenerationId(This,pID)	\
    (This)->lpVtbl -> GetGenerationId(This,pID)

#define IDXBaseObject_IncrementGenerationId(This,bRefresh)	\
    (This)->lpVtbl -> IncrementGenerationId(This,bRefresh)

#define IDXBaseObject_GetObjectSize(This,pcbSize)	\
    (This)->lpVtbl -> GetObjectSize(This,pcbSize)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXBaseObject_GetGenerationId_Proxy( 
    IDXBaseObject * This,
    /* [out] */ ULONG *pID);


void __RPC_STUB IDXBaseObject_GetGenerationId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXBaseObject_IncrementGenerationId_Proxy( 
    IDXBaseObject * This,
    /* [in] */ BOOL bRefresh);


void __RPC_STUB IDXBaseObject_IncrementGenerationId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXBaseObject_GetObjectSize_Proxy( 
    IDXBaseObject * This,
    /* [out] */ ULONG *pcbSize);


void __RPC_STUB IDXBaseObject_GetObjectSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXBaseObject_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtrans_0260 */
/* [local] */ 

typedef 
enum DXBNDID
    {	DXB_X	= 0,
	DXB_Y	= 1,
	DXB_Z	= 2,
	DXB_T	= 3
    } 	DXBNDID;

typedef 
enum DXBNDTYPE
    {	DXBT_DISCRETE	= 0,
	DXBT_DISCRETE64	= DXBT_DISCRETE + 1,
	DXBT_CONTINUOUS	= DXBT_DISCRETE64 + 1,
	DXBT_CONTINUOUS64	= DXBT_CONTINUOUS + 1
    } 	DXBNDTYPE;

typedef struct DXDBND
    {
    long Min;
    long Max;
    } 	DXDBND;

typedef DXDBND DXDBNDS[ 4 ];

typedef struct DXDBND64
    {
    LONGLONG Min;
    LONGLONG Max;
    } 	DXDBND64;

typedef DXDBND64 DXDBNDS64[ 4 ];

typedef struct DXCBND
    {
    float Min;
    float Max;
    } 	DXCBND;

typedef DXCBND DXCBNDS[ 4 ];

typedef struct DXCBND64
    {
    double Min;
    double Max;
    } 	DXCBND64;

typedef DXCBND64 DXCBNDS64[ 4 ];

typedef struct DXBNDS
    {
    DXBNDTYPE eType;
    /* [switch_is] */ /* [switch_type] */ union __MIDL___MIDL_itf_dxtrans_0260_0001
        {
        /* [case()] */ DXDBND D[ 4 ];
        /* [case()] */ DXDBND64 LD[ 4 ];
        /* [case()] */ DXCBND C[ 4 ];
        /* [case()] */ DXCBND64 LC[ 4 ];
        } 	u;
    } 	DXBNDS;

typedef long DXDVEC[ 4 ];

typedef LONGLONG DXDVEC64[ 4 ];

typedef float DXCVEC[ 4 ];

typedef double DXCVEC64[ 4 ];

typedef struct DXVEC
    {
    DXBNDTYPE eType;
    /* [switch_is] */ /* [switch_type] */ union __MIDL___MIDL_itf_dxtrans_0260_0002
        {
        /* [case()] */ long D[ 4 ];
        /* [case()] */ LONGLONG LD[ 4 ];
        /* [case()] */ float C[ 4 ];
        /* [case()] */ double LC[ 4 ];
        } 	u;
    } 	DXVEC;



extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0260_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0260_v0_0_s_ifspec;

#ifndef __IDXTransformFactory_INTERFACE_DEFINED__
#define __IDXTransformFactory_INTERFACE_DEFINED__

/* interface IDXTransformFactory */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXTransformFactory;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6A950B2B-A971-11d1-81C8-0000F87557DB")
    IDXTransformFactory : public IServiceProvider
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetService( 
            /* [in] */ REFGUID guidService,
            /* [in] */ IUnknown *pUnkService,
            /* [in] */ BOOL bWeakReference) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateTransform( 
            /* [size_is][in] */ IUnknown **punkInputs,
            /* [in] */ ULONG ulNumInputs,
            /* [size_is][in] */ IUnknown **punkOutputs,
            /* [in] */ ULONG ulNumOutputs,
            /* [in] */ IPropertyBag *pInitProps,
            /* [in] */ IErrorLog *pErrLog,
            /* [in] */ REFCLSID TransCLSID,
            /* [in] */ REFIID TransIID,
            /* [iid_is][out] */ void **ppTransform) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InitializeTransform( 
            /* [in] */ IDXTransform *pTransform,
            /* [size_is][in] */ IUnknown **punkInputs,
            /* [in] */ ULONG ulNumInputs,
            /* [size_is][in] */ IUnknown **punkOutputs,
            /* [in] */ ULONG ulNumOutputs,
            /* [in] */ IPropertyBag *pInitProps,
            /* [in] */ IErrorLog *pErrLog) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTransformFactoryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTransformFactory * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTransformFactory * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTransformFactory * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *QueryService )( 
            IDXTransformFactory * This,
            /* [in] */ REFGUID guidService,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppvObject);
        
        HRESULT ( STDMETHODCALLTYPE *SetService )( 
            IDXTransformFactory * This,
            /* [in] */ REFGUID guidService,
            /* [in] */ IUnknown *pUnkService,
            /* [in] */ BOOL bWeakReference);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTransform )( 
            IDXTransformFactory * This,
            /* [size_is][in] */ IUnknown **punkInputs,
            /* [in] */ ULONG ulNumInputs,
            /* [size_is][in] */ IUnknown **punkOutputs,
            /* [in] */ ULONG ulNumOutputs,
            /* [in] */ IPropertyBag *pInitProps,
            /* [in] */ IErrorLog *pErrLog,
            /* [in] */ REFCLSID TransCLSID,
            /* [in] */ REFIID TransIID,
            /* [iid_is][out] */ void **ppTransform);
        
        HRESULT ( STDMETHODCALLTYPE *InitializeTransform )( 
            IDXTransformFactory * This,
            /* [in] */ IDXTransform *pTransform,
            /* [size_is][in] */ IUnknown **punkInputs,
            /* [in] */ ULONG ulNumInputs,
            /* [size_is][in] */ IUnknown **punkOutputs,
            /* [in] */ ULONG ulNumOutputs,
            /* [in] */ IPropertyBag *pInitProps,
            /* [in] */ IErrorLog *pErrLog);
        
        END_INTERFACE
    } IDXTransformFactoryVtbl;

    interface IDXTransformFactory
    {
        CONST_VTBL struct IDXTransformFactoryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTransformFactory_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTransformFactory_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTransformFactory_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTransformFactory_QueryService(This,guidService,riid,ppvObject)	\
    (This)->lpVtbl -> QueryService(This,guidService,riid,ppvObject)


#define IDXTransformFactory_SetService(This,guidService,pUnkService,bWeakReference)	\
    (This)->lpVtbl -> SetService(This,guidService,pUnkService,bWeakReference)

#define IDXTransformFactory_CreateTransform(This,punkInputs,ulNumInputs,punkOutputs,ulNumOutputs,pInitProps,pErrLog,TransCLSID,TransIID,ppTransform)	\
    (This)->lpVtbl -> CreateTransform(This,punkInputs,ulNumInputs,punkOutputs,ulNumOutputs,pInitProps,pErrLog,TransCLSID,TransIID,ppTransform)

#define IDXTransformFactory_InitializeTransform(This,pTransform,punkInputs,ulNumInputs,punkOutputs,ulNumOutputs,pInitProps,pErrLog)	\
    (This)->lpVtbl -> InitializeTransform(This,pTransform,punkInputs,ulNumInputs,punkOutputs,ulNumOutputs,pInitProps,pErrLog)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXTransformFactory_SetService_Proxy( 
    IDXTransformFactory * This,
    /* [in] */ REFGUID guidService,
    /* [in] */ IUnknown *pUnkService,
    /* [in] */ BOOL bWeakReference);


void __RPC_STUB IDXTransformFactory_SetService_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTransformFactory_CreateTransform_Proxy( 
    IDXTransformFactory * This,
    /* [size_is][in] */ IUnknown **punkInputs,
    /* [in] */ ULONG ulNumInputs,
    /* [size_is][in] */ IUnknown **punkOutputs,
    /* [in] */ ULONG ulNumOutputs,
    /* [in] */ IPropertyBag *pInitProps,
    /* [in] */ IErrorLog *pErrLog,
    /* [in] */ REFCLSID TransCLSID,
    /* [in] */ REFIID TransIID,
    /* [iid_is][out] */ void **ppTransform);


void __RPC_STUB IDXTransformFactory_CreateTransform_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTransformFactory_InitializeTransform_Proxy( 
    IDXTransformFactory * This,
    /* [in] */ IDXTransform *pTransform,
    /* [size_is][in] */ IUnknown **punkInputs,
    /* [in] */ ULONG ulNumInputs,
    /* [size_is][in] */ IUnknown **punkOutputs,
    /* [in] */ ULONG ulNumOutputs,
    /* [in] */ IPropertyBag *pInitProps,
    /* [in] */ IErrorLog *pErrLog);


void __RPC_STUB IDXTransformFactory_InitializeTransform_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTransformFactory_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtrans_0261 */
/* [local] */ 

typedef 
enum DXTMISCFLAGS
    {	DXTMF_BLEND_WITH_OUTPUT	= 1L << 0,
	DXTMF_DITHER_OUTPUT	= 1L << 1,
	DXTMF_OPTION_MASK	= 0xffff,
	DXTMF_VALID_OPTIONS	= DXTMF_BLEND_WITH_OUTPUT | DXTMF_DITHER_OUTPUT,
	DXTMF_BLEND_SUPPORTED	= 1L << 16,
	DXTMF_DITHER_SUPPORTED	= 1L << 17,
	DXTMF_INPLACE_OPERATION	= 1L << 24,
	DXTMF_BOUNDS_SUPPORTED	= 1L << 25,
	DXTMF_PLACEMENT_SUPPORTED	= 1L << 26,
	DXTMF_QUALITY_SUPPORTED	= 1L << 27,
	DXTMF_OPAQUE_RESULT	= 1L << 28
    } 	DXTMISCFLAGS;

typedef 
enum DXINOUTINFOFLAGS
    {	DXINOUTF_OPTIONAL	= 1L << 0
    } 	DXINOUTINFOFLAGS;



extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0261_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0261_v0_0_s_ifspec;

#ifndef __IDXTransform_INTERFACE_DEFINED__
#define __IDXTransform_INTERFACE_DEFINED__

/* interface IDXTransform */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXTransform;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("30A5FB78-E11F-11d1-9064-00C04FD9189D")
    IDXTransform : public IDXBaseObject
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Setup( 
            /* [size_is][in] */ IUnknown *const *punkInputs,
            /* [in] */ ULONG ulNumInputs,
            /* [size_is][in] */ IUnknown *const *punkOutputs,
            /* [in] */ ULONG ulNumOutputs,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Execute( 
            /* [in] */ const GUID *pRequestID,
            /* [in] */ const DXBNDS *pClipBnds,
            /* [in] */ const DXVEC *pPlacement) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MapBoundsIn2Out( 
            /* [in] */ const DXBNDS *pInBounds,
            /* [in] */ ULONG ulNumInBnds,
            /* [in] */ ULONG ulOutIndex,
            /* [out] */ DXBNDS *pOutBounds) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MapBoundsOut2In( 
            /* [in] */ ULONG ulOutIndex,
            /* [in] */ const DXBNDS *pOutBounds,
            /* [in] */ ULONG ulInIndex,
            /* [out] */ DXBNDS *pInBounds) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMiscFlags( 
            /* [in] */ DWORD dwMiscFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMiscFlags( 
            /* [out] */ DWORD *pdwMiscFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInOutInfo( 
            /* [in] */ BOOL bIsOutput,
            /* [in] */ ULONG ulIndex,
            /* [out] */ DWORD *pdwFlags,
            /* [size_is][out] */ GUID *pIDs,
            /* [out][in] */ ULONG *pcIDs,
            /* [out] */ IUnknown **ppUnkCurrentObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetQuality( 
            /* [in] */ float fQuality) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetQuality( 
            /* [out] */ float *fQuality) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTransformVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTransform * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTransform * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTransform * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetGenerationId )( 
            IDXTransform * This,
            /* [out] */ ULONG *pID);
        
        HRESULT ( STDMETHODCALLTYPE *IncrementGenerationId )( 
            IDXTransform * This,
            /* [in] */ BOOL bRefresh);
        
        HRESULT ( STDMETHODCALLTYPE *GetObjectSize )( 
            IDXTransform * This,
            /* [out] */ ULONG *pcbSize);
        
        HRESULT ( STDMETHODCALLTYPE *Setup )( 
            IDXTransform * This,
            /* [size_is][in] */ IUnknown *const *punkInputs,
            /* [in] */ ULONG ulNumInputs,
            /* [size_is][in] */ IUnknown *const *punkOutputs,
            /* [in] */ ULONG ulNumOutputs,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Execute )( 
            IDXTransform * This,
            /* [in] */ const GUID *pRequestID,
            /* [in] */ const DXBNDS *pClipBnds,
            /* [in] */ const DXVEC *pPlacement);
        
        HRESULT ( STDMETHODCALLTYPE *MapBoundsIn2Out )( 
            IDXTransform * This,
            /* [in] */ const DXBNDS *pInBounds,
            /* [in] */ ULONG ulNumInBnds,
            /* [in] */ ULONG ulOutIndex,
            /* [out] */ DXBNDS *pOutBounds);
        
        HRESULT ( STDMETHODCALLTYPE *MapBoundsOut2In )( 
            IDXTransform * This,
            /* [in] */ ULONG ulOutIndex,
            /* [in] */ const DXBNDS *pOutBounds,
            /* [in] */ ULONG ulInIndex,
            /* [out] */ DXBNDS *pInBounds);
        
        HRESULT ( STDMETHODCALLTYPE *SetMiscFlags )( 
            IDXTransform * This,
            /* [in] */ DWORD dwMiscFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetMiscFlags )( 
            IDXTransform * This,
            /* [out] */ DWORD *pdwMiscFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetInOutInfo )( 
            IDXTransform * This,
            /* [in] */ BOOL bIsOutput,
            /* [in] */ ULONG ulIndex,
            /* [out] */ DWORD *pdwFlags,
            /* [size_is][out] */ GUID *pIDs,
            /* [out][in] */ ULONG *pcIDs,
            /* [out] */ IUnknown **ppUnkCurrentObject);
        
        HRESULT ( STDMETHODCALLTYPE *SetQuality )( 
            IDXTransform * This,
            /* [in] */ float fQuality);
        
        HRESULT ( STDMETHODCALLTYPE *GetQuality )( 
            IDXTransform * This,
            /* [out] */ float *fQuality);
        
        END_INTERFACE
    } IDXTransformVtbl;

    interface IDXTransform
    {
        CONST_VTBL struct IDXTransformVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTransform_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTransform_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTransform_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTransform_GetGenerationId(This,pID)	\
    (This)->lpVtbl -> GetGenerationId(This,pID)

#define IDXTransform_IncrementGenerationId(This,bRefresh)	\
    (This)->lpVtbl -> IncrementGenerationId(This,bRefresh)

#define IDXTransform_GetObjectSize(This,pcbSize)	\
    (This)->lpVtbl -> GetObjectSize(This,pcbSize)


#define IDXTransform_Setup(This,punkInputs,ulNumInputs,punkOutputs,ulNumOutputs,dwFlags)	\
    (This)->lpVtbl -> Setup(This,punkInputs,ulNumInputs,punkOutputs,ulNumOutputs,dwFlags)

#define IDXTransform_Execute(This,pRequestID,pClipBnds,pPlacement)	\
    (This)->lpVtbl -> Execute(This,pRequestID,pClipBnds,pPlacement)

#define IDXTransform_MapBoundsIn2Out(This,pInBounds,ulNumInBnds,ulOutIndex,pOutBounds)	\
    (This)->lpVtbl -> MapBoundsIn2Out(This,pInBounds,ulNumInBnds,ulOutIndex,pOutBounds)

#define IDXTransform_MapBoundsOut2In(This,ulOutIndex,pOutBounds,ulInIndex,pInBounds)	\
    (This)->lpVtbl -> MapBoundsOut2In(This,ulOutIndex,pOutBounds,ulInIndex,pInBounds)

#define IDXTransform_SetMiscFlags(This,dwMiscFlags)	\
    (This)->lpVtbl -> SetMiscFlags(This,dwMiscFlags)

#define IDXTransform_GetMiscFlags(This,pdwMiscFlags)	\
    (This)->lpVtbl -> GetMiscFlags(This,pdwMiscFlags)

#define IDXTransform_GetInOutInfo(This,bIsOutput,ulIndex,pdwFlags,pIDs,pcIDs,ppUnkCurrentObject)	\
    (This)->lpVtbl -> GetInOutInfo(This,bIsOutput,ulIndex,pdwFlags,pIDs,pcIDs,ppUnkCurrentObject)

#define IDXTransform_SetQuality(This,fQuality)	\
    (This)->lpVtbl -> SetQuality(This,fQuality)

#define IDXTransform_GetQuality(This,fQuality)	\
    (This)->lpVtbl -> GetQuality(This,fQuality)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXTransform_Setup_Proxy( 
    IDXTransform * This,
    /* [size_is][in] */ IUnknown *const *punkInputs,
    /* [in] */ ULONG ulNumInputs,
    /* [size_is][in] */ IUnknown *const *punkOutputs,
    /* [in] */ ULONG ulNumOutputs,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IDXTransform_Setup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTransform_Execute_Proxy( 
    IDXTransform * This,
    /* [in] */ const GUID *pRequestID,
    /* [in] */ const DXBNDS *pClipBnds,
    /* [in] */ const DXVEC *pPlacement);


void __RPC_STUB IDXTransform_Execute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTransform_MapBoundsIn2Out_Proxy( 
    IDXTransform * This,
    /* [in] */ const DXBNDS *pInBounds,
    /* [in] */ ULONG ulNumInBnds,
    /* [in] */ ULONG ulOutIndex,
    /* [out] */ DXBNDS *pOutBounds);


void __RPC_STUB IDXTransform_MapBoundsIn2Out_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTransform_MapBoundsOut2In_Proxy( 
    IDXTransform * This,
    /* [in] */ ULONG ulOutIndex,
    /* [in] */ const DXBNDS *pOutBounds,
    /* [in] */ ULONG ulInIndex,
    /* [out] */ DXBNDS *pInBounds);


void __RPC_STUB IDXTransform_MapBoundsOut2In_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTransform_SetMiscFlags_Proxy( 
    IDXTransform * This,
    /* [in] */ DWORD dwMiscFlags);


void __RPC_STUB IDXTransform_SetMiscFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTransform_GetMiscFlags_Proxy( 
    IDXTransform * This,
    /* [out] */ DWORD *pdwMiscFlags);


void __RPC_STUB IDXTransform_GetMiscFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTransform_GetInOutInfo_Proxy( 
    IDXTransform * This,
    /* [in] */ BOOL bIsOutput,
    /* [in] */ ULONG ulIndex,
    /* [out] */ DWORD *pdwFlags,
    /* [size_is][out] */ GUID *pIDs,
    /* [out][in] */ ULONG *pcIDs,
    /* [out] */ IUnknown **ppUnkCurrentObject);


void __RPC_STUB IDXTransform_GetInOutInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTransform_SetQuality_Proxy( 
    IDXTransform * This,
    /* [in] */ float fQuality);


void __RPC_STUB IDXTransform_SetQuality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTransform_GetQuality_Proxy( 
    IDXTransform * This,
    /* [out] */ float *fQuality);


void __RPC_STUB IDXTransform_GetQuality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTransform_INTERFACE_DEFINED__ */


#ifndef __IDXSurfacePick_INTERFACE_DEFINED__
#define __IDXSurfacePick_INTERFACE_DEFINED__

/* interface IDXSurfacePick */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXSurfacePick;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("30A5FB79-E11F-11d1-9064-00C04FD9189D")
    IDXSurfacePick : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE PointPick( 
            /* [in] */ const DXVEC *pPoint,
            /* [out] */ ULONG *pulInputSurfaceIndex,
            /* [out] */ DXVEC *pInputPoint) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXSurfacePickVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXSurfacePick * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXSurfacePick * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXSurfacePick * This);
        
        HRESULT ( STDMETHODCALLTYPE *PointPick )( 
            IDXSurfacePick * This,
            /* [in] */ const DXVEC *pPoint,
            /* [out] */ ULONG *pulInputSurfaceIndex,
            /* [out] */ DXVEC *pInputPoint);
        
        END_INTERFACE
    } IDXSurfacePickVtbl;

    interface IDXSurfacePick
    {
        CONST_VTBL struct IDXSurfacePickVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXSurfacePick_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXSurfacePick_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXSurfacePick_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXSurfacePick_PointPick(This,pPoint,pulInputSurfaceIndex,pInputPoint)	\
    (This)->lpVtbl -> PointPick(This,pPoint,pulInputSurfaceIndex,pInputPoint)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXSurfacePick_PointPick_Proxy( 
    IDXSurfacePick * This,
    /* [in] */ const DXVEC *pPoint,
    /* [out] */ ULONG *pulInputSurfaceIndex,
    /* [out] */ DXVEC *pInputPoint);


void __RPC_STUB IDXSurfacePick_PointPick_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXSurfacePick_INTERFACE_DEFINED__ */


#ifndef __IDXTBindHost_INTERFACE_DEFINED__
#define __IDXTBindHost_INTERFACE_DEFINED__

/* interface IDXTBindHost */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXTBindHost;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D26BCE55-E9DC-11d1-9066-00C04FD9189D")
    IDXTBindHost : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetBindHost( 
            /* [in] */ IBindHost *pBindHost) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTBindHostVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTBindHost * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTBindHost * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTBindHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetBindHost )( 
            IDXTBindHost * This,
            /* [in] */ IBindHost *pBindHost);
        
        END_INTERFACE
    } IDXTBindHostVtbl;

    interface IDXTBindHost
    {
        CONST_VTBL struct IDXTBindHostVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTBindHost_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTBindHost_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTBindHost_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTBindHost_SetBindHost(This,pBindHost)	\
    (This)->lpVtbl -> SetBindHost(This,pBindHost)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXTBindHost_SetBindHost_Proxy( 
    IDXTBindHost * This,
    /* [in] */ IBindHost *pBindHost);


void __RPC_STUB IDXTBindHost_SetBindHost_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTBindHost_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtrans_0264 */
/* [local] */ 

typedef void __stdcall __stdcall DXTASKPROC( 
    void *pTaskData,
    BOOL *pbContinueProcessing);

typedef DXTASKPROC *PFNDXTASKPROC;

typedef void __stdcall __stdcall DXAPCPROC( 
    DWORD dwData);

typedef DXAPCPROC *PFNDXAPCPROC;

#ifdef __cplusplus
typedef struct DXTMTASKINFO
{
    PFNDXTASKPROC pfnTaskProc;       // Pointer to function to execute
    PVOID         pTaskData;         // Pointer to argument data
    PFNDXAPCPROC  pfnCompletionAPC;  // Pointer to completion APC proc
    DWORD         dwCompletionData;  // Pointer to APC proc data
    const GUID*   pRequestID;        // Used to identify groups of tasks
} DXTMTASKINFO;
#else
typedef struct DXTMTASKINFO
    {
    PVOID pfnTaskProc;
    PVOID pTaskData;
    PVOID pfnCompletionAPC;
    DWORD dwCompletionData;
    const GUID *pRequestID;
    } 	DXTMTASKINFO;

#endif


extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0264_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0264_v0_0_s_ifspec;

#ifndef __IDXTaskManager_INTERFACE_DEFINED__
#define __IDXTaskManager_INTERFACE_DEFINED__

/* interface IDXTaskManager */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXTaskManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("254DBBC1-F922-11d0-883A-3C8B00C10000")
    IDXTaskManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE QueryNumProcessors( 
            /* [out] */ ULONG *pulNumProc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetThreadPoolSize( 
            /* [in] */ ULONG ulNumThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetThreadPoolSize( 
            /* [out] */ ULONG *pulNumThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetConcurrencyLimit( 
            /* [in] */ ULONG ulNumThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConcurrencyLimit( 
            /* [out] */ ULONG *pulNumThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ScheduleTasks( 
            /* [in] */ DXTMTASKINFO TaskInfo[  ],
            /* [in] */ HANDLE Events[  ],
            /* [out] */ DWORD TaskIDs[  ],
            /* [in] */ ULONG ulNumTasks,
            /* [in] */ ULONG ulWaitPeriod) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TerminateTasks( 
            /* [in] */ DWORD TaskIDs[  ],
            /* [in] */ ULONG ulCount,
            /* [in] */ ULONG ulTimeOut) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TerminateRequest( 
            /* [in] */ REFIID RequestID,
            /* [in] */ ULONG ulTimeOut) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTaskManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTaskManager * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTaskManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTaskManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *QueryNumProcessors )( 
            IDXTaskManager * This,
            /* [out] */ ULONG *pulNumProc);
        
        HRESULT ( STDMETHODCALLTYPE *SetThreadPoolSize )( 
            IDXTaskManager * This,
            /* [in] */ ULONG ulNumThreads);
        
        HRESULT ( STDMETHODCALLTYPE *GetThreadPoolSize )( 
            IDXTaskManager * This,
            /* [out] */ ULONG *pulNumThreads);
        
        HRESULT ( STDMETHODCALLTYPE *SetConcurrencyLimit )( 
            IDXTaskManager * This,
            /* [in] */ ULONG ulNumThreads);
        
        HRESULT ( STDMETHODCALLTYPE *GetConcurrencyLimit )( 
            IDXTaskManager * This,
            /* [out] */ ULONG *pulNumThreads);
        
        HRESULT ( STDMETHODCALLTYPE *ScheduleTasks )( 
            IDXTaskManager * This,
            /* [in] */ DXTMTASKINFO TaskInfo[  ],
            /* [in] */ HANDLE Events[  ],
            /* [out] */ DWORD TaskIDs[  ],
            /* [in] */ ULONG ulNumTasks,
            /* [in] */ ULONG ulWaitPeriod);
        
        HRESULT ( STDMETHODCALLTYPE *TerminateTasks )( 
            IDXTaskManager * This,
            /* [in] */ DWORD TaskIDs[  ],
            /* [in] */ ULONG ulCount,
            /* [in] */ ULONG ulTimeOut);
        
        HRESULT ( STDMETHODCALLTYPE *TerminateRequest )( 
            IDXTaskManager * This,
            /* [in] */ REFIID RequestID,
            /* [in] */ ULONG ulTimeOut);
        
        END_INTERFACE
    } IDXTaskManagerVtbl;

    interface IDXTaskManager
    {
        CONST_VTBL struct IDXTaskManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTaskManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTaskManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTaskManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTaskManager_QueryNumProcessors(This,pulNumProc)	\
    (This)->lpVtbl -> QueryNumProcessors(This,pulNumProc)

#define IDXTaskManager_SetThreadPoolSize(This,ulNumThreads)	\
    (This)->lpVtbl -> SetThreadPoolSize(This,ulNumThreads)

#define IDXTaskManager_GetThreadPoolSize(This,pulNumThreads)	\
    (This)->lpVtbl -> GetThreadPoolSize(This,pulNumThreads)

#define IDXTaskManager_SetConcurrencyLimit(This,ulNumThreads)	\
    (This)->lpVtbl -> SetConcurrencyLimit(This,ulNumThreads)

#define IDXTaskManager_GetConcurrencyLimit(This,pulNumThreads)	\
    (This)->lpVtbl -> GetConcurrencyLimit(This,pulNumThreads)

#define IDXTaskManager_ScheduleTasks(This,TaskInfo,Events,TaskIDs,ulNumTasks,ulWaitPeriod)	\
    (This)->lpVtbl -> ScheduleTasks(This,TaskInfo,Events,TaskIDs,ulNumTasks,ulWaitPeriod)

#define IDXTaskManager_TerminateTasks(This,TaskIDs,ulCount,ulTimeOut)	\
    (This)->lpVtbl -> TerminateTasks(This,TaskIDs,ulCount,ulTimeOut)

#define IDXTaskManager_TerminateRequest(This,RequestID,ulTimeOut)	\
    (This)->lpVtbl -> TerminateRequest(This,RequestID,ulTimeOut)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXTaskManager_QueryNumProcessors_Proxy( 
    IDXTaskManager * This,
    /* [out] */ ULONG *pulNumProc);


void __RPC_STUB IDXTaskManager_QueryNumProcessors_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTaskManager_SetThreadPoolSize_Proxy( 
    IDXTaskManager * This,
    /* [in] */ ULONG ulNumThreads);


void __RPC_STUB IDXTaskManager_SetThreadPoolSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTaskManager_GetThreadPoolSize_Proxy( 
    IDXTaskManager * This,
    /* [out] */ ULONG *pulNumThreads);


void __RPC_STUB IDXTaskManager_GetThreadPoolSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTaskManager_SetConcurrencyLimit_Proxy( 
    IDXTaskManager * This,
    /* [in] */ ULONG ulNumThreads);


void __RPC_STUB IDXTaskManager_SetConcurrencyLimit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTaskManager_GetConcurrencyLimit_Proxy( 
    IDXTaskManager * This,
    /* [out] */ ULONG *pulNumThreads);


void __RPC_STUB IDXTaskManager_GetConcurrencyLimit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTaskManager_ScheduleTasks_Proxy( 
    IDXTaskManager * This,
    /* [in] */ DXTMTASKINFO TaskInfo[  ],
    /* [in] */ HANDLE Events[  ],
    /* [out] */ DWORD TaskIDs[  ],
    /* [in] */ ULONG ulNumTasks,
    /* [in] */ ULONG ulWaitPeriod);


void __RPC_STUB IDXTaskManager_ScheduleTasks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTaskManager_TerminateTasks_Proxy( 
    IDXTaskManager * This,
    /* [in] */ DWORD TaskIDs[  ],
    /* [in] */ ULONG ulCount,
    /* [in] */ ULONG ulTimeOut);


void __RPC_STUB IDXTaskManager_TerminateTasks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTaskManager_TerminateRequest_Proxy( 
    IDXTaskManager * This,
    /* [in] */ REFIID RequestID,
    /* [in] */ ULONG ulTimeOut);


void __RPC_STUB IDXTaskManager_TerminateRequest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTaskManager_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtrans_0265 */
/* [local] */ 

#ifdef __cplusplus
/////////////////////////////////////////////////////

class DXBASESAMPLE;
class DXSAMPLE;
class DXPMSAMPLE;

/////////////////////////////////////////////////////

class DXBASESAMPLE
{
public:
    BYTE Blue;
    BYTE Green;
    BYTE Red;
    BYTE Alpha;
    DXBASESAMPLE() {}
    DXBASESAMPLE(const BYTE alpha, const BYTE red, const BYTE green, const BYTE blue) :
        Alpha(alpha),
        Red(red),
        Green(green),
        Blue(blue) {}
    DXBASESAMPLE(const DWORD val) { *this = (*(DXBASESAMPLE *)&val); }
    operator DWORD () const {return *((DWORD *)this); }
    DWORD operator=(const DWORD val) { return *this = *((DXBASESAMPLE *)&val); }
}; // DXBASESAMPLE

/////////////////////////////////////////////////////

class DXSAMPLE : public DXBASESAMPLE
{
public:
    DXSAMPLE() {}
    DXSAMPLE(const BYTE alpha, const BYTE red, const BYTE green, const BYTE blue) :
         DXBASESAMPLE(alpha, red, green, blue) {}
    DXSAMPLE(const DWORD val) { *this = (*(DXSAMPLE *)&val); }
    operator DWORD () const {return *((DWORD *)this); }
    DWORD operator=(const DWORD val) { return *this = *((DXSAMPLE *)&val); }
    operator DXPMSAMPLE() const;
}; // DXSAMPLE

/////////////////////////////////////////////////////

class DXPMSAMPLE : public DXBASESAMPLE
{
public:
    DXPMSAMPLE() {}
    DXPMSAMPLE(const BYTE alpha, const BYTE red, const BYTE green, const BYTE blue) :
         DXBASESAMPLE(alpha, red, green, blue) {}
    DXPMSAMPLE(const DWORD val) { *this = (*(DXPMSAMPLE *)&val); }
    operator DWORD () const {return *((DWORD *)this); }
    DWORD operator=(const DWORD val) { return *this = *((DXPMSAMPLE *)&val); }
    operator DXSAMPLE() const;
}; // DXPMSAMPLE

//
// The following cast operators are to prevent a direct assignment of a DXSAMPLE to a DXPMSAMPLE
//
inline DXSAMPLE::operator DXPMSAMPLE() const { return *((DXPMSAMPLE *)this); }
inline DXPMSAMPLE::operator DXSAMPLE() const { return *((DXSAMPLE *)this); }
#else // !__cplusplus
typedef struct DXBASESAMPLE
    {
    BYTE Blue;
    BYTE Green;
    BYTE Red;
    BYTE Alpha;
    } 	DXBASESAMPLE;

typedef struct DXSAMPLE
    {
    BYTE Blue;
    BYTE Green;
    BYTE Red;
    BYTE Alpha;
    } 	DXSAMPLE;

typedef struct DXPMSAMPLE
    {
    BYTE Blue;
    BYTE Green;
    BYTE Red;
    BYTE Alpha;
    } 	DXPMSAMPLE;

#endif // !__cplusplus
typedef 
enum DXRUNTYPE
    {	DXRUNTYPE_CLEAR	= 0,
	DXRUNTYPE_OPAQUE	= 1,
	DXRUNTYPE_TRANS	= 2,
	DXRUNTYPE_UNKNOWN	= 3
    } 	DXRUNTYPE;

#define	DX_MAX_RUN_INFO_COUNT	( 128 )

// Ignore the definition used by MIDL for TLB generation
#if 0
typedef struct DXRUNINFO
    {
    ULONG Bitfields;
    } 	DXRUNINFO;

#endif // 0
typedef struct DXRUNINFO
{
    ULONG   Type  : 2;   // Type
    ULONG   Count : 30;  // Number of samples in run
} DXRUNINFO;
typedef 
enum DXSFCREATE
    {	DXSF_FORMAT_IS_CLSID	= 1L << 0,
	DXSF_NO_LAZY_DDRAW_LOCK	= 1L << 1
    } 	DXSFCREATE;

typedef 
enum DXBLTOPTIONS
    {	DXBOF_DO_OVER	= 1L << 0,
	DXBOF_DITHER	= 1L << 1
    } 	DXBLTOPTIONS;



extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0265_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0265_v0_0_s_ifspec;

#ifndef __IDXSurfaceFactory_INTERFACE_DEFINED__
#define __IDXSurfaceFactory_INTERFACE_DEFINED__

/* interface IDXSurfaceFactory */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXSurfaceFactory;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("144946F5-C4D4-11d1-81D1-0000F87557DB")
    IDXSurfaceFactory : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateSurface( 
            /* [in] */ IUnknown *pDirectDraw,
            /* [in] */ const DDSURFACEDESC *pDDSurfaceDesc,
            /* [in] */ const GUID *pFormatID,
            /* [in] */ const DXBNDS *pBounds,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IUnknown *punkOuter,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppDXSurface) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateFromDDSurface( 
            /* [in] */ IUnknown *pDDrawSurface,
            /* [in] */ const GUID *pFormatID,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IUnknown *punkOuter,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppDXSurface) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LoadImage( 
            /* [in] */ const LPWSTR pszFileName,
            /* [in] */ IUnknown *pDirectDraw,
            /* [in] */ const DDSURFACEDESC *pDDSurfaceDesc,
            /* [in] */ const GUID *pFormatID,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppDXSurface) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LoadImageFromStream( 
            /* [in] */ IStream *pStream,
            /* [in] */ IUnknown *pDirectDraw,
            /* [in] */ const DDSURFACEDESC *pDDSurfaceDesc,
            /* [in] */ const GUID *pFormatID,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppDXSurface) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CopySurfaceToNewFormat( 
            /* [in] */ IDXSurface *pSrc,
            /* [in] */ IUnknown *pDirectDraw,
            /* [in] */ const DDSURFACEDESC *pDDSurfaceDesc,
            /* [in] */ const GUID *pDestFormatID,
            /* [out] */ IDXSurface **ppNewSurface) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateD3DRMTexture( 
            /* [in] */ IDXSurface *pSrc,
            /* [in] */ IUnknown *pDirectDraw,
            /* [in] */ IUnknown *pD3DRM3,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppTexture3) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BitBlt( 
            /* [in] */ IDXSurface *pDest,
            /* [in] */ const DXVEC *pPlacement,
            /* [in] */ IDXSurface *pSrc,
            /* [in] */ const DXBNDS *pClipBounds,
            /* [in] */ DWORD dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXSurfaceFactoryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXSurfaceFactory * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXSurfaceFactory * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXSurfaceFactory * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSurface )( 
            IDXSurfaceFactory * This,
            /* [in] */ IUnknown *pDirectDraw,
            /* [in] */ const DDSURFACEDESC *pDDSurfaceDesc,
            /* [in] */ const GUID *pFormatID,
            /* [in] */ const DXBNDS *pBounds,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IUnknown *punkOuter,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppDXSurface);
        
        HRESULT ( STDMETHODCALLTYPE *CreateFromDDSurface )( 
            IDXSurfaceFactory * This,
            /* [in] */ IUnknown *pDDrawSurface,
            /* [in] */ const GUID *pFormatID,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IUnknown *punkOuter,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppDXSurface);
        
        HRESULT ( STDMETHODCALLTYPE *LoadImage )( 
            IDXSurfaceFactory * This,
            /* [in] */ const LPWSTR pszFileName,
            /* [in] */ IUnknown *pDirectDraw,
            /* [in] */ const DDSURFACEDESC *pDDSurfaceDesc,
            /* [in] */ const GUID *pFormatID,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppDXSurface);
        
        HRESULT ( STDMETHODCALLTYPE *LoadImageFromStream )( 
            IDXSurfaceFactory * This,
            /* [in] */ IStream *pStream,
            /* [in] */ IUnknown *pDirectDraw,
            /* [in] */ const DDSURFACEDESC *pDDSurfaceDesc,
            /* [in] */ const GUID *pFormatID,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppDXSurface);
        
        HRESULT ( STDMETHODCALLTYPE *CopySurfaceToNewFormat )( 
            IDXSurfaceFactory * This,
            /* [in] */ IDXSurface *pSrc,
            /* [in] */ IUnknown *pDirectDraw,
            /* [in] */ const DDSURFACEDESC *pDDSurfaceDesc,
            /* [in] */ const GUID *pDestFormatID,
            /* [out] */ IDXSurface **ppNewSurface);
        
        HRESULT ( STDMETHODCALLTYPE *CreateD3DRMTexture )( 
            IDXSurfaceFactory * This,
            /* [in] */ IDXSurface *pSrc,
            /* [in] */ IUnknown *pDirectDraw,
            /* [in] */ IUnknown *pD3DRM3,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppTexture3);
        
        HRESULT ( STDMETHODCALLTYPE *BitBlt )( 
            IDXSurfaceFactory * This,
            /* [in] */ IDXSurface *pDest,
            /* [in] */ const DXVEC *pPlacement,
            /* [in] */ IDXSurface *pSrc,
            /* [in] */ const DXBNDS *pClipBounds,
            /* [in] */ DWORD dwFlags);
        
        END_INTERFACE
    } IDXSurfaceFactoryVtbl;

    interface IDXSurfaceFactory
    {
        CONST_VTBL struct IDXSurfaceFactoryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXSurfaceFactory_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXSurfaceFactory_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXSurfaceFactory_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXSurfaceFactory_CreateSurface(This,pDirectDraw,pDDSurfaceDesc,pFormatID,pBounds,dwFlags,punkOuter,riid,ppDXSurface)	\
    (This)->lpVtbl -> CreateSurface(This,pDirectDraw,pDDSurfaceDesc,pFormatID,pBounds,dwFlags,punkOuter,riid,ppDXSurface)

#define IDXSurfaceFactory_CreateFromDDSurface(This,pDDrawSurface,pFormatID,dwFlags,punkOuter,riid,ppDXSurface)	\
    (This)->lpVtbl -> CreateFromDDSurface(This,pDDrawSurface,pFormatID,dwFlags,punkOuter,riid,ppDXSurface)

#define IDXSurfaceFactory_LoadImage(This,pszFileName,pDirectDraw,pDDSurfaceDesc,pFormatID,riid,ppDXSurface)	\
    (This)->lpVtbl -> LoadImage(This,pszFileName,pDirectDraw,pDDSurfaceDesc,pFormatID,riid,ppDXSurface)

#define IDXSurfaceFactory_LoadImageFromStream(This,pStream,pDirectDraw,pDDSurfaceDesc,pFormatID,riid,ppDXSurface)	\
    (This)->lpVtbl -> LoadImageFromStream(This,pStream,pDirectDraw,pDDSurfaceDesc,pFormatID,riid,ppDXSurface)

#define IDXSurfaceFactory_CopySurfaceToNewFormat(This,pSrc,pDirectDraw,pDDSurfaceDesc,pDestFormatID,ppNewSurface)	\
    (This)->lpVtbl -> CopySurfaceToNewFormat(This,pSrc,pDirectDraw,pDDSurfaceDesc,pDestFormatID,ppNewSurface)

#define IDXSurfaceFactory_CreateD3DRMTexture(This,pSrc,pDirectDraw,pD3DRM3,riid,ppTexture3)	\
    (This)->lpVtbl -> CreateD3DRMTexture(This,pSrc,pDirectDraw,pD3DRM3,riid,ppTexture3)

#define IDXSurfaceFactory_BitBlt(This,pDest,pPlacement,pSrc,pClipBounds,dwFlags)	\
    (This)->lpVtbl -> BitBlt(This,pDest,pPlacement,pSrc,pClipBounds,dwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXSurfaceFactory_CreateSurface_Proxy( 
    IDXSurfaceFactory * This,
    /* [in] */ IUnknown *pDirectDraw,
    /* [in] */ const DDSURFACEDESC *pDDSurfaceDesc,
    /* [in] */ const GUID *pFormatID,
    /* [in] */ const DXBNDS *pBounds,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IUnknown *punkOuter,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppDXSurface);


void __RPC_STUB IDXSurfaceFactory_CreateSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceFactory_CreateFromDDSurface_Proxy( 
    IDXSurfaceFactory * This,
    /* [in] */ IUnknown *pDDrawSurface,
    /* [in] */ const GUID *pFormatID,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IUnknown *punkOuter,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppDXSurface);


void __RPC_STUB IDXSurfaceFactory_CreateFromDDSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceFactory_LoadImage_Proxy( 
    IDXSurfaceFactory * This,
    /* [in] */ const LPWSTR pszFileName,
    /* [in] */ IUnknown *pDirectDraw,
    /* [in] */ const DDSURFACEDESC *pDDSurfaceDesc,
    /* [in] */ const GUID *pFormatID,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppDXSurface);


void __RPC_STUB IDXSurfaceFactory_LoadImage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceFactory_LoadImageFromStream_Proxy( 
    IDXSurfaceFactory * This,
    /* [in] */ IStream *pStream,
    /* [in] */ IUnknown *pDirectDraw,
    /* [in] */ const DDSURFACEDESC *pDDSurfaceDesc,
    /* [in] */ const GUID *pFormatID,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppDXSurface);


void __RPC_STUB IDXSurfaceFactory_LoadImageFromStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceFactory_CopySurfaceToNewFormat_Proxy( 
    IDXSurfaceFactory * This,
    /* [in] */ IDXSurface *pSrc,
    /* [in] */ IUnknown *pDirectDraw,
    /* [in] */ const DDSURFACEDESC *pDDSurfaceDesc,
    /* [in] */ const GUID *pDestFormatID,
    /* [out] */ IDXSurface **ppNewSurface);


void __RPC_STUB IDXSurfaceFactory_CopySurfaceToNewFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceFactory_CreateD3DRMTexture_Proxy( 
    IDXSurfaceFactory * This,
    /* [in] */ IDXSurface *pSrc,
    /* [in] */ IUnknown *pDirectDraw,
    /* [in] */ IUnknown *pD3DRM3,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppTexture3);


void __RPC_STUB IDXSurfaceFactory_CreateD3DRMTexture_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceFactory_BitBlt_Proxy( 
    IDXSurfaceFactory * This,
    /* [in] */ IDXSurface *pDest,
    /* [in] */ const DXVEC *pPlacement,
    /* [in] */ IDXSurface *pSrc,
    /* [in] */ const DXBNDS *pClipBounds,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IDXSurfaceFactory_BitBlt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXSurfaceFactory_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtrans_0266 */
/* [local] */ 

typedef 
enum DXSURFMODCOMPOP
    {	DXSURFMOD_COMP_OVER	= 0,
	DXSURFMOD_COMP_ALPHA_MASK	= 1,
	DXSURFMOD_COMP_MAX_VALID	= 1
    } 	DXSURFMODCOMPOP;



extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0266_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0266_v0_0_s_ifspec;

#ifndef __IDXSurfaceModifier_INTERFACE_DEFINED__
#define __IDXSurfaceModifier_INTERFACE_DEFINED__

/* interface IDXSurfaceModifier */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXSurfaceModifier;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9EA3B637-C37D-11d1-905E-00C04FD9189D")
    IDXSurfaceModifier : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetFillColor( 
            /* [in] */ DXSAMPLE Color) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFillColor( 
            /* [out] */ DXSAMPLE *pColor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBounds( 
            /* [in] */ const DXBNDS *pBounds) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBackground( 
            /* [in] */ IDXSurface *pSurface) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBackground( 
            /* [out] */ IDXSurface **ppSurface) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCompositeOperation( 
            /* [in] */ DXSURFMODCOMPOP CompOp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompositeOperation( 
            /* [out] */ DXSURFMODCOMPOP *pCompOp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetForeground( 
            /* [in] */ IDXSurface *pSurface,
            /* [in] */ BOOL bTile,
            /* [in] */ const POINT *pOrigin) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetForeground( 
            /* [out] */ IDXSurface **ppSurface,
            /* [out] */ BOOL *pbTile,
            /* [out] */ POINT *pOrigin) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOpacity( 
            /* [in] */ float Opacity) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOpacity( 
            /* [out] */ float *pOpacity) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLookup( 
            /* [in] */ IDXLookupTable *pLookupTable) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLookup( 
            /* [out] */ IDXLookupTable **ppLookupTable) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXSurfaceModifierVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXSurfaceModifier * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXSurfaceModifier * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXSurfaceModifier * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetFillColor )( 
            IDXSurfaceModifier * This,
            /* [in] */ DXSAMPLE Color);
        
        HRESULT ( STDMETHODCALLTYPE *GetFillColor )( 
            IDXSurfaceModifier * This,
            /* [out] */ DXSAMPLE *pColor);
        
        HRESULT ( STDMETHODCALLTYPE *SetBounds )( 
            IDXSurfaceModifier * This,
            /* [in] */ const DXBNDS *pBounds);
        
        HRESULT ( STDMETHODCALLTYPE *SetBackground )( 
            IDXSurfaceModifier * This,
            /* [in] */ IDXSurface *pSurface);
        
        HRESULT ( STDMETHODCALLTYPE *GetBackground )( 
            IDXSurfaceModifier * This,
            /* [out] */ IDXSurface **ppSurface);
        
        HRESULT ( STDMETHODCALLTYPE *SetCompositeOperation )( 
            IDXSurfaceModifier * This,
            /* [in] */ DXSURFMODCOMPOP CompOp);
        
        HRESULT ( STDMETHODCALLTYPE *GetCompositeOperation )( 
            IDXSurfaceModifier * This,
            /* [out] */ DXSURFMODCOMPOP *pCompOp);
        
        HRESULT ( STDMETHODCALLTYPE *SetForeground )( 
            IDXSurfaceModifier * This,
            /* [in] */ IDXSurface *pSurface,
            /* [in] */ BOOL bTile,
            /* [in] */ const POINT *pOrigin);
        
        HRESULT ( STDMETHODCALLTYPE *GetForeground )( 
            IDXSurfaceModifier * This,
            /* [out] */ IDXSurface **ppSurface,
            /* [out] */ BOOL *pbTile,
            /* [out] */ POINT *pOrigin);
        
        HRESULT ( STDMETHODCALLTYPE *SetOpacity )( 
            IDXSurfaceModifier * This,
            /* [in] */ float Opacity);
        
        HRESULT ( STDMETHODCALLTYPE *GetOpacity )( 
            IDXSurfaceModifier * This,
            /* [out] */ float *pOpacity);
        
        HRESULT ( STDMETHODCALLTYPE *SetLookup )( 
            IDXSurfaceModifier * This,
            /* [in] */ IDXLookupTable *pLookupTable);
        
        HRESULT ( STDMETHODCALLTYPE *GetLookup )( 
            IDXSurfaceModifier * This,
            /* [out] */ IDXLookupTable **ppLookupTable);
        
        END_INTERFACE
    } IDXSurfaceModifierVtbl;

    interface IDXSurfaceModifier
    {
        CONST_VTBL struct IDXSurfaceModifierVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXSurfaceModifier_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXSurfaceModifier_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXSurfaceModifier_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXSurfaceModifier_SetFillColor(This,Color)	\
    (This)->lpVtbl -> SetFillColor(This,Color)

#define IDXSurfaceModifier_GetFillColor(This,pColor)	\
    (This)->lpVtbl -> GetFillColor(This,pColor)

#define IDXSurfaceModifier_SetBounds(This,pBounds)	\
    (This)->lpVtbl -> SetBounds(This,pBounds)

#define IDXSurfaceModifier_SetBackground(This,pSurface)	\
    (This)->lpVtbl -> SetBackground(This,pSurface)

#define IDXSurfaceModifier_GetBackground(This,ppSurface)	\
    (This)->lpVtbl -> GetBackground(This,ppSurface)

#define IDXSurfaceModifier_SetCompositeOperation(This,CompOp)	\
    (This)->lpVtbl -> SetCompositeOperation(This,CompOp)

#define IDXSurfaceModifier_GetCompositeOperation(This,pCompOp)	\
    (This)->lpVtbl -> GetCompositeOperation(This,pCompOp)

#define IDXSurfaceModifier_SetForeground(This,pSurface,bTile,pOrigin)	\
    (This)->lpVtbl -> SetForeground(This,pSurface,bTile,pOrigin)

#define IDXSurfaceModifier_GetForeground(This,ppSurface,pbTile,pOrigin)	\
    (This)->lpVtbl -> GetForeground(This,ppSurface,pbTile,pOrigin)

#define IDXSurfaceModifier_SetOpacity(This,Opacity)	\
    (This)->lpVtbl -> SetOpacity(This,Opacity)

#define IDXSurfaceModifier_GetOpacity(This,pOpacity)	\
    (This)->lpVtbl -> GetOpacity(This,pOpacity)

#define IDXSurfaceModifier_SetLookup(This,pLookupTable)	\
    (This)->lpVtbl -> SetLookup(This,pLookupTable)

#define IDXSurfaceModifier_GetLookup(This,ppLookupTable)	\
    (This)->lpVtbl -> GetLookup(This,ppLookupTable)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXSurfaceModifier_SetFillColor_Proxy( 
    IDXSurfaceModifier * This,
    /* [in] */ DXSAMPLE Color);


void __RPC_STUB IDXSurfaceModifier_SetFillColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceModifier_GetFillColor_Proxy( 
    IDXSurfaceModifier * This,
    /* [out] */ DXSAMPLE *pColor);


void __RPC_STUB IDXSurfaceModifier_GetFillColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceModifier_SetBounds_Proxy( 
    IDXSurfaceModifier * This,
    /* [in] */ const DXBNDS *pBounds);


void __RPC_STUB IDXSurfaceModifier_SetBounds_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceModifier_SetBackground_Proxy( 
    IDXSurfaceModifier * This,
    /* [in] */ IDXSurface *pSurface);


void __RPC_STUB IDXSurfaceModifier_SetBackground_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceModifier_GetBackground_Proxy( 
    IDXSurfaceModifier * This,
    /* [out] */ IDXSurface **ppSurface);


void __RPC_STUB IDXSurfaceModifier_GetBackground_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceModifier_SetCompositeOperation_Proxy( 
    IDXSurfaceModifier * This,
    /* [in] */ DXSURFMODCOMPOP CompOp);


void __RPC_STUB IDXSurfaceModifier_SetCompositeOperation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceModifier_GetCompositeOperation_Proxy( 
    IDXSurfaceModifier * This,
    /* [out] */ DXSURFMODCOMPOP *pCompOp);


void __RPC_STUB IDXSurfaceModifier_GetCompositeOperation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceModifier_SetForeground_Proxy( 
    IDXSurfaceModifier * This,
    /* [in] */ IDXSurface *pSurface,
    /* [in] */ BOOL bTile,
    /* [in] */ const POINT *pOrigin);


void __RPC_STUB IDXSurfaceModifier_SetForeground_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceModifier_GetForeground_Proxy( 
    IDXSurfaceModifier * This,
    /* [out] */ IDXSurface **ppSurface,
    /* [out] */ BOOL *pbTile,
    /* [out] */ POINT *pOrigin);


void __RPC_STUB IDXSurfaceModifier_GetForeground_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceModifier_SetOpacity_Proxy( 
    IDXSurfaceModifier * This,
    /* [in] */ float Opacity);


void __RPC_STUB IDXSurfaceModifier_SetOpacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceModifier_GetOpacity_Proxy( 
    IDXSurfaceModifier * This,
    /* [out] */ float *pOpacity);


void __RPC_STUB IDXSurfaceModifier_GetOpacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceModifier_SetLookup_Proxy( 
    IDXSurfaceModifier * This,
    /* [in] */ IDXLookupTable *pLookupTable);


void __RPC_STUB IDXSurfaceModifier_SetLookup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceModifier_GetLookup_Proxy( 
    IDXSurfaceModifier * This,
    /* [out] */ IDXLookupTable **ppLookupTable);


void __RPC_STUB IDXSurfaceModifier_GetLookup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXSurfaceModifier_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtrans_0267 */
/* [local] */ 

typedef 
enum DXSAMPLEFORMATENUM
    {	DXPF_FLAGSMASK	= 0xffff0000,
	DXPF_NONPREMULT	= 0x10000,
	DXPF_TRANSPARENCY	= 0x20000,
	DXPF_TRANSLUCENCY	= 0x40000,
	DXPF_2BITERROR	= 0x200000,
	DXPF_3BITERROR	= 0x300000,
	DXPF_4BITERROR	= 0x400000,
	DXPF_5BITERROR	= 0x500000,
	DXPF_ERRORMASK	= 0x700000,
	DXPF_NONSTANDARD	= 0,
	DXPF_PMARGB32	= 1 | DXPF_TRANSPARENCY | DXPF_TRANSLUCENCY,
	DXPF_ARGB32	= 2 | DXPF_NONPREMULT | DXPF_TRANSPARENCY | DXPF_TRANSLUCENCY,
	DXPF_ARGB4444	= 3 | DXPF_NONPREMULT | DXPF_TRANSPARENCY | DXPF_TRANSLUCENCY | DXPF_4BITERROR,
	DXPF_A8	= 4 | DXPF_TRANSPARENCY | DXPF_TRANSLUCENCY,
	DXPF_RGB32	= 5,
	DXPF_RGB24	= 6,
	DXPF_RGB565	= 7 | DXPF_3BITERROR,
	DXPF_RGB555	= 8 | DXPF_3BITERROR,
	DXPF_RGB8	= 9 | DXPF_5BITERROR,
	DXPF_ARGB1555	= 10 | DXPF_TRANSPARENCY | DXPF_3BITERROR,
	DXPF_RGB32_CK	= DXPF_RGB32 | DXPF_TRANSPARENCY,
	DXPF_RGB24_CK	= DXPF_RGB24 | DXPF_TRANSPARENCY,
	DXPF_RGB555_CK	= DXPF_RGB555 | DXPF_TRANSPARENCY,
	DXPF_RGB565_CK	= DXPF_RGB565 | DXPF_TRANSPARENCY,
	DXPF_RGB8_CK	= DXPF_RGB8 | DXPF_TRANSPARENCY
    } 	DXSAMPLEFORMATENUM;

typedef 
enum DXLOCKSURF
    {	DXLOCKF_READ	= 0,
	DXLOCKF_READWRITE	= 1 << 0,
	DXLOCKF_EXISTINGINFOONLY	= 1 << 1,
	DXLOCKF_WANTRUNINFO	= 1 << 2,
	DXLOCKF_NONPREMULT	= 1 << 16,
	DXLOCKF_VALIDFLAGS	= DXLOCKF_READWRITE | DXLOCKF_EXISTINGINFOONLY | DXLOCKF_WANTRUNINFO | DXLOCKF_NONPREMULT
    } 	DXLOCKSURF;

typedef 
enum DXSURFSTATUS
    {	DXSURF_TRANSIENT	= 1 << 0,
	DXSURF_READONLY	= 1 << 1,
	DXSURF_VALIDFLAGS	= DXSURF_TRANSIENT | DXSURF_READONLY
    } 	DXSURFSTATUS;



extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0267_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0267_v0_0_s_ifspec;

#ifndef __IDXSurface_INTERFACE_DEFINED__
#define __IDXSurface_INTERFACE_DEFINED__

/* interface IDXSurface */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXSurface;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B39FD73F-E139-11d1-9065-00C04FD9189D")
    IDXSurface : public IDXBaseObject
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPixelFormat( 
            /* [out] */ GUID *pFormatID,
            /* [out] */ DXSAMPLEFORMATENUM *pSampleFormatEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBounds( 
            /* [out] */ DXBNDS *pBounds) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatusFlags( 
            /* [out] */ DWORD *pdwStatusFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetStatusFlags( 
            /* [in] */ DWORD dwStatusFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LockSurface( 
            /* [in] */ const DXBNDS *pBounds,
            /* [in] */ ULONG ulTimeOut,
            /* [in] */ DWORD dwFlags,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppPointer,
            /* [out] */ ULONG *pulGenerationId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDirectDrawSurface( 
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppSurface) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetColorKey( 
            DXSAMPLE *pColorKey) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetColorKey( 
            DXSAMPLE ColorKey) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LockSurfaceDC( 
            /* [in] */ const DXBNDS *pBounds,
            /* [in] */ ULONG ulTimeOut,
            /* [in] */ DWORD dwFlags,
            /* [out] */ IDXDCLock **ppDCLock) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAppData( 
            DWORD_PTR dwAppData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAppData( 
            DWORD_PTR *pdwAppData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXSurfaceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXSurface * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXSurface * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXSurface * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetGenerationId )( 
            IDXSurface * This,
            /* [out] */ ULONG *pID);
        
        HRESULT ( STDMETHODCALLTYPE *IncrementGenerationId )( 
            IDXSurface * This,
            /* [in] */ BOOL bRefresh);
        
        HRESULT ( STDMETHODCALLTYPE *GetObjectSize )( 
            IDXSurface * This,
            /* [out] */ ULONG *pcbSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetPixelFormat )( 
            IDXSurface * This,
            /* [out] */ GUID *pFormatID,
            /* [out] */ DXSAMPLEFORMATENUM *pSampleFormatEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetBounds )( 
            IDXSurface * This,
            /* [out] */ DXBNDS *pBounds);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatusFlags )( 
            IDXSurface * This,
            /* [out] */ DWORD *pdwStatusFlags);
        
        HRESULT ( STDMETHODCALLTYPE *SetStatusFlags )( 
            IDXSurface * This,
            /* [in] */ DWORD dwStatusFlags);
        
        HRESULT ( STDMETHODCALLTYPE *LockSurface )( 
            IDXSurface * This,
            /* [in] */ const DXBNDS *pBounds,
            /* [in] */ ULONG ulTimeOut,
            /* [in] */ DWORD dwFlags,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppPointer,
            /* [out] */ ULONG *pulGenerationId);
        
        HRESULT ( STDMETHODCALLTYPE *GetDirectDrawSurface )( 
            IDXSurface * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppSurface);
        
        HRESULT ( STDMETHODCALLTYPE *GetColorKey )( 
            IDXSurface * This,
            DXSAMPLE *pColorKey);
        
        HRESULT ( STDMETHODCALLTYPE *SetColorKey )( 
            IDXSurface * This,
            DXSAMPLE ColorKey);
        
        HRESULT ( STDMETHODCALLTYPE *LockSurfaceDC )( 
            IDXSurface * This,
            /* [in] */ const DXBNDS *pBounds,
            /* [in] */ ULONG ulTimeOut,
            /* [in] */ DWORD dwFlags,
            /* [out] */ IDXDCLock **ppDCLock);
        
        HRESULT ( STDMETHODCALLTYPE *SetAppData )( 
            IDXSurface * This,
            DWORD_PTR dwAppData);
        
        HRESULT ( STDMETHODCALLTYPE *GetAppData )( 
            IDXSurface * This,
            DWORD_PTR *pdwAppData);
        
        END_INTERFACE
    } IDXSurfaceVtbl;

    interface IDXSurface
    {
        CONST_VTBL struct IDXSurfaceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXSurface_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXSurface_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXSurface_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXSurface_GetGenerationId(This,pID)	\
    (This)->lpVtbl -> GetGenerationId(This,pID)

#define IDXSurface_IncrementGenerationId(This,bRefresh)	\
    (This)->lpVtbl -> IncrementGenerationId(This,bRefresh)

#define IDXSurface_GetObjectSize(This,pcbSize)	\
    (This)->lpVtbl -> GetObjectSize(This,pcbSize)


#define IDXSurface_GetPixelFormat(This,pFormatID,pSampleFormatEnum)	\
    (This)->lpVtbl -> GetPixelFormat(This,pFormatID,pSampleFormatEnum)

#define IDXSurface_GetBounds(This,pBounds)	\
    (This)->lpVtbl -> GetBounds(This,pBounds)

#define IDXSurface_GetStatusFlags(This,pdwStatusFlags)	\
    (This)->lpVtbl -> GetStatusFlags(This,pdwStatusFlags)

#define IDXSurface_SetStatusFlags(This,dwStatusFlags)	\
    (This)->lpVtbl -> SetStatusFlags(This,dwStatusFlags)

#define IDXSurface_LockSurface(This,pBounds,ulTimeOut,dwFlags,riid,ppPointer,pulGenerationId)	\
    (This)->lpVtbl -> LockSurface(This,pBounds,ulTimeOut,dwFlags,riid,ppPointer,pulGenerationId)

#define IDXSurface_GetDirectDrawSurface(This,riid,ppSurface)	\
    (This)->lpVtbl -> GetDirectDrawSurface(This,riid,ppSurface)

#define IDXSurface_GetColorKey(This,pColorKey)	\
    (This)->lpVtbl -> GetColorKey(This,pColorKey)

#define IDXSurface_SetColorKey(This,ColorKey)	\
    (This)->lpVtbl -> SetColorKey(This,ColorKey)

#define IDXSurface_LockSurfaceDC(This,pBounds,ulTimeOut,dwFlags,ppDCLock)	\
    (This)->lpVtbl -> LockSurfaceDC(This,pBounds,ulTimeOut,dwFlags,ppDCLock)

#define IDXSurface_SetAppData(This,dwAppData)	\
    (This)->lpVtbl -> SetAppData(This,dwAppData)

#define IDXSurface_GetAppData(This,pdwAppData)	\
    (This)->lpVtbl -> GetAppData(This,pdwAppData)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXSurface_GetPixelFormat_Proxy( 
    IDXSurface * This,
    /* [out] */ GUID *pFormatID,
    /* [out] */ DXSAMPLEFORMATENUM *pSampleFormatEnum);


void __RPC_STUB IDXSurface_GetPixelFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurface_GetBounds_Proxy( 
    IDXSurface * This,
    /* [out] */ DXBNDS *pBounds);


void __RPC_STUB IDXSurface_GetBounds_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurface_GetStatusFlags_Proxy( 
    IDXSurface * This,
    /* [out] */ DWORD *pdwStatusFlags);


void __RPC_STUB IDXSurface_GetStatusFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurface_SetStatusFlags_Proxy( 
    IDXSurface * This,
    /* [in] */ DWORD dwStatusFlags);


void __RPC_STUB IDXSurface_SetStatusFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurface_LockSurface_Proxy( 
    IDXSurface * This,
    /* [in] */ const DXBNDS *pBounds,
    /* [in] */ ULONG ulTimeOut,
    /* [in] */ DWORD dwFlags,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppPointer,
    /* [out] */ ULONG *pulGenerationId);


void __RPC_STUB IDXSurface_LockSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurface_GetDirectDrawSurface_Proxy( 
    IDXSurface * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppSurface);


void __RPC_STUB IDXSurface_GetDirectDrawSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurface_GetColorKey_Proxy( 
    IDXSurface * This,
    DXSAMPLE *pColorKey);


void __RPC_STUB IDXSurface_GetColorKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurface_SetColorKey_Proxy( 
    IDXSurface * This,
    DXSAMPLE ColorKey);


void __RPC_STUB IDXSurface_SetColorKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurface_LockSurfaceDC_Proxy( 
    IDXSurface * This,
    /* [in] */ const DXBNDS *pBounds,
    /* [in] */ ULONG ulTimeOut,
    /* [in] */ DWORD dwFlags,
    /* [out] */ IDXDCLock **ppDCLock);


void __RPC_STUB IDXSurface_LockSurfaceDC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurface_SetAppData_Proxy( 
    IDXSurface * This,
    DWORD_PTR dwAppData);


void __RPC_STUB IDXSurface_SetAppData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurface_GetAppData_Proxy( 
    IDXSurface * This,
    DWORD_PTR *pdwAppData);


void __RPC_STUB IDXSurface_GetAppData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXSurface_INTERFACE_DEFINED__ */


#ifndef __IDXSurfaceInit_INTERFACE_DEFINED__
#define __IDXSurfaceInit_INTERFACE_DEFINED__

/* interface IDXSurfaceInit */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXSurfaceInit;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9EA3B639-C37D-11d1-905E-00C04FD9189D")
    IDXSurfaceInit : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitSurface( 
            /* [in] */ IUnknown *pDirectDraw,
            /* [in] */ const DDSURFACEDESC *pDDSurfaceDesc,
            /* [in] */ const GUID *pFormatID,
            /* [in] */ const DXBNDS *pBounds,
            /* [in] */ DWORD dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXSurfaceInitVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXSurfaceInit * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXSurfaceInit * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXSurfaceInit * This);
        
        HRESULT ( STDMETHODCALLTYPE *InitSurface )( 
            IDXSurfaceInit * This,
            /* [in] */ IUnknown *pDirectDraw,
            /* [in] */ const DDSURFACEDESC *pDDSurfaceDesc,
            /* [in] */ const GUID *pFormatID,
            /* [in] */ const DXBNDS *pBounds,
            /* [in] */ DWORD dwFlags);
        
        END_INTERFACE
    } IDXSurfaceInitVtbl;

    interface IDXSurfaceInit
    {
        CONST_VTBL struct IDXSurfaceInitVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXSurfaceInit_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXSurfaceInit_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXSurfaceInit_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXSurfaceInit_InitSurface(This,pDirectDraw,pDDSurfaceDesc,pFormatID,pBounds,dwFlags)	\
    (This)->lpVtbl -> InitSurface(This,pDirectDraw,pDDSurfaceDesc,pFormatID,pBounds,dwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXSurfaceInit_InitSurface_Proxy( 
    IDXSurfaceInit * This,
    /* [in] */ IUnknown *pDirectDraw,
    /* [in] */ const DDSURFACEDESC *pDDSurfaceDesc,
    /* [in] */ const GUID *pFormatID,
    /* [in] */ const DXBNDS *pBounds,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IDXSurfaceInit_InitSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXSurfaceInit_INTERFACE_DEFINED__ */


#ifndef __IDXARGBSurfaceInit_INTERFACE_DEFINED__
#define __IDXARGBSurfaceInit_INTERFACE_DEFINED__

/* interface IDXARGBSurfaceInit */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXARGBSurfaceInit;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9EA3B63A-C37D-11d1-905E-00C04FD9189D")
    IDXARGBSurfaceInit : public IDXSurfaceInit
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitFromDDSurface( 
            /* [in] */ IUnknown *pDDrawSurface,
            /* [in] */ const GUID *pFormatID,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InitFromRawSurface( 
            /* [in] */ IDXRawSurface *pRawSurface) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXARGBSurfaceInitVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXARGBSurfaceInit * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXARGBSurfaceInit * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXARGBSurfaceInit * This);
        
        HRESULT ( STDMETHODCALLTYPE *InitSurface )( 
            IDXARGBSurfaceInit * This,
            /* [in] */ IUnknown *pDirectDraw,
            /* [in] */ const DDSURFACEDESC *pDDSurfaceDesc,
            /* [in] */ const GUID *pFormatID,
            /* [in] */ const DXBNDS *pBounds,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *InitFromDDSurface )( 
            IDXARGBSurfaceInit * This,
            /* [in] */ IUnknown *pDDrawSurface,
            /* [in] */ const GUID *pFormatID,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *InitFromRawSurface )( 
            IDXARGBSurfaceInit * This,
            /* [in] */ IDXRawSurface *pRawSurface);
        
        END_INTERFACE
    } IDXARGBSurfaceInitVtbl;

    interface IDXARGBSurfaceInit
    {
        CONST_VTBL struct IDXARGBSurfaceInitVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXARGBSurfaceInit_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXARGBSurfaceInit_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXARGBSurfaceInit_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXARGBSurfaceInit_InitSurface(This,pDirectDraw,pDDSurfaceDesc,pFormatID,pBounds,dwFlags)	\
    (This)->lpVtbl -> InitSurface(This,pDirectDraw,pDDSurfaceDesc,pFormatID,pBounds,dwFlags)


#define IDXARGBSurfaceInit_InitFromDDSurface(This,pDDrawSurface,pFormatID,dwFlags)	\
    (This)->lpVtbl -> InitFromDDSurface(This,pDDrawSurface,pFormatID,dwFlags)

#define IDXARGBSurfaceInit_InitFromRawSurface(This,pRawSurface)	\
    (This)->lpVtbl -> InitFromRawSurface(This,pRawSurface)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXARGBSurfaceInit_InitFromDDSurface_Proxy( 
    IDXARGBSurfaceInit * This,
    /* [in] */ IUnknown *pDDrawSurface,
    /* [in] */ const GUID *pFormatID,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IDXARGBSurfaceInit_InitFromDDSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXARGBSurfaceInit_InitFromRawSurface_Proxy( 
    IDXARGBSurfaceInit * This,
    /* [in] */ IDXRawSurface *pRawSurface);


void __RPC_STUB IDXARGBSurfaceInit_InitFromRawSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXARGBSurfaceInit_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtrans_0270 */
/* [local] */ 

typedef struct tagDXNATIVETYPEINFO
    {
    BYTE *pCurrentData;
    BYTE *pFirstByte;
    long lPitch;
    DWORD dwColorKey;
    } 	DXNATIVETYPEINFO;

typedef struct tagDXPACKEDRECTDESC
    {
    DXBASESAMPLE *pSamples;
    BOOL bPremult;
    RECT rect;
    long lRowPadding;
    } 	DXPACKEDRECTDESC;

typedef struct tagDXOVERSAMPLEDESC
    {
    POINT p;
    DXPMSAMPLE Color;
    } 	DXOVERSAMPLEDESC;



extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0270_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0270_v0_0_s_ifspec;

#ifndef __IDXARGBReadPtr_INTERFACE_DEFINED__
#define __IDXARGBReadPtr_INTERFACE_DEFINED__

/* interface IDXARGBReadPtr */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXARGBReadPtr;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EAAAC2D6-C290-11d1-905D-00C04FD9189D")
    IDXARGBReadPtr : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSurface( 
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppSurface) = 0;
        
        virtual DXSAMPLEFORMATENUM STDMETHODCALLTYPE GetNativeType( 
            /* [out] */ DXNATIVETYPEINFO *pInfo) = 0;
        
        virtual void STDMETHODCALLTYPE Move( 
            /* [in] */ long cSamples) = 0;
        
        virtual void STDMETHODCALLTYPE MoveToRow( 
            /* [in] */ ULONG y) = 0;
        
        virtual void STDMETHODCALLTYPE MoveToXY( 
            /* [in] */ ULONG x,
            /* [in] */ ULONG y) = 0;
        
        virtual ULONG STDMETHODCALLTYPE MoveAndGetRunInfo( 
            /* [in] */ ULONG Row,
            /* [out] */ const DXRUNINFO **ppInfo) = 0;
        
        virtual DXSAMPLE *STDMETHODCALLTYPE Unpack( 
            /* [in] */ DXSAMPLE *pSamples,
            /* [in] */ ULONG cSamples,
            /* [in] */ BOOL bMove) = 0;
        
        virtual DXPMSAMPLE *STDMETHODCALLTYPE UnpackPremult( 
            /* [in] */ DXPMSAMPLE *pSamples,
            /* [in] */ ULONG cSamples,
            /* [in] */ BOOL bMove) = 0;
        
        virtual void STDMETHODCALLTYPE UnpackRect( 
            /* [in] */ const DXPACKEDRECTDESC *pRectDesc) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXARGBReadPtrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXARGBReadPtr * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXARGBReadPtr * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXARGBReadPtr * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSurface )( 
            IDXARGBReadPtr * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppSurface);
        
        DXSAMPLEFORMATENUM ( STDMETHODCALLTYPE *GetNativeType )( 
            IDXARGBReadPtr * This,
            /* [out] */ DXNATIVETYPEINFO *pInfo);
        
        void ( STDMETHODCALLTYPE *Move )( 
            IDXARGBReadPtr * This,
            /* [in] */ long cSamples);
        
        void ( STDMETHODCALLTYPE *MoveToRow )( 
            IDXARGBReadPtr * This,
            /* [in] */ ULONG y);
        
        void ( STDMETHODCALLTYPE *MoveToXY )( 
            IDXARGBReadPtr * This,
            /* [in] */ ULONG x,
            /* [in] */ ULONG y);
        
        ULONG ( STDMETHODCALLTYPE *MoveAndGetRunInfo )( 
            IDXARGBReadPtr * This,
            /* [in] */ ULONG Row,
            /* [out] */ const DXRUNINFO **ppInfo);
        
        DXSAMPLE *( STDMETHODCALLTYPE *Unpack )( 
            IDXARGBReadPtr * This,
            /* [in] */ DXSAMPLE *pSamples,
            /* [in] */ ULONG cSamples,
            /* [in] */ BOOL bMove);
        
        DXPMSAMPLE *( STDMETHODCALLTYPE *UnpackPremult )( 
            IDXARGBReadPtr * This,
            /* [in] */ DXPMSAMPLE *pSamples,
            /* [in] */ ULONG cSamples,
            /* [in] */ BOOL bMove);
        
        void ( STDMETHODCALLTYPE *UnpackRect )( 
            IDXARGBReadPtr * This,
            /* [in] */ const DXPACKEDRECTDESC *pRectDesc);
        
        END_INTERFACE
    } IDXARGBReadPtrVtbl;

    interface IDXARGBReadPtr
    {
        CONST_VTBL struct IDXARGBReadPtrVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXARGBReadPtr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXARGBReadPtr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXARGBReadPtr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXARGBReadPtr_GetSurface(This,riid,ppSurface)	\
    (This)->lpVtbl -> GetSurface(This,riid,ppSurface)

#define IDXARGBReadPtr_GetNativeType(This,pInfo)	\
    (This)->lpVtbl -> GetNativeType(This,pInfo)

#define IDXARGBReadPtr_Move(This,cSamples)	\
    (This)->lpVtbl -> Move(This,cSamples)

#define IDXARGBReadPtr_MoveToRow(This,y)	\
    (This)->lpVtbl -> MoveToRow(This,y)

#define IDXARGBReadPtr_MoveToXY(This,x,y)	\
    (This)->lpVtbl -> MoveToXY(This,x,y)

#define IDXARGBReadPtr_MoveAndGetRunInfo(This,Row,ppInfo)	\
    (This)->lpVtbl -> MoveAndGetRunInfo(This,Row,ppInfo)

#define IDXARGBReadPtr_Unpack(This,pSamples,cSamples,bMove)	\
    (This)->lpVtbl -> Unpack(This,pSamples,cSamples,bMove)

#define IDXARGBReadPtr_UnpackPremult(This,pSamples,cSamples,bMove)	\
    (This)->lpVtbl -> UnpackPremult(This,pSamples,cSamples,bMove)

#define IDXARGBReadPtr_UnpackRect(This,pRectDesc)	\
    (This)->lpVtbl -> UnpackRect(This,pRectDesc)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXARGBReadPtr_GetSurface_Proxy( 
    IDXARGBReadPtr * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppSurface);


void __RPC_STUB IDXARGBReadPtr_GetSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DXSAMPLEFORMATENUM STDMETHODCALLTYPE IDXARGBReadPtr_GetNativeType_Proxy( 
    IDXARGBReadPtr * This,
    /* [out] */ DXNATIVETYPEINFO *pInfo);


void __RPC_STUB IDXARGBReadPtr_GetNativeType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDXARGBReadPtr_Move_Proxy( 
    IDXARGBReadPtr * This,
    /* [in] */ long cSamples);


void __RPC_STUB IDXARGBReadPtr_Move_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDXARGBReadPtr_MoveToRow_Proxy( 
    IDXARGBReadPtr * This,
    /* [in] */ ULONG y);


void __RPC_STUB IDXARGBReadPtr_MoveToRow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDXARGBReadPtr_MoveToXY_Proxy( 
    IDXARGBReadPtr * This,
    /* [in] */ ULONG x,
    /* [in] */ ULONG y);


void __RPC_STUB IDXARGBReadPtr_MoveToXY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


ULONG STDMETHODCALLTYPE IDXARGBReadPtr_MoveAndGetRunInfo_Proxy( 
    IDXARGBReadPtr * This,
    /* [in] */ ULONG Row,
    /* [out] */ const DXRUNINFO **ppInfo);


void __RPC_STUB IDXARGBReadPtr_MoveAndGetRunInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DXSAMPLE *STDMETHODCALLTYPE IDXARGBReadPtr_Unpack_Proxy( 
    IDXARGBReadPtr * This,
    /* [in] */ DXSAMPLE *pSamples,
    /* [in] */ ULONG cSamples,
    /* [in] */ BOOL bMove);


void __RPC_STUB IDXARGBReadPtr_Unpack_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DXPMSAMPLE *STDMETHODCALLTYPE IDXARGBReadPtr_UnpackPremult_Proxy( 
    IDXARGBReadPtr * This,
    /* [in] */ DXPMSAMPLE *pSamples,
    /* [in] */ ULONG cSamples,
    /* [in] */ BOOL bMove);


void __RPC_STUB IDXARGBReadPtr_UnpackPremult_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDXARGBReadPtr_UnpackRect_Proxy( 
    IDXARGBReadPtr * This,
    /* [in] */ const DXPACKEDRECTDESC *pRectDesc);


void __RPC_STUB IDXARGBReadPtr_UnpackRect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXARGBReadPtr_INTERFACE_DEFINED__ */


#ifndef __IDXARGBReadWritePtr_INTERFACE_DEFINED__
#define __IDXARGBReadWritePtr_INTERFACE_DEFINED__

/* interface IDXARGBReadWritePtr */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXARGBReadWritePtr;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EAAAC2D7-C290-11d1-905D-00C04FD9189D")
    IDXARGBReadWritePtr : public IDXARGBReadPtr
    {
    public:
        virtual void STDMETHODCALLTYPE PackAndMove( 
            /* [in] */ const DXSAMPLE *pSamples,
            /* [in] */ ULONG cSamples) = 0;
        
        virtual void STDMETHODCALLTYPE PackPremultAndMove( 
            /* [in] */ const DXPMSAMPLE *pSamples,
            /* [in] */ ULONG cSamples) = 0;
        
        virtual void STDMETHODCALLTYPE PackRect( 
            /* [in] */ const DXPACKEDRECTDESC *pRectDesc) = 0;
        
        virtual void STDMETHODCALLTYPE CopyAndMoveBoth( 
            /* [in] */ DXBASESAMPLE *pScratchBuffer,
            /* [in] */ IDXARGBReadPtr *pSrc,
            /* [in] */ ULONG cSamples,
            /* [in] */ BOOL bIsOpaque) = 0;
        
        virtual void STDMETHODCALLTYPE CopyRect( 
            /* [in] */ DXBASESAMPLE *pScratchBuffer,
            /* [in] */ const RECT *pDestRect,
            /* [in] */ IDXARGBReadPtr *pSrc,
            /* [in] */ const POINT *pSrcOrigin,
            /* [in] */ BOOL bIsOpaque) = 0;
        
        virtual void STDMETHODCALLTYPE FillAndMove( 
            /* [in] */ DXBASESAMPLE *pScratchBuffer,
            /* [in] */ DXPMSAMPLE SampVal,
            /* [in] */ ULONG cSamples,
            /* [in] */ BOOL bDoOver) = 0;
        
        virtual void STDMETHODCALLTYPE FillRect( 
            /* [in] */ const RECT *pRect,
            /* [in] */ DXPMSAMPLE SampVal,
            /* [in] */ BOOL bDoOver) = 0;
        
        virtual void STDMETHODCALLTYPE OverSample( 
            /* [in] */ const DXOVERSAMPLEDESC *pOverDesc) = 0;
        
        virtual void STDMETHODCALLTYPE OverArrayAndMove( 
            /* [in] */ DXBASESAMPLE *pScratchBuffer,
            /* [in] */ const DXPMSAMPLE *pSrc,
            /* [in] */ ULONG cSamples) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXARGBReadWritePtrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXARGBReadWritePtr * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXARGBReadWritePtr * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXARGBReadWritePtr * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSurface )( 
            IDXARGBReadWritePtr * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppSurface);
        
        DXSAMPLEFORMATENUM ( STDMETHODCALLTYPE *GetNativeType )( 
            IDXARGBReadWritePtr * This,
            /* [out] */ DXNATIVETYPEINFO *pInfo);
        
        void ( STDMETHODCALLTYPE *Move )( 
            IDXARGBReadWritePtr * This,
            /* [in] */ long cSamples);
        
        void ( STDMETHODCALLTYPE *MoveToRow )( 
            IDXARGBReadWritePtr * This,
            /* [in] */ ULONG y);
        
        void ( STDMETHODCALLTYPE *MoveToXY )( 
            IDXARGBReadWritePtr * This,
            /* [in] */ ULONG x,
            /* [in] */ ULONG y);
        
        ULONG ( STDMETHODCALLTYPE *MoveAndGetRunInfo )( 
            IDXARGBReadWritePtr * This,
            /* [in] */ ULONG Row,
            /* [out] */ const DXRUNINFO **ppInfo);
        
        DXSAMPLE *( STDMETHODCALLTYPE *Unpack )( 
            IDXARGBReadWritePtr * This,
            /* [in] */ DXSAMPLE *pSamples,
            /* [in] */ ULONG cSamples,
            /* [in] */ BOOL bMove);
        
        DXPMSAMPLE *( STDMETHODCALLTYPE *UnpackPremult )( 
            IDXARGBReadWritePtr * This,
            /* [in] */ DXPMSAMPLE *pSamples,
            /* [in] */ ULONG cSamples,
            /* [in] */ BOOL bMove);
        
        void ( STDMETHODCALLTYPE *UnpackRect )( 
            IDXARGBReadWritePtr * This,
            /* [in] */ const DXPACKEDRECTDESC *pRectDesc);
        
        void ( STDMETHODCALLTYPE *PackAndMove )( 
            IDXARGBReadWritePtr * This,
            /* [in] */ const DXSAMPLE *pSamples,
            /* [in] */ ULONG cSamples);
        
        void ( STDMETHODCALLTYPE *PackPremultAndMove )( 
            IDXARGBReadWritePtr * This,
            /* [in] */ const DXPMSAMPLE *pSamples,
            /* [in] */ ULONG cSamples);
        
        void ( STDMETHODCALLTYPE *PackRect )( 
            IDXARGBReadWritePtr * This,
            /* [in] */ const DXPACKEDRECTDESC *pRectDesc);
        
        void ( STDMETHODCALLTYPE *CopyAndMoveBoth )( 
            IDXARGBReadWritePtr * This,
            /* [in] */ DXBASESAMPLE *pScratchBuffer,
            /* [in] */ IDXARGBReadPtr *pSrc,
            /* [in] */ ULONG cSamples,
            /* [in] */ BOOL bIsOpaque);
        
        void ( STDMETHODCALLTYPE *CopyRect )( 
            IDXARGBReadWritePtr * This,
            /* [in] */ DXBASESAMPLE *pScratchBuffer,
            /* [in] */ const RECT *pDestRect,
            /* [in] */ IDXARGBReadPtr *pSrc,
            /* [in] */ const POINT *pSrcOrigin,
            /* [in] */ BOOL bIsOpaque);
        
        void ( STDMETHODCALLTYPE *FillAndMove )( 
            IDXARGBReadWritePtr * This,
            /* [in] */ DXBASESAMPLE *pScratchBuffer,
            /* [in] */ DXPMSAMPLE SampVal,
            /* [in] */ ULONG cSamples,
            /* [in] */ BOOL bDoOver);
        
        void ( STDMETHODCALLTYPE *FillRect )( 
            IDXARGBReadWritePtr * This,
            /* [in] */ const RECT *pRect,
            /* [in] */ DXPMSAMPLE SampVal,
            /* [in] */ BOOL bDoOver);
        
        void ( STDMETHODCALLTYPE *OverSample )( 
            IDXARGBReadWritePtr * This,
            /* [in] */ const DXOVERSAMPLEDESC *pOverDesc);
        
        void ( STDMETHODCALLTYPE *OverArrayAndMove )( 
            IDXARGBReadWritePtr * This,
            /* [in] */ DXBASESAMPLE *pScratchBuffer,
            /* [in] */ const DXPMSAMPLE *pSrc,
            /* [in] */ ULONG cSamples);
        
        END_INTERFACE
    } IDXARGBReadWritePtrVtbl;

    interface IDXARGBReadWritePtr
    {
        CONST_VTBL struct IDXARGBReadWritePtrVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXARGBReadWritePtr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXARGBReadWritePtr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXARGBReadWritePtr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXARGBReadWritePtr_GetSurface(This,riid,ppSurface)	\
    (This)->lpVtbl -> GetSurface(This,riid,ppSurface)

#define IDXARGBReadWritePtr_GetNativeType(This,pInfo)	\
    (This)->lpVtbl -> GetNativeType(This,pInfo)

#define IDXARGBReadWritePtr_Move(This,cSamples)	\
    (This)->lpVtbl -> Move(This,cSamples)

#define IDXARGBReadWritePtr_MoveToRow(This,y)	\
    (This)->lpVtbl -> MoveToRow(This,y)

#define IDXARGBReadWritePtr_MoveToXY(This,x,y)	\
    (This)->lpVtbl -> MoveToXY(This,x,y)

#define IDXARGBReadWritePtr_MoveAndGetRunInfo(This,Row,ppInfo)	\
    (This)->lpVtbl -> MoveAndGetRunInfo(This,Row,ppInfo)

#define IDXARGBReadWritePtr_Unpack(This,pSamples,cSamples,bMove)	\
    (This)->lpVtbl -> Unpack(This,pSamples,cSamples,bMove)

#define IDXARGBReadWritePtr_UnpackPremult(This,pSamples,cSamples,bMove)	\
    (This)->lpVtbl -> UnpackPremult(This,pSamples,cSamples,bMove)

#define IDXARGBReadWritePtr_UnpackRect(This,pRectDesc)	\
    (This)->lpVtbl -> UnpackRect(This,pRectDesc)


#define IDXARGBReadWritePtr_PackAndMove(This,pSamples,cSamples)	\
    (This)->lpVtbl -> PackAndMove(This,pSamples,cSamples)

#define IDXARGBReadWritePtr_PackPremultAndMove(This,pSamples,cSamples)	\
    (This)->lpVtbl -> PackPremultAndMove(This,pSamples,cSamples)

#define IDXARGBReadWritePtr_PackRect(This,pRectDesc)	\
    (This)->lpVtbl -> PackRect(This,pRectDesc)

#define IDXARGBReadWritePtr_CopyAndMoveBoth(This,pScratchBuffer,pSrc,cSamples,bIsOpaque)	\
    (This)->lpVtbl -> CopyAndMoveBoth(This,pScratchBuffer,pSrc,cSamples,bIsOpaque)

#define IDXARGBReadWritePtr_CopyRect(This,pScratchBuffer,pDestRect,pSrc,pSrcOrigin,bIsOpaque)	\
    (This)->lpVtbl -> CopyRect(This,pScratchBuffer,pDestRect,pSrc,pSrcOrigin,bIsOpaque)

#define IDXARGBReadWritePtr_FillAndMove(This,pScratchBuffer,SampVal,cSamples,bDoOver)	\
    (This)->lpVtbl -> FillAndMove(This,pScratchBuffer,SampVal,cSamples,bDoOver)

#define IDXARGBReadWritePtr_FillRect(This,pRect,SampVal,bDoOver)	\
    (This)->lpVtbl -> FillRect(This,pRect,SampVal,bDoOver)

#define IDXARGBReadWritePtr_OverSample(This,pOverDesc)	\
    (This)->lpVtbl -> OverSample(This,pOverDesc)

#define IDXARGBReadWritePtr_OverArrayAndMove(This,pScratchBuffer,pSrc,cSamples)	\
    (This)->lpVtbl -> OverArrayAndMove(This,pScratchBuffer,pSrc,cSamples)

#endif /* COBJMACROS */


#endif 	/* C style interface */



void STDMETHODCALLTYPE IDXARGBReadWritePtr_PackAndMove_Proxy( 
    IDXARGBReadWritePtr * This,
    /* [in] */ const DXSAMPLE *pSamples,
    /* [in] */ ULONG cSamples);


void __RPC_STUB IDXARGBReadWritePtr_PackAndMove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDXARGBReadWritePtr_PackPremultAndMove_Proxy( 
    IDXARGBReadWritePtr * This,
    /* [in] */ const DXPMSAMPLE *pSamples,
    /* [in] */ ULONG cSamples);


void __RPC_STUB IDXARGBReadWritePtr_PackPremultAndMove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDXARGBReadWritePtr_PackRect_Proxy( 
    IDXARGBReadWritePtr * This,
    /* [in] */ const DXPACKEDRECTDESC *pRectDesc);


void __RPC_STUB IDXARGBReadWritePtr_PackRect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDXARGBReadWritePtr_CopyAndMoveBoth_Proxy( 
    IDXARGBReadWritePtr * This,
    /* [in] */ DXBASESAMPLE *pScratchBuffer,
    /* [in] */ IDXARGBReadPtr *pSrc,
    /* [in] */ ULONG cSamples,
    /* [in] */ BOOL bIsOpaque);


void __RPC_STUB IDXARGBReadWritePtr_CopyAndMoveBoth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDXARGBReadWritePtr_CopyRect_Proxy( 
    IDXARGBReadWritePtr * This,
    /* [in] */ DXBASESAMPLE *pScratchBuffer,
    /* [in] */ const RECT *pDestRect,
    /* [in] */ IDXARGBReadPtr *pSrc,
    /* [in] */ const POINT *pSrcOrigin,
    /* [in] */ BOOL bIsOpaque);


void __RPC_STUB IDXARGBReadWritePtr_CopyRect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDXARGBReadWritePtr_FillAndMove_Proxy( 
    IDXARGBReadWritePtr * This,
    /* [in] */ DXBASESAMPLE *pScratchBuffer,
    /* [in] */ DXPMSAMPLE SampVal,
    /* [in] */ ULONG cSamples,
    /* [in] */ BOOL bDoOver);


void __RPC_STUB IDXARGBReadWritePtr_FillAndMove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDXARGBReadWritePtr_FillRect_Proxy( 
    IDXARGBReadWritePtr * This,
    /* [in] */ const RECT *pRect,
    /* [in] */ DXPMSAMPLE SampVal,
    /* [in] */ BOOL bDoOver);


void __RPC_STUB IDXARGBReadWritePtr_FillRect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDXARGBReadWritePtr_OverSample_Proxy( 
    IDXARGBReadWritePtr * This,
    /* [in] */ const DXOVERSAMPLEDESC *pOverDesc);


void __RPC_STUB IDXARGBReadWritePtr_OverSample_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDXARGBReadWritePtr_OverArrayAndMove_Proxy( 
    IDXARGBReadWritePtr * This,
    /* [in] */ DXBASESAMPLE *pScratchBuffer,
    /* [in] */ const DXPMSAMPLE *pSrc,
    /* [in] */ ULONG cSamples);


void __RPC_STUB IDXARGBReadWritePtr_OverArrayAndMove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXARGBReadWritePtr_INTERFACE_DEFINED__ */


#ifndef __IDXDCLock_INTERFACE_DEFINED__
#define __IDXDCLock_INTERFACE_DEFINED__

/* interface IDXDCLock */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXDCLock;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0F619456-CF39-11d1-905E-00C04FD9189D")
    IDXDCLock : public IUnknown
    {
    public:
        virtual HDC STDMETHODCALLTYPE GetDC( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXDCLockVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXDCLock * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXDCLock * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXDCLock * This);
        
        HDC ( STDMETHODCALLTYPE *GetDC )( 
            IDXDCLock * This);
        
        END_INTERFACE
    } IDXDCLockVtbl;

    interface IDXDCLock
    {
        CONST_VTBL struct IDXDCLockVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXDCLock_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXDCLock_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXDCLock_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXDCLock_GetDC(This)	\
    (This)->lpVtbl -> GetDC(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HDC STDMETHODCALLTYPE IDXDCLock_GetDC_Proxy( 
    IDXDCLock * This);


void __RPC_STUB IDXDCLock_GetDC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXDCLock_INTERFACE_DEFINED__ */


#ifndef __IDXTScaleOutput_INTERFACE_DEFINED__
#define __IDXTScaleOutput_INTERFACE_DEFINED__

/* interface IDXTScaleOutput */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXTScaleOutput;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B2024B50-EE77-11d1-9066-00C04FD9189D")
    IDXTScaleOutput : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetOutputSize( 
            /* [in] */ const SIZE OutSize,
            /* [in] */ BOOL bMaintainAspect) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTScaleOutputVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTScaleOutput * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTScaleOutput * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTScaleOutput * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetOutputSize )( 
            IDXTScaleOutput * This,
            /* [in] */ const SIZE OutSize,
            /* [in] */ BOOL bMaintainAspect);
        
        END_INTERFACE
    } IDXTScaleOutputVtbl;

    interface IDXTScaleOutput
    {
        CONST_VTBL struct IDXTScaleOutputVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTScaleOutput_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTScaleOutput_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTScaleOutput_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTScaleOutput_SetOutputSize(This,OutSize,bMaintainAspect)	\
    (This)->lpVtbl -> SetOutputSize(This,OutSize,bMaintainAspect)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXTScaleOutput_SetOutputSize_Proxy( 
    IDXTScaleOutput * This,
    /* [in] */ const SIZE OutSize,
    /* [in] */ BOOL bMaintainAspect);


void __RPC_STUB IDXTScaleOutput_SetOutputSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTScaleOutput_INTERFACE_DEFINED__ */


#ifndef __IDXGradient_INTERFACE_DEFINED__
#define __IDXGradient_INTERFACE_DEFINED__

/* interface IDXGradient */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXGradient;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B2024B51-EE77-11d1-9066-00C04FD9189D")
    IDXGradient : public IDXTScaleOutput
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetGradient( 
            DXSAMPLE StartColor,
            DXSAMPLE EndColor,
            BOOL bHorizontal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOutputSize( 
            /* [out] */ SIZE *pOutSize) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXGradientVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGradient * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGradient * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGradient * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetOutputSize )( 
            IDXGradient * This,
            /* [in] */ const SIZE OutSize,
            /* [in] */ BOOL bMaintainAspect);
        
        HRESULT ( STDMETHODCALLTYPE *SetGradient )( 
            IDXGradient * This,
            DXSAMPLE StartColor,
            DXSAMPLE EndColor,
            BOOL bHorizontal);
        
        HRESULT ( STDMETHODCALLTYPE *GetOutputSize )( 
            IDXGradient * This,
            /* [out] */ SIZE *pOutSize);
        
        END_INTERFACE
    } IDXGradientVtbl;

    interface IDXGradient
    {
        CONST_VTBL struct IDXGradientVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGradient_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXGradient_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXGradient_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXGradient_SetOutputSize(This,OutSize,bMaintainAspect)	\
    (This)->lpVtbl -> SetOutputSize(This,OutSize,bMaintainAspect)


#define IDXGradient_SetGradient(This,StartColor,EndColor,bHorizontal)	\
    (This)->lpVtbl -> SetGradient(This,StartColor,EndColor,bHorizontal)

#define IDXGradient_GetOutputSize(This,pOutSize)	\
    (This)->lpVtbl -> GetOutputSize(This,pOutSize)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXGradient_SetGradient_Proxy( 
    IDXGradient * This,
    DXSAMPLE StartColor,
    DXSAMPLE EndColor,
    BOOL bHorizontal);


void __RPC_STUB IDXGradient_SetGradient_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXGradient_GetOutputSize_Proxy( 
    IDXGradient * This,
    /* [out] */ SIZE *pOutSize);


void __RPC_STUB IDXGradient_GetOutputSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXGradient_INTERFACE_DEFINED__ */


#ifndef __IDXTScale_INTERFACE_DEFINED__
#define __IDXTScale_INTERFACE_DEFINED__

/* interface IDXTScale */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXTScale;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B39FD742-E139-11d1-9065-00C04FD9189D")
    IDXTScale : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetScales( 
            /* [in] */ float Scales[ 2 ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetScales( 
            /* [out] */ float Scales[ 2 ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ScaleFitToSize( 
            /* [out][in] */ DXBNDS *pClipBounds,
            /* [in] */ SIZE FitToSize,
            /* [in] */ BOOL bMaintainAspect) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTScaleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTScale * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTScale * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTScale * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetScales )( 
            IDXTScale * This,
            /* [in] */ float Scales[ 2 ]);
        
        HRESULT ( STDMETHODCALLTYPE *GetScales )( 
            IDXTScale * This,
            /* [out] */ float Scales[ 2 ]);
        
        HRESULT ( STDMETHODCALLTYPE *ScaleFitToSize )( 
            IDXTScale * This,
            /* [out][in] */ DXBNDS *pClipBounds,
            /* [in] */ SIZE FitToSize,
            /* [in] */ BOOL bMaintainAspect);
        
        END_INTERFACE
    } IDXTScaleVtbl;

    interface IDXTScale
    {
        CONST_VTBL struct IDXTScaleVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTScale_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTScale_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTScale_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTScale_SetScales(This,Scales)	\
    (This)->lpVtbl -> SetScales(This,Scales)

#define IDXTScale_GetScales(This,Scales)	\
    (This)->lpVtbl -> GetScales(This,Scales)

#define IDXTScale_ScaleFitToSize(This,pClipBounds,FitToSize,bMaintainAspect)	\
    (This)->lpVtbl -> ScaleFitToSize(This,pClipBounds,FitToSize,bMaintainAspect)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXTScale_SetScales_Proxy( 
    IDXTScale * This,
    /* [in] */ float Scales[ 2 ]);


void __RPC_STUB IDXTScale_SetScales_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTScale_GetScales_Proxy( 
    IDXTScale * This,
    /* [out] */ float Scales[ 2 ]);


void __RPC_STUB IDXTScale_GetScales_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTScale_ScaleFitToSize_Proxy( 
    IDXTScale * This,
    /* [out][in] */ DXBNDS *pClipBounds,
    /* [in] */ SIZE FitToSize,
    /* [in] */ BOOL bMaintainAspect);


void __RPC_STUB IDXTScale_ScaleFitToSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTScale_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtrans_0276 */
/* [local] */ 

typedef 
enum DISPIDDXEFFECT
    {	DISPID_DXECAPABILITIES	= 10000,
	DISPID_DXEPROGRESS	= DISPID_DXECAPABILITIES + 1,
	DISPID_DXESTEP	= DISPID_DXEPROGRESS + 1,
	DISPID_DXEDURATION	= DISPID_DXESTEP + 1,
	DISPID_DXE_NEXT_ID	= DISPID_DXEDURATION + 1
    } 	DISPIDDXBOUNDEDEFFECT;

typedef 
enum DXEFFECTTYPE
    {	DXTET_PERIODIC	= 1 << 0,
	DXTET_MORPH	= 1 << 1
    } 	DXEFFECTTYPE;



extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0276_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0276_v0_0_s_ifspec;

#ifndef __IDXEffect_INTERFACE_DEFINED__
#define __IDXEffect_INTERFACE_DEFINED__

/* interface IDXEffect */
/* [dual][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXEffect;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E31FB81B-1335-11d1-8189-0000F87557DB")
    IDXEffect : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Capabilities( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Progress( 
            /* [retval][out] */ float *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Progress( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_StepResolution( 
            /* [retval][out] */ float *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Duration( 
            /* [retval][out] */ float *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Duration( 
            /* [in] */ float newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXEffectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXEffect * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXEffect * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXEffect * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDXEffect * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDXEffect * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDXEffect * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDXEffect * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capabilities )( 
            IDXEffect * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            IDXEffect * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            IDXEffect * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StepResolution )( 
            IDXEffect * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IDXEffect * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            IDXEffect * This,
            /* [in] */ float newVal);
        
        END_INTERFACE
    } IDXEffectVtbl;

    interface IDXEffect
    {
        CONST_VTBL struct IDXEffectVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXEffect_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXEffect_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXEffect_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXEffect_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXEffect_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXEffect_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXEffect_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXEffect_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXEffect_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXEffect_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXEffect_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXEffect_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXEffect_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXEffect_get_Capabilities_Proxy( 
    IDXEffect * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IDXEffect_get_Capabilities_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXEffect_get_Progress_Proxy( 
    IDXEffect * This,
    /* [retval][out] */ float *pVal);


void __RPC_STUB IDXEffect_get_Progress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXEffect_put_Progress_Proxy( 
    IDXEffect * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXEffect_put_Progress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXEffect_get_StepResolution_Proxy( 
    IDXEffect * This,
    /* [retval][out] */ float *pVal);


void __RPC_STUB IDXEffect_get_StepResolution_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXEffect_get_Duration_Proxy( 
    IDXEffect * This,
    /* [retval][out] */ float *pVal);


void __RPC_STUB IDXEffect_get_Duration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXEffect_put_Duration_Proxy( 
    IDXEffect * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXEffect_put_Duration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXEffect_INTERFACE_DEFINED__ */


#ifndef __IDXLookupTable_INTERFACE_DEFINED__
#define __IDXLookupTable_INTERFACE_DEFINED__

/* interface IDXLookupTable */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXLookupTable;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("01BAFC7F-9E63-11d1-9053-00C04FD9189D")
    IDXLookupTable : public IDXBaseObject
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetTables( 
            /* [out] */ BYTE RedLUT[ 256 ],
            /* [out] */ BYTE GreenLUT[ 256 ],
            /* [out] */ BYTE BlueLUT[ 256 ],
            /* [out] */ BYTE AlphaLUT[ 256 ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsChannelIdentity( 
            /* [out] */ DXBASESAMPLE *pSampleBools) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIndexValues( 
            /* [in] */ ULONG Index,
            /* [out] */ DXBASESAMPLE *pSample) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ApplyTables( 
            /* [out][in] */ DXSAMPLE *pSamples,
            /* [in] */ ULONG cSamples) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXLookupTableVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXLookupTable * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXLookupTable * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXLookupTable * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetGenerationId )( 
            IDXLookupTable * This,
            /* [out] */ ULONG *pID);
        
        HRESULT ( STDMETHODCALLTYPE *IncrementGenerationId )( 
            IDXLookupTable * This,
            /* [in] */ BOOL bRefresh);
        
        HRESULT ( STDMETHODCALLTYPE *GetObjectSize )( 
            IDXLookupTable * This,
            /* [out] */ ULONG *pcbSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetTables )( 
            IDXLookupTable * This,
            /* [out] */ BYTE RedLUT[ 256 ],
            /* [out] */ BYTE GreenLUT[ 256 ],
            /* [out] */ BYTE BlueLUT[ 256 ],
            /* [out] */ BYTE AlphaLUT[ 256 ]);
        
        HRESULT ( STDMETHODCALLTYPE *IsChannelIdentity )( 
            IDXLookupTable * This,
            /* [out] */ DXBASESAMPLE *pSampleBools);
        
        HRESULT ( STDMETHODCALLTYPE *GetIndexValues )( 
            IDXLookupTable * This,
            /* [in] */ ULONG Index,
            /* [out] */ DXBASESAMPLE *pSample);
        
        HRESULT ( STDMETHODCALLTYPE *ApplyTables )( 
            IDXLookupTable * This,
            /* [out][in] */ DXSAMPLE *pSamples,
            /* [in] */ ULONG cSamples);
        
        END_INTERFACE
    } IDXLookupTableVtbl;

    interface IDXLookupTable
    {
        CONST_VTBL struct IDXLookupTableVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXLookupTable_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXLookupTable_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXLookupTable_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXLookupTable_GetGenerationId(This,pID)	\
    (This)->lpVtbl -> GetGenerationId(This,pID)

#define IDXLookupTable_IncrementGenerationId(This,bRefresh)	\
    (This)->lpVtbl -> IncrementGenerationId(This,bRefresh)

#define IDXLookupTable_GetObjectSize(This,pcbSize)	\
    (This)->lpVtbl -> GetObjectSize(This,pcbSize)


#define IDXLookupTable_GetTables(This,RedLUT,GreenLUT,BlueLUT,AlphaLUT)	\
    (This)->lpVtbl -> GetTables(This,RedLUT,GreenLUT,BlueLUT,AlphaLUT)

#define IDXLookupTable_IsChannelIdentity(This,pSampleBools)	\
    (This)->lpVtbl -> IsChannelIdentity(This,pSampleBools)

#define IDXLookupTable_GetIndexValues(This,Index,pSample)	\
    (This)->lpVtbl -> GetIndexValues(This,Index,pSample)

#define IDXLookupTable_ApplyTables(This,pSamples,cSamples)	\
    (This)->lpVtbl -> ApplyTables(This,pSamples,cSamples)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXLookupTable_GetTables_Proxy( 
    IDXLookupTable * This,
    /* [out] */ BYTE RedLUT[ 256 ],
    /* [out] */ BYTE GreenLUT[ 256 ],
    /* [out] */ BYTE BlueLUT[ 256 ],
    /* [out] */ BYTE AlphaLUT[ 256 ]);


void __RPC_STUB IDXLookupTable_GetTables_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLookupTable_IsChannelIdentity_Proxy( 
    IDXLookupTable * This,
    /* [out] */ DXBASESAMPLE *pSampleBools);


void __RPC_STUB IDXLookupTable_IsChannelIdentity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLookupTable_GetIndexValues_Proxy( 
    IDXLookupTable * This,
    /* [in] */ ULONG Index,
    /* [out] */ DXBASESAMPLE *pSample);


void __RPC_STUB IDXLookupTable_GetIndexValues_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLookupTable_ApplyTables_Proxy( 
    IDXLookupTable * This,
    /* [out][in] */ DXSAMPLE *pSamples,
    /* [in] */ ULONG cSamples);


void __RPC_STUB IDXLookupTable_ApplyTables_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXLookupTable_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtrans_0278 */
/* [local] */ 

typedef struct DXRAWSURFACEINFO
    {
    BYTE *pFirstByte;
    long lPitch;
    ULONG Width;
    ULONG Height;
    const GUID *pPixelFormat;
    HDC hdc;
    DWORD dwColorKey;
    DXBASESAMPLE *pPalette;
    } 	DXRAWSURFACEINFO;



extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0278_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0278_v0_0_s_ifspec;

#ifndef __IDXRawSurface_INTERFACE_DEFINED__
#define __IDXRawSurface_INTERFACE_DEFINED__

/* interface IDXRawSurface */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXRawSurface;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("09756C8A-D96A-11d1-9062-00C04FD9189D")
    IDXRawSurface : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSurfaceInfo( 
            DXRAWSURFACEINFO *pSurfaceInfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXRawSurfaceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXRawSurface * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXRawSurface * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXRawSurface * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSurfaceInfo )( 
            IDXRawSurface * This,
            DXRAWSURFACEINFO *pSurfaceInfo);
        
        END_INTERFACE
    } IDXRawSurfaceVtbl;

    interface IDXRawSurface
    {
        CONST_VTBL struct IDXRawSurfaceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXRawSurface_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXRawSurface_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXRawSurface_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXRawSurface_GetSurfaceInfo(This,pSurfaceInfo)	\
    (This)->lpVtbl -> GetSurfaceInfo(This,pSurfaceInfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXRawSurface_GetSurfaceInfo_Proxy( 
    IDXRawSurface * This,
    DXRAWSURFACEINFO *pSurfaceInfo);


void __RPC_STUB IDXRawSurface_GetSurfaceInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXRawSurface_INTERFACE_DEFINED__ */


#ifndef __IHTMLDXTransform_INTERFACE_DEFINED__
#define __IHTMLDXTransform_INTERFACE_DEFINED__

/* interface IHTMLDXTransform */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IHTMLDXTransform;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("30E2AB7D-4FDD-4159-B7EA-DC722BF4ADE5")
    IHTMLDXTransform : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetHostUrl( 
            BSTR bstrHostUrl) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHTMLDXTransformVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHTMLDXTransform * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHTMLDXTransform * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHTMLDXTransform * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetHostUrl )( 
            IHTMLDXTransform * This,
            BSTR bstrHostUrl);
        
        END_INTERFACE
    } IHTMLDXTransformVtbl;

    interface IHTMLDXTransform
    {
        CONST_VTBL struct IHTMLDXTransformVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHTMLDXTransform_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHTMLDXTransform_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHTMLDXTransform_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHTMLDXTransform_SetHostUrl(This,bstrHostUrl)	\
    (This)->lpVtbl -> SetHostUrl(This,bstrHostUrl)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHTMLDXTransform_SetHostUrl_Proxy( 
    IHTMLDXTransform * This,
    BSTR bstrHostUrl);


void __RPC_STUB IHTMLDXTransform_SetHostUrl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHTMLDXTransform_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtrans_0280 */
/* [local] */ 

typedef 
enum DXTFILTER_STATUS
    {	DXTFILTER_STATUS_Stopped	= 0,
	DXTFILTER_STATUS_Applied	= DXTFILTER_STATUS_Stopped + 1,
	DXTFILTER_STATUS_Playing	= DXTFILTER_STATUS_Applied + 1,
	DXTFILTER_STATUS_MAX	= DXTFILTER_STATUS_Playing + 1
    } 	DXTFILTER_STATUS;

typedef 
enum DXTFILTER_DISPID
    {	DISPID_DXTFilter_Percent	= 1,
	DISPID_DXTFilter_Duration	= DISPID_DXTFilter_Percent + 1,
	DISPID_DXTFilter_Enabled	= DISPID_DXTFilter_Duration + 1,
	DISPID_DXTFilter_Status	= DISPID_DXTFilter_Enabled + 1,
	DISPID_DXTFilter_Apply	= DISPID_DXTFilter_Status + 1,
	DISPID_DXTFilter_Play	= DISPID_DXTFilter_Apply + 1,
	DISPID_DXTFilter_Stop	= DISPID_DXTFilter_Play + 1,
	DISPID_DXTFilter_MAX	= DISPID_DXTFilter_Stop + 1
    } 	DXTFILTER_DISPID;



extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0280_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0280_v0_0_s_ifspec;

#ifndef __ICSSFilterDispatch_INTERFACE_DEFINED__
#define __ICSSFilterDispatch_INTERFACE_DEFINED__

/* interface ICSSFilterDispatch */
/* [dual][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ICSSFilterDispatch;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9519152B-9484-4A6C-B6A7-4F25E92D6C6B")
    ICSSFilterDispatch : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Percent( 
            /* [retval][out] */ float *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Percent( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Duration( 
            /* [retval][out] */ float *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Duration( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Enabled( 
            /* [retval][out] */ VARIANT_BOOL *pfVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Enabled( 
            /* [in] */ VARIANT_BOOL fVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Status( 
            /* [retval][out] */ DXTFILTER_STATUS *peVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Apply( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Play( 
            /* [optional][in] */ VARIANT varDuration) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICSSFilterDispatchVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICSSFilterDispatch * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICSSFilterDispatch * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICSSFilterDispatch * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICSSFilterDispatch * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICSSFilterDispatch * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICSSFilterDispatch * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICSSFilterDispatch * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Percent )( 
            ICSSFilterDispatch * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Percent )( 
            ICSSFilterDispatch * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            ICSSFilterDispatch * This,
            /* [retval][out] */ float *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Duration )( 
            ICSSFilterDispatch * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Enabled )( 
            ICSSFilterDispatch * This,
            /* [retval][out] */ VARIANT_BOOL *pfVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Enabled )( 
            ICSSFilterDispatch * This,
            /* [in] */ VARIANT_BOOL fVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            ICSSFilterDispatch * This,
            /* [retval][out] */ DXTFILTER_STATUS *peVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Apply )( 
            ICSSFilterDispatch * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Play )( 
            ICSSFilterDispatch * This,
            /* [optional][in] */ VARIANT varDuration);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Stop )( 
            ICSSFilterDispatch * This);
        
        END_INTERFACE
    } ICSSFilterDispatchVtbl;

    interface ICSSFilterDispatch
    {
        CONST_VTBL struct ICSSFilterDispatchVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICSSFilterDispatch_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICSSFilterDispatch_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICSSFilterDispatch_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICSSFilterDispatch_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICSSFilterDispatch_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICSSFilterDispatch_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICSSFilterDispatch_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICSSFilterDispatch_get_Percent(This,pVal)	\
    (This)->lpVtbl -> get_Percent(This,pVal)

#define ICSSFilterDispatch_put_Percent(This,newVal)	\
    (This)->lpVtbl -> put_Percent(This,newVal)

#define ICSSFilterDispatch_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define ICSSFilterDispatch_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)

#define ICSSFilterDispatch_get_Enabled(This,pfVal)	\
    (This)->lpVtbl -> get_Enabled(This,pfVal)

#define ICSSFilterDispatch_put_Enabled(This,fVal)	\
    (This)->lpVtbl -> put_Enabled(This,fVal)

#define ICSSFilterDispatch_get_Status(This,peVal)	\
    (This)->lpVtbl -> get_Status(This,peVal)

#define ICSSFilterDispatch_Apply(This)	\
    (This)->lpVtbl -> Apply(This)

#define ICSSFilterDispatch_Play(This,varDuration)	\
    (This)->lpVtbl -> Play(This,varDuration)

#define ICSSFilterDispatch_Stop(This)	\
    (This)->lpVtbl -> Stop(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICSSFilterDispatch_get_Percent_Proxy( 
    ICSSFilterDispatch * This,
    /* [retval][out] */ float *pVal);


void __RPC_STUB ICSSFilterDispatch_get_Percent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICSSFilterDispatch_put_Percent_Proxy( 
    ICSSFilterDispatch * This,
    /* [in] */ float newVal);


void __RPC_STUB ICSSFilterDispatch_put_Percent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICSSFilterDispatch_get_Duration_Proxy( 
    ICSSFilterDispatch * This,
    /* [retval][out] */ float *pVal);


void __RPC_STUB ICSSFilterDispatch_get_Duration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICSSFilterDispatch_put_Duration_Proxy( 
    ICSSFilterDispatch * This,
    /* [in] */ float newVal);


void __RPC_STUB ICSSFilterDispatch_put_Duration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICSSFilterDispatch_get_Enabled_Proxy( 
    ICSSFilterDispatch * This,
    /* [retval][out] */ VARIANT_BOOL *pfVal);


void __RPC_STUB ICSSFilterDispatch_get_Enabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICSSFilterDispatch_put_Enabled_Proxy( 
    ICSSFilterDispatch * This,
    /* [in] */ VARIANT_BOOL fVal);


void __RPC_STUB ICSSFilterDispatch_put_Enabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICSSFilterDispatch_get_Status_Proxy( 
    ICSSFilterDispatch * This,
    /* [retval][out] */ DXTFILTER_STATUS *peVal);


void __RPC_STUB ICSSFilterDispatch_get_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ICSSFilterDispatch_Apply_Proxy( 
    ICSSFilterDispatch * This);


void __RPC_STUB ICSSFilterDispatch_Apply_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ICSSFilterDispatch_Play_Proxy( 
    ICSSFilterDispatch * This,
    /* [optional][in] */ VARIANT varDuration);


void __RPC_STUB ICSSFilterDispatch_Play_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ICSSFilterDispatch_Stop_Proxy( 
    ICSSFilterDispatch * This);


void __RPC_STUB ICSSFilterDispatch_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICSSFilterDispatch_INTERFACE_DEFINED__ */



#ifndef __DXTRANSLib_LIBRARY_DEFINED__
#define __DXTRANSLib_LIBRARY_DEFINED__

/* library DXTRANSLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_DXTRANSLib;

EXTERN_C const CLSID CLSID_DXTransformFactory;

#ifdef __cplusplus

class DECLSPEC_UUID("D1FE6762-FC48-11D0-883A-3C8B00C10000")
DXTransformFactory;
#endif

EXTERN_C const CLSID CLSID_DXTaskManager;

#ifdef __cplusplus

class DECLSPEC_UUID("4CB26C03-FF93-11d0-817E-0000F87557DB")
DXTaskManager;
#endif

EXTERN_C const CLSID CLSID_DXTScale;

#ifdef __cplusplus

class DECLSPEC_UUID("555278E2-05DB-11D1-883A-3C8B00C10000")
DXTScale;
#endif

EXTERN_C const CLSID CLSID_DXSurface;

#ifdef __cplusplus

class DECLSPEC_UUID("0E890F83-5F79-11D1-9043-00C04FD9189D")
DXSurface;
#endif

EXTERN_C const CLSID CLSID_DXSurfaceModifier;

#ifdef __cplusplus

class DECLSPEC_UUID("3E669F1D-9C23-11d1-9053-00C04FD9189D")
DXSurfaceModifier;
#endif

EXTERN_C const CLSID CLSID_DXGradient;

#ifdef __cplusplus

class DECLSPEC_UUID("C6365470-F667-11d1-9067-00C04FD9189D")
DXGradient;
#endif

EXTERN_C const CLSID CLSID_DXTFilter;

#ifdef __cplusplus

class DECLSPEC_UUID("385A91BC-1E8A-4e4a-A7A6-F4FC1E6CA1BD")
DXTFilter;
#endif
#endif /* __DXTRANSLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long *, VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


