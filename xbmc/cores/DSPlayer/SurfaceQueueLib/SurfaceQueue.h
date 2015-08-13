// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* Compiler settings for surfacequeue.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 7.00.0555 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 500
#endif

/* verify that the <rpcsal.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCSAL_H_VERSION__
#define __REQUIRED_RPCSAL_H_VERSION__ 100
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

#ifndef __surfacequeue_h__
#define __surfacequeue_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ISurfaceProducer_FWD_DEFINED__
#define __ISurfaceProducer_FWD_DEFINED__
typedef interface ISurfaceProducer ISurfaceProducer;
#endif 	/* __ISurfaceProducer_FWD_DEFINED__ */


#ifndef __ISurfaceConsumer_FWD_DEFINED__
#define __ISurfaceConsumer_FWD_DEFINED__
typedef interface ISurfaceConsumer ISurfaceConsumer;
#endif 	/* __ISurfaceConsumer_FWD_DEFINED__ */


#ifndef __ISurfaceQueue_FWD_DEFINED__
#define __ISurfaceQueue_FWD_DEFINED__
typedef interface ISurfaceQueue ISurfaceQueue;
#endif 	/* __ISurfaceQueue_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "dxgitype.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_surfacequeue_0000_0000 */
/* [local] */ 

typedef struct SURFACE_QUEUE_DESC
    {
    UINT Width;
    UINT Height;
    DXGI_FORMAT Format;
    UINT NumSurfaces;
    UINT MetaDataSize;
    DWORD Flags;
    } 	SURFACE_QUEUE_DESC;

typedef struct SURFACE_QUEUE_CLONE_DESC
    {
    UINT MetaDataSize;
    DWORD Flags;
    } 	SURFACE_QUEUE_CLONE_DESC;

typedef 
enum SURFACE_QUEUE_FLAG
    {	SURFACE_QUEUE_FLAG_DO_NOT_WAIT	= 0x1L,
	SURFACE_QUEUE_FLAG_SINGLE_THREADED	= 0x2L
    } 	SURFACE_QUEUE_FLAG;



extern RPC_IF_HANDLE __MIDL_itf_surfacequeue_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_surfacequeue_0000_0000_v0_0_s_ifspec;

#ifndef __ISurfaceProducer_INTERFACE_DEFINED__
#define __ISurfaceProducer_INTERFACE_DEFINED__

/* interface ISurfaceProducer */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_ISurfaceProducer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B8B0B73B-79C1-4446-BB8A-19595018B0B7")
    ISurfaceProducer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Enqueue( 
            /* [in] */ IUnknown *pSurface,
            /* [in] */ void *pBuffer,
            /* [in] */ UINT BufferSize,
            /* [in] */ DWORD Flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Flush( 
            /* [in] */ DWORD Flags,
            /* [out] */ UINT *NumSurfaces) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISurfaceProducerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISurfaceProducer * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISurfaceProducer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISurfaceProducer * This);
        
        HRESULT ( STDMETHODCALLTYPE *Enqueue )( 
            ISurfaceProducer * This,
            /* [in] */ IUnknown *pSurface,
            /* [in] */ void *pBuffer,
            /* [in] */ UINT BufferSize,
            /* [in] */ DWORD Flags);
        
        HRESULT ( STDMETHODCALLTYPE *Flush )( 
            ISurfaceProducer * This,
            /* [in] */ DWORD Flags,
            /* [out] */ UINT *NumSurfaces);
        
        END_INTERFACE
    } ISurfaceProducerVtbl;

    interface ISurfaceProducer
    {
        CONST_VTBL struct ISurfaceProducerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISurfaceProducer_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ISurfaceProducer_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ISurfaceProducer_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ISurfaceProducer_Enqueue(This,pSurface,pBuffer,BufferSize,Flags)	\
    ( (This)->lpVtbl -> Enqueue(This,pSurface,pBuffer,BufferSize,Flags) ) 

#define ISurfaceProducer_Flush(This,Flags,NumSurfaces)	\
    ( (This)->lpVtbl -> Flush(This,Flags,NumSurfaces) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ISurfaceProducer_INTERFACE_DEFINED__ */


#ifndef __ISurfaceConsumer_INTERFACE_DEFINED__
#define __ISurfaceConsumer_INTERFACE_DEFINED__

/* interface ISurfaceConsumer */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_ISurfaceConsumer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("97E305E1-1EC7-41a6-972C-99092DE6A31E")
    ISurfaceConsumer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Dequeue( 
            /* [in] */ REFIID id,
            /* [out] */ void **ppSurface,
            /* [out] */ void *pBuffer,
            /* [out][in] */ UINT *pBufferSize,
            /* [in] */ DWORD dwTimeout) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISurfaceConsumerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISurfaceConsumer * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISurfaceConsumer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISurfaceConsumer * This);
        
        HRESULT ( STDMETHODCALLTYPE *Dequeue )( 
            ISurfaceConsumer * This,
            /* [in] */ REFIID id,
            /* [out] */ void **ppSurface,
            /* [out] */ void *pBuffer,
            /* [out][in] */ UINT *pBufferSize,
            /* [in] */ DWORD dwTimeout);
        
        END_INTERFACE
    } ISurfaceConsumerVtbl;

    interface ISurfaceConsumer
    {
        CONST_VTBL struct ISurfaceConsumerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISurfaceConsumer_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ISurfaceConsumer_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ISurfaceConsumer_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ISurfaceConsumer_Dequeue(This,id,ppSurface,pBuffer,pBufferSize,dwTimeout)	\
    ( (This)->lpVtbl -> Dequeue(This,id,ppSurface,pBuffer,pBufferSize,dwTimeout) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ISurfaceConsumer_INTERFACE_DEFINED__ */


#ifndef __ISurfaceQueue_INTERFACE_DEFINED__
#define __ISurfaceQueue_INTERFACE_DEFINED__

/* interface ISurfaceQueue */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_ISurfaceQueue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1C08437F-48DF-467e-8D55-CA9268C73779")
    ISurfaceQueue : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OpenProducer( 
            /* [in] */ IUnknown *pDevice,
            /* [out] */ ISurfaceProducer **ppProducer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenConsumer( 
            /* [in] */ IUnknown *pDevice,
            /* [out] */ ISurfaceConsumer **ppConsumer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [in] */ SURFACE_QUEUE_CLONE_DESC *pDesc,
            /* [out] */ ISurfaceQueue **ppQueue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISurfaceQueueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISurfaceQueue * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISurfaceQueue * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISurfaceQueue * This);
        
        HRESULT ( STDMETHODCALLTYPE *OpenProducer )( 
            ISurfaceQueue * This,
            /* [in] */ IUnknown *pDevice,
            /* [out] */ ISurfaceProducer **ppProducer);
        
        HRESULT ( STDMETHODCALLTYPE *OpenConsumer )( 
            ISurfaceQueue * This,
            /* [in] */ IUnknown *pDevice,
            /* [out] */ ISurfaceConsumer **ppConsumer);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            ISurfaceQueue * This,
            /* [in] */ SURFACE_QUEUE_CLONE_DESC *pDesc,
            /* [out] */ ISurfaceQueue **ppQueue);
        
        END_INTERFACE
    } ISurfaceQueueVtbl;

    interface ISurfaceQueue
    {
        CONST_VTBL struct ISurfaceQueueVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISurfaceQueue_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ISurfaceQueue_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ISurfaceQueue_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ISurfaceQueue_OpenProducer(This,pDevice,ppProducer)	\
    ( (This)->lpVtbl -> OpenProducer(This,pDevice,ppProducer) ) 

#define ISurfaceQueue_OpenConsumer(This,pDevice,ppConsumer)	\
    ( (This)->lpVtbl -> OpenConsumer(This,pDevice,ppConsumer) ) 

#define ISurfaceQueue_Clone(This,pDesc,ppQueue)	\
    ( (This)->lpVtbl -> Clone(This,pDesc,ppQueue) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ISurfaceQueue_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_surfacequeue_0000_0003 */
/* [local] */ 

HRESULT WINAPI CreateSurfaceQueue( SURFACE_QUEUE_DESC*  pDesc,
                                   IUnknown*            pDevice,
                                   ISurfaceQueue**      ppQueue);


extern RPC_IF_HANDLE __MIDL_itf_surfacequeue_0000_0003_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_surfacequeue_0000_0003_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


