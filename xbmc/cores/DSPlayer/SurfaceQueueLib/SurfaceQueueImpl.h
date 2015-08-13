// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#include <d3d9.h>
#include <D3D10_1.h>
#include <D3D11.h>

#include "surfacequeue.h"

#include <assert.h>
#define ASSERT(x) assert(x);

//
// This library doesn’t use exceptions, and can be used in C++ environments with or without exception support.  
// It uses a non-throwing new that returns null on out-of-memory.  The user of the library MUST provide a define 
// to tell it whether the C++ environment provides a standard throwing new.  In standard-compliant environments 
// define QUEUE_USE_CONFORMANT_NEW (the library will specify std::nothrow to opt for a non-throwing new.)  In 
// environments where new doesn’t throw by default define QUEUE_USE_OLD_NEW.
//
// WARNING: If  you define QUEUE_USE_OLD_NEW and use the library with a throwing new the library will have undefined behavior.
//
// For the default Visual Studio solution, QUEUE_USE_CONFORMANT_NEW has been defined.
//
#if defined(QUEUE_USE_OLD_NEW)        
#define QUEUE_NOTHROW_SPECIFIER 
#elif defined (QUEUE_USE_CONFORMANT_NEW)
#define QUEUE_NOTHROW_SPECIFIER (std::nothrow) 
#else
#error "Please select operator new for out-of-memory exception behavior."
#endif


//
// This defines the size of staging resource.  The purpose of the staging resource is
// so we can copy & lock as a way to wait for rendering to complete.  We ideally, want 
// to copy to a 1x1 staging texture but because of various driver bugs, it is more reliable
// to use a slightly bigger texture (16x16).
//
#define SHARED_SURFACE_COPY_SIZE (16)

/****************************************************************************************\
 *
 *  Implementation
 *
\****************************************************************************************/
class CSurfaceConsumer;
class CSurfaceProducer;
class CSurfaceQueue;

// Interface to abstract away different runtime devices.  Each of the runtimes will
// have a wrapper that implements this interface.  This interface contains a small
// subset of the public APIs that the queue needs.
class ISurfaceQueueDevice
{
    public:
        virtual HRESULT CreateSharedSurface(UINT Width, UINT Height, 
                                            DXGI_FORMAT format, 
                                            IUnknown** ppSurface,
                                            HANDLE* handle) = 0;

        // The surface queue is designed to only share 2D textures.  
        //    D3D9 will be expecting IDirect3DTexture9
        //    D3D10 will be expecting ID3D10Texture2D
        //    D3D11 will be expecting ID3D11Texture2D 
        virtual BOOL ValidateREFIID(REFIID) = 0;

        // Opens a shared surface with the given handle.
        virtual HRESULT OpenSurface(HANDLE, void**, UINT w, UINT h, DXGI_FORMAT) = 0;

        // Returns the shared handle for a queue surface.  This lets the queue
        // validate that the surface the user provided is one owned by the queue.
        virtual HRESULT GetSharedHandle(IUnknown*, HANDLE*) = 0;

        // Creates a staging resource that will be used for the synchronization.
        virtual HRESULT CreateCopyResource(DXGI_FORMAT, UINT width, UINT height, IUnknown** pRes) = 0;
       
        // Copy from the queue surface to the staging resource.
        virtual HRESULT CopySurface(IUnknown* pDst, IUnknown* pSrc, UINT width, UINT height) = 0;

        // Locks the (staging) surface.  When this call completes, the surface
        // has been flushed and is ready to be used by another device
        virtual HRESULT LockSurface(IUnknown* pSurface, DWORD flags) = 0;

        // Unlocks the (staging) surface.
        virtual HRESULT UnlockSurface(IUnknown* pSurface) = 0;

        // The wrapper maintins a refence to the underlying I*Device.
        virtual ~ISurfaceQueueDevice() {};
};

// Implementation of SurfaceQueueDevice for D3D9Ex
class CSurfaceQueueDeviceD3D9 : public ISurfaceQueueDevice
{
    public:
        HRESULT CreateSharedSurface(UINT Width, UINT Height, 
                                    DXGI_FORMAT format, 
                                    IUnknown** ppSurface,
                                    HANDLE* handle);
        BOOL ValidateREFIID(REFIID);
        HRESULT OpenSurface(HANDLE, void**, UINT Width, UINT Height, DXGI_FORMAT format);
        HRESULT GetSharedHandle(IUnknown*, HANDLE*);
        HRESULT CreateCopyResource(DXGI_FORMAT, UINT width, UINT height, IUnknown** pRes);

        HRESULT CopySurface(IUnknown* pDst, IUnknown* pSrc, UINT width, UINT height);
        HRESULT LockSurface(IUnknown* pSurface, DWORD flags);
        HRESULT UnlockSurface(IUnknown* pSurface);

        CSurfaceQueueDeviceD3D9(IDirect3DDevice9Ex* pD3D9Device);
        ~CSurfaceQueueDeviceD3D9();

    private:
        IDirect3DDevice9Ex*     m_pDevice;
};

// Implementation of SurfaceQueueDevice for D3D10
class CSurfaceQueueDeviceD3D10 : public ISurfaceQueueDevice
{
    public:
        HRESULT CreateSharedSurface(UINT Width, UINT Height, 
                                    DXGI_FORMAT format, 
                                    IUnknown** ppSurface,
                                    HANDLE* handle);
        BOOL ValidateREFIID(REFIID);
        HRESULT OpenSurface(HANDLE, void**, UINT w, UINT h, DXGI_FORMAT);
        HRESULT GetSharedHandle(IUnknown*, HANDLE*);
        HRESULT CreateCopyResource(DXGI_FORMAT, UINT width, UINT height, IUnknown** pRes);

        HRESULT CopySurface(IUnknown* pDst, IUnknown* pSrc, UINT width, UINT height);
        HRESULT LockSurface(IUnknown* pSurface, DWORD flags);
        HRESULT UnlockSurface(IUnknown* pSurface);

        CSurfaceQueueDeviceD3D10(ID3D10Device* pD3D10Device);
        ~CSurfaceQueueDeviceD3D10();

    private:
        ID3D10Device*           m_pDevice;
};

// Implementation of SurfaceQueueDevice for D3D11
class CSurfaceQueueDeviceD3D11 : public ISurfaceQueueDevice
{
    public:
        HRESULT CreateSharedSurface(UINT Width, UINT Height, 
                                    DXGI_FORMAT format, 
                                    IUnknown** ppSurface,
                                    HANDLE* handle);
        BOOL ValidateREFIID(REFIID);
        HRESULT OpenSurface(HANDLE, void**, UINT w, UINT h, DXGI_FORMAT);
        HRESULT GetSharedHandle(IUnknown*, HANDLE*);
        HRESULT CreateCopyResource(DXGI_FORMAT, UINT width, UINT height, IUnknown** pRes);

        HRESULT CopySurface(IUnknown* pDst, IUnknown* pSrc, UINT width, UINT height);
        HRESULT LockSurface(IUnknown* pSurface, DWORD flags);
        HRESULT UnlockSurface(IUnknown* pSurface);

        CSurfaceQueueDeviceD3D11(ID3D11Device* pD3D11Device);
        ~CSurfaceQueueDeviceD3D11();

    private:
        ID3D11Device*           m_pDevice;
};

enum SharedSurfaceState
{
    SHARED_SURFACE_STATE_UNINITIALIZED = 0,
    SHARED_SURFACE_STATE_DEQUEUED,
    SHARED_SURFACE_STATE_ENQUEUED,
    SHARED_SURFACE_STATE_FLUSHED,
};

// This object is shared between all queues in a network and maintains state 
// about the surface.  
struct SharedSurfaceObject
{
    HANDLE                      hSharedHandle;
    SharedSurfaceState          state; 

    UINT                        width;
    UINT                        height;
    DXGI_FORMAT                 format;

    // Tracks which queue or device currently is using the surface
    union
    {
        ISurfaceQueue*          queue;
        ISurfaceQueueDevice*    device;
    };
    IUnknown*                   pSurface;

    SharedSurfaceObject(UINT Width, UINT Height, DXGI_FORMAT format);
    ~SharedSurfaceObject();
};

class CSurfaceConsumer : public ISurfaceConsumer
{
    // Com Interfaces
    public:
        STDMETHOD(  QueryInterface) (REFIID ID, void** ppInterface);
        STDMETHOD_( ULONG, AddRef)();
        STDMETHOD_( ULONG, Release)();

    // Public Interfaces
    public:
        STDMETHOD (Dequeue) (
                                REFIID id,
                                void** ppSurface,
                                void*  pBuffer,
                                UINT*  BufferSize,
                                DWORD  dwTimeout 
                            );
    // Implementation
    public:
        CSurfaceConsumer(BOOL IsMultithreaded);
        ~CSurfaceConsumer();

        HRESULT Initialize(IUnknown* pDevice);
        void SetQueue(CSurfaceQueue*);

        ISurfaceQueueDevice* GetDevice() { return m_pDevice; }
    
    private:
        
        LONG                                m_RefCount;

        BOOL                                m_IsMultithreaded;
        
        // Weak reference to the queue this is part of
        CSurfaceQueue*                      m_pQueue;

        // The device this was opened with
        ISurfaceQueueDevice*                m_pDevice;
        
        // Critical Section for the consumer
        CRITICAL_SECTION                    m_lock;

};

class CSurfaceProducer : public ISurfaceProducer
{
    // Com Interfaces
    public:
        STDMETHOD(  QueryInterface) (REFIID ID, void** ppInterface);
        STDMETHOD_( ULONG, AddRef)();
        STDMETHOD_( ULONG, Release)();
    
    // Public Interfaces
    public:
        STDMETHOD (Enqueue) ( 
                                IUnknown* pSurface,
                                void*     pBuffer,
                                UINT      BufferSize,
                                DWORD     Flags 
                            );

        STDMETHOD (Flush)   (
                                DWORD     Flags,
                                UINT*     NumSurfaces
                            );

    // Implementation
    public:
        CSurfaceProducer(BOOL IsMultithreaded);
        ~CSurfaceProducer();

        HRESULT Initialize(IUnknown* pDevice, UINT uNumSurfaces, SURFACE_QUEUE_DESC* queueDesc);
        void SetQueue(CSurfaceQueue*);
        
        ISurfaceQueueDevice* GetDevice() { return m_pDevice; }

    private:
        LONG                        m_RefCount;       

        BOOL                        m_IsMultithreaded;        

        // Reference to the queue this is part of
        CSurfaceQueue*              m_pQueue;

        // The producer device
        ISurfaceQueueDevice*        m_pDevice;
        
        // Critical Section for the producer
        CRITICAL_SECTION            m_lock;

        // Circular buffer of staging resources
        UINT                        m_nStagingResources;
        IUnknown**                  m_pStagingResources;

        // Size of staging resource
        UINT                        m_uiStagingResourceWidth;
        UINT                        m_uiStagingResourceHeight;

        // Index of current staging resource to use
        UINT                        m_iCurrentResource;
};

class CSurfaceQueue : public ISurfaceQueue
{
    // Com Functions
    public:
        STDMETHOD(  QueryInterface) (REFIID ID, void** ppInterface);
        STDMETHOD_( ULONG, AddRef)();
        STDMETHOD_( ULONG, Release)();

    // ISurfaceQueue functions
    public:
        STDMETHOD (OpenProducer) (
                                    IUnknown*                   pDevice,
                                    ISurfaceProducer**          ppProducer
                                 );

        STDMETHOD (OpenConsumer) (
                                    IUnknown*                   pDevice,
                                    ISurfaceConsumer**          ppConsumer
                                 );

        STDMETHOD (Clone)        (   
                                    SURFACE_QUEUE_CLONE_DESC*   pDesc,
                                    ISurfaceQueue**             ppQueue 
                                 );
    
    // Implementation Functions
    public:
        CSurfaceQueue();
        ~CSurfaceQueue();

        // Initializes the queue.  Creates the surfaces, initializes the synchronization code
        HRESULT Initialize(SURFACE_QUEUE_DESC*, IUnknown*, CSurfaceQueue*);

        // Removes the producer device
        void RemoveProducer();

        // Removes the consumer device.  
        void RemoveConsumer();

        HRESULT Enqueue(
                            IUnknown*   pSurface, 
                            void*       pBuffer, 
                            UINT        BufferSize, 
                            DWORD       Flags,
                            IUnknown*   pStagingResource,
                            UINT        width,
                            UINT        height        
                        );

        HRESULT Dequeue(
                            void**      ppSurface,
                            void*       pBuffer,
                            UINT*       BufferSize,
                            DWORD       dwTimeout  
                        );

        HRESULT Flush(
                            DWORD       Flags,
                            UINT*       NumSurfaces
                        );

    private:
        struct SharedSurfaceQueueEntry
        {
            SharedSurfaceObject*    surface;
            BYTE*                   pMetaData;
            UINT                    bMetaDataSize;
            IUnknown*               pStagingResource;

            SharedSurfaceQueueEntry()
            {
                surface             = NULL;
                pMetaData           = NULL;
                bMetaDataSize       = 0;
                pStagingResource    = NULL;
            }
        };

        struct SharedSurfaceOpenedMapping
        {
            SharedSurfaceObject*    pObject;
            IUnknown*               pSurface;
        };

    private:
        void Destroy();

        HRESULT CreateSurfaces();
        void CopySurfaceReferences(CSurfaceQueue*);
        HRESULT AllocateMetaDataBuffers();

        ISurfaceQueueDevice* GetCreatorDevice();

        UINT GetNumQueuesInNetwork(); 
        UINT AddQueueToNetwork(); 
        UINT RemoveQueueFromNetwork();

        void Dequeue(SharedSurfaceQueueEntry& entry);
        void Enqueue(SharedSurfaceQueueEntry& entry);
        void Front(SharedSurfaceQueueEntry& entry);

        SharedSurfaceObject* GetSurfaceObjectFromHandle(HANDLE h);
        IUnknown* GetOpenedSurface(const SharedSurfaceObject*) const;

    private:
        LONG                                    m_RefCount;

        BOOL                                    m_IsMultithreaded;

        // Synchronization object to handle concurrent dequeues and enqueues
        // For the single threaded case, we can keep track of the number of
        // availible surfaces to help the user prevent hanging on an empty
        // queue.
        union
        {
            HANDLE                              m_hSemaphore;
	    UINT				m_nFlushedSurfaces;
	};

        // Refernce to the source queue object
        CSurfaceQueue*                          m_pRootQueue;

        // Number of Queue objects in the network - only stored in root queue
        volatile LONG                           m_NumQueuesInNetwork;

        // References to producer and consumer objects
        CSurfaceConsumer*                       m_pConsumer;
        CSurfaceProducer*                       m_pProducer;

        // Reference to the creating device
        ISurfaceQueueDevice*                    m_pCreator;
        
        // FIFO Surface Queue
        SharedSurfaceQueueEntry*                m_SurfaceQueue;
        UINT                                    m_QueueHead;
        UINT                                    m_QueueSize;

        SharedSurfaceOpenedMapping*             m_ConsumerSurfaces;
        SharedSurfaceObject**                   m_CreatedSurfaces;
       
        UINT                                    m_iEnqueuedHead; 
        UINT                                    m_nEnqueuedSurfaces;

        SURFACE_QUEUE_DESC                      m_Desc;
        
        // Lock around all of the public queue functions.  This should have very little contention
        // and is used to synchronize rare queue state changes (i.e. the consumer device changes).
        SRWLOCK                                 m_lock;

        // Lock for access to the underlying queue
        CRITICAL_SECTION                        m_QueueLock;
};

