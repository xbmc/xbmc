// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include <new>
#include "SurfaceQueue.inl"

//
// Notes about the synchronization:  It's important for this library
// to be reasonably parallel; it should be possible to have simultaneously
// enqueues/dequeues.  It is undesirable for a thread to grab the queue
// lock while it calls into a blocking or very time consuming DirectX API.
//
// The single threaded flag will disable the synchronization constructs.
//
// There are 4 synchronization primitives used in this library.  
//		1) Critical Section in CSurfaceProducer/CSurfaceConsumer that protects
//         simultaneous access to their apis (Enqueue&Flush/Dequeue).  The public
//         functions from those objects are not designed to be multithreaded.  It
//         is not designed to support, for example, simultaneous Enqueues to the same
//         queue.  This lock guarantees that the Queue can not have simultaneous Enqueues
//         and Flushes.  There are a few CSurfaceQueue member variables that are shared
//         ONLY between Enqueue and Flush and do not need to be protected in CSurfaceQueue.
//      2) A Semaphore to control waiting when the Queue is empty.  The semaphore is
//         released on Enqueue/Flush and is waited on in Dequeue.  
//      3) A SlimReaderWriter lock protecting the CSurfaceQueue object.  All of the
//         high frequency calls grab shared locks (Enqueue/Flush/Dequeue) to allow
//         parallel access to the queue.  The low frequency state changes
//         (i.e. OpenProducer) will grab an exclusive lock.
//      4) A critical section protecting the underlying circular queue.  Both Enqueue
//         and dequeue will contend for this lock but the duration the lock is held
//         is kept to a minimum.
//

//-----------------------------------------------------------------------------
// Helper Functions
//-----------------------------------------------------------------------------
HRESULT CreateDeviceWrapper(IUnknown* pUnknown, ISurfaceQueueDevice** ppDevice)
{
    IDirect3DDevice9Ex* pD3D9Device;
    ID3D10Device*       pD3D10Device;
    ID3D11Device*       pD3D11Device;

    HRESULT hr = S_OK;
    *ppDevice  = NULL;

    if (SUCCEEDED(pUnknown->QueryInterface(__uuidof(IDirect3DDevice9Ex), (void**)&pD3D9Device)))
    {
        pD3D9Device->Release();
        *ppDevice = new QUEUE_NOTHROW_SPECIFIER CSurfaceQueueDeviceD3D9(pD3D9Device);
    }
    else if (SUCCEEDED(pUnknown->QueryInterface(__uuidof(ID3D10Device), (void**)&pD3D10Device)))
    {
        pD3D10Device->Release();
        *ppDevice = new QUEUE_NOTHROW_SPECIFIER CSurfaceQueueDeviceD3D10(pD3D10Device);
    }
    else if (SUCCEEDED(pUnknown->QueryInterface(__uuidof(ID3D11Device), (void**)&pD3D11Device)))
    {
        pD3D11Device->Release();
        *ppDevice = new QUEUE_NOTHROW_SPECIFIER CSurfaceQueueDeviceD3D11(pD3D11Device);
    }
    else
    {
        hr = E_INVALIDARG;    
    }

    if (SUCCEEDED(hr) && *ppDevice == NULL)
    {
        hr = E_OUTOFMEMORY;
    }

    return hr; 
};

//-----------------------------------------------------------------------------
// SharedSurfaceObject Implementation
//-----------------------------------------------------------------------------
SharedSurfaceObject::SharedSurfaceObject(UINT Width, UINT Height, DXGI_FORMAT Format)
{
    hSharedHandle   = NULL;
    state           = SHARED_SURFACE_STATE_UNINITIALIZED;
    queue           = NULL;

    width           = Width;
    height          = Height;
    format          = Format;

    pSurface        = NULL;
}

SharedSurfaceObject::~SharedSurfaceObject()
{
    // Release the reference to the created surface
    if (pSurface)
    {
        pSurface->Release();
    }
}

//-----------------------------------------------------------------------------
// CreateSurfaceQueue
//-----------------------------------------------------------------------------
HRESULT WINAPI CreateSurfaceQueue(
                    SURFACE_QUEUE_DESC*  pDesc,
                    IUnknown*            pDevice,
                    ISurfaceQueue**      ppQueue)
{
    HRESULT hr  = E_FAIL;
    
    if (ppQueue == NULL)
    {
        return E_INVALIDARG;
    }
    
    *ppQueue    = NULL;

    if (pDesc == NULL)
    {
        return E_INVALIDARG;
    }

    if (pDevice == NULL)
    {
        return E_INVALIDARG;
    }

    if (pDesc->NumSurfaces == 0)
    {
        return E_INVALIDARG;
    }
   
    if (pDesc->Width == 0 || pDesc->Height == 0)
    {
        return E_INVALIDARG;
    }

    if (pDesc->Flags != 0 && pDesc->Flags != SURFACE_QUEUE_FLAG_SINGLE_THREADED)
    {
        return E_INVALIDARG;
    }

    CSurfaceQueue* pSurfaceQueue = new QUEUE_NOTHROW_SPECIFIER CSurfaceQueue();
    if (!pSurfaceQueue)
    {
        hr = E_OUTOFMEMORY;
        goto end;
    }
    
    hr = pSurfaceQueue->Initialize(pDesc, pDevice, pSurfaceQueue);
    if (FAILED(hr))
    {
        goto end;
    }

    hr = pSurfaceQueue->QueryInterface(__uuidof(ISurfaceQueue), (void**)ppQueue);

end:
    if (FAILED(hr))
    {
        if (pSurfaceQueue)
        {
            delete pSurfaceQueue;
        }
        *ppQueue = NULL;
    }

    return hr;
}

//-----------------------------------------------------------------------------
// CSurfaceConsumer implementation
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
CSurfaceConsumer:: CSurfaceConsumer(BOOL IsMultithreaded) :
    m_RefCount(0),
    m_IsMultithreaded(IsMultithreaded),
    m_pQueue(NULL),
    m_pDevice(NULL)
{
    if (m_IsMultithreaded)
    {
        InitializeCriticalSection(&m_lock);
    }
}

//-----------------------------------------------------------------------------
CSurfaceConsumer::~CSurfaceConsumer()
{
    if (m_pQueue)
    {
        m_pQueue->RemoveConsumer();
        m_pQueue->Release();
    }
    if (m_pDevice)
    {
        delete m_pDevice;
    }
    if (m_IsMultithreaded)
    {
        DeleteCriticalSection(&m_lock);
    }
}

//-----------------------------------------------------------------------------
HRESULT CSurfaceConsumer::Initialize(IUnknown* pDevice)
{
    ASSERT(pDevice);
    ASSERT(m_pDevice == NULL);
   
    HRESULT hr;
    hr = CreateDeviceWrapper(pDevice, &m_pDevice);

    if (FAILED(hr))
    {
        if (m_pDevice)
        {
            delete m_pDevice;
            m_pDevice = NULL;
        }
    }

    return hr;
}

//-----------------------------------------------------------------------------
void CSurfaceConsumer::SetQueue(CSurfaceQueue* queue)
{
    ASSERT(!m_pQueue && queue);
    m_pQueue = queue;
    m_pQueue->AddRef();
}

//-----------------------------------------------------------------------------
HRESULT CSurfaceConsumer::Dequeue(
                        REFIID id,
                        void** ppSurface,
                        void*  pBuffer,
                        UINT*  BufferSize,
                        DWORD  dwTimeout)
{
    ASSERT(m_pQueue);

    HRESULT hr = S_OK;

    if (m_IsMultithreaded)
    {
        EnterCriticalSection(&m_lock);
    }

    // Validate that REFIID is correct for a surface from this device
    if (!m_pDevice->ValidateREFIID(id))
    {
        hr = E_INVALIDARG;
        goto end;
    }
    if (ppSurface == NULL)
    {
        hr = E_INVALIDARG;
        goto end;
    }

    *ppSurface = NULL;
    
    // Forward to queue
    hr = m_pQueue->Dequeue(ppSurface, pBuffer, BufferSize, dwTimeout);

end:
    if (m_IsMultithreaded)
    {
        LeaveCriticalSection(&m_lock);
    }
    return hr;
}


//-----------------------------------------------------------------------------
// CSurfaceProducer implementation
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
CSurfaceProducer:: CSurfaceProducer(BOOL IsMultithreaded) :
    m_RefCount(0),
    m_IsMultithreaded(IsMultithreaded),
    m_pQueue(NULL),
    m_pDevice(NULL),
    m_nStagingResources(0),
    m_pStagingResources(NULL),
    m_uiStagingResourceHeight(0),
    m_uiStagingResourceWidth(0),
    m_iCurrentResource(0)
{
    if (m_IsMultithreaded)
    {
        InitializeCriticalSection(&m_lock);
    }
}

//-----------------------------------------------------------------------------
CSurfaceProducer::~CSurfaceProducer()
{

    if (m_pQueue)
    {
        m_pQueue->RemoveProducer();
        m_pQueue->Release();
    }

    if (m_pStagingResources)
    {
        for (UINT i = 0; i < m_nStagingResources; i++)
        {
            if (m_pStagingResources[i])
            {
                m_pStagingResources[i]->Release();
            }
        }
        delete[] m_pStagingResources;
    }
    
    if (m_pDevice)
    {
        delete m_pDevice;
    }
    
    if (m_IsMultithreaded)
    { 
        DeleteCriticalSection(&m_lock);
    }
}

//-----------------------------------------------------------------------------
void CSurfaceProducer::SetQueue(CSurfaceQueue* queue)
{
    ASSERT(!m_pQueue && queue);
    m_pQueue = queue;
    m_pQueue->AddRef();
}

//-----------------------------------------------------------------------------
HRESULT CSurfaceProducer::Initialize(IUnknown* pDevice, UINT uNumSurfaces, SURFACE_QUEUE_DESC* queueDesc)
{
    ASSERT(pDevice);
    ASSERT(!m_pStagingResources && m_nStagingResources == 0)
   
    HRESULT hr = S_OK;
    hr = CreateDeviceWrapper(pDevice, &m_pDevice);
    if (FAILED(hr))
    {
        goto end;
    }

    m_pStagingResources = new QUEUE_NOTHROW_SPECIFIER IUnknown*[uNumSurfaces];
    if (!m_pStagingResources)
    {
        hr = E_OUTOFMEMORY;
        goto end;
    }
    ZeroMemory(m_pStagingResources, sizeof(IUnknown*) * uNumSurfaces);
    m_nStagingResources = uNumSurfaces;
   
    // Determine the size of the staging resource in case the queue surface is less than SHARED_SURFACE_COPY_SIZE
    m_uiStagingResourceWidth    = min(queueDesc->Width, SHARED_SURFACE_COPY_SIZE);
    m_uiStagingResourceHeight   = min(queueDesc->Height, SHARED_SURFACE_COPY_SIZE);

    // Create the staging resources
    for (UINT i = 0; i < m_nStagingResources; i++)
    {
        if (FAILED(hr = m_pDevice->CreateCopyResource(queueDesc->Format, m_uiStagingResourceWidth,
                                                      m_uiStagingResourceHeight, &(m_pStagingResources[i]))))
        {
            goto end;
        }
    }

end:
    if (FAILED(hr))
    {
        if (m_pStagingResources)
        {
            for (UINT i = 0; i < m_nStagingResources; i++)
            {
                if (m_pStagingResources[i])
                {
                    m_pStagingResources[i]->Release();
                }
            }
            delete[] m_pStagingResources;
            m_pStagingResources = NULL;
            m_nStagingResources = 0;
        }

        if (m_pDevice)
        {
            delete m_pDevice;
            m_pDevice = NULL;
        }
    }

    return hr;
}

//-----------------------------------------------------------------------------
HRESULT CSurfaceProducer::Enqueue(
                        IUnknown*   pSurface,
                        void*       pBuffer,
                        UINT        BufferSize,
                        DWORD       Flags )
{
    //
    // This function essentially does simple error checking and then
    // forwards the call to the queue object.  The SurfaceProducer
    // maintains a circular buffer of staging resources to use and will
    // pass the next availible one to the queue.
    //

    ASSERT(m_pQueue);

    if (m_IsMultithreaded)
    {
        EnterCriticalSection(&m_lock);
    }

    HRESULT hr;

    if (m_pDevice == NULL)
    {
        hr = E_INVALIDARG;
        goto end;
    }
    if (!pSurface)
    {
        hr = E_INVALIDARG;
        goto end;
    }
    if (Flags && Flags != SURFACE_QUEUE_FLAG_DO_NOT_WAIT)
    {
        hr = E_INVALIDARG;
        goto end;
    }

    // Forward call to queue
    hr = m_pQueue->Enqueue(
                            pSurface, 
                            pBuffer, 
                            BufferSize, 
                            Flags, 
                            m_pStagingResources[m_iCurrentResource],
                            m_uiStagingResourceWidth,
                            m_uiStagingResourceHeight
                          );
    
    if (hr == DXGI_ERROR_WAS_STILL_DRAWING)
    {
        //
        // Increment the staging resource only if the current one is still
        // being used.  This only happens if the function returns with 
        // DXGI_ERROR_WAS_STILL_DRAWING indicating that a future flush 
        // will still need the resource
        //
        // We do not need to worry about wrapping around and reusing staging
        // surfaces that are currently in use.  The design of the queue makes
        // it invalid to enqueue when the queue is already full.  If the user
        // does that, the queue will fail the call with E_INVALIDARG.
        m_iCurrentResource = (m_iCurrentResource + 1) % m_nStagingResources;
    }

end:
    if (m_IsMultithreaded)
    {
        LeaveCriticalSection(&m_lock);
    }
    return hr;
}

//-----------------------------------------------------------------------------
HRESULT CSurfaceProducer::Flush(
                        DWORD       Flags,
                        UINT*       NumSurfaces )
{
    ASSERT(m_pQueue);

    if (m_IsMultithreaded)
    {
        EnterCriticalSection(&m_lock);
    }

    HRESULT hr;

    if (m_pDevice == NULL)
    {
        hr = E_INVALIDARG;
        goto end;
    }
    if (Flags && Flags != SURFACE_QUEUE_FLAG_DO_NOT_WAIT)
    {
        hr = E_INVALIDARG;
        goto end;
    }
    
    // Forward call to queue
    hr = m_pQueue->Flush(Flags, NumSurfaces);

end:
    if (m_IsMultithreaded)
    {
        LeaveCriticalSection(&m_lock);
    }
    return hr;
}



//-----------------------------------------------------------------------------
// CSurfaceQueue implementation
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
CSurfaceQueue:: CSurfaceQueue()
    :
        m_RefCount(0),
        m_IsMultithreaded(TRUE),
        m_hSemaphore(NULL),
        m_pRootQueue(NULL),
        m_NumQueuesInNetwork(0),
        m_pConsumer(NULL),
        m_pProducer(NULL),
        m_pCreator(NULL),
        m_SurfaceQueue(NULL),
        m_QueueHead(0),
        m_QueueSize(0),
        m_ConsumerSurfaces(NULL),
        m_CreatedSurfaces(NULL),
        m_iEnqueuedHead(0),
        m_nEnqueuedSurfaces(0)
{
}

//-----------------------------------------------------------------------------
CSurfaceQueue::~CSurfaceQueue()
{
    Destroy();
}

//-----------------------------------------------------------------------------
void CSurfaceQueue::Destroy()
{
    RemoveQueueFromNetwork();
    
    // The ref counting should guarantee that the root queue object
    // is the last to be deleted
    if (m_pRootQueue != this)
    {
        m_pRootQueue->Release();
    }
    else
    {
        ASSERT(m_NumQueuesInNetwork == 0);
    }
    
    // The root queue will destroy the creating device
    if (m_pCreator)
    {
        delete m_pCreator;
        m_pCreator = NULL;
    }

    // Release all opened surfaces
    if (m_ConsumerSurfaces)
    {
        for (UINT i = 0; i < m_Desc.NumSurfaces; i++)
        {
            if (m_ConsumerSurfaces[i].pSurface)
            {
                m_ConsumerSurfaces[i].pSurface->Release();
            }
        }
        delete[] m_ConsumerSurfaces;
        m_ConsumerSurfaces = NULL;
    }

    // Clean up the allocated meta data buffers
    if (m_SurfaceQueue)
    {
        for (UINT i = 0; i < m_Desc.NumSurfaces; i++)
        {
            if (m_SurfaceQueue[i].pMetaData)
            {
                delete[] m_SurfaceQueue[i].pMetaData;
            }
        }
        delete[] m_SurfaceQueue;
        m_SurfaceQueue = NULL;
    }

    // The root queue object created the surfaces.  All other queue
    // objects only have a reference.
    if (m_CreatedSurfaces)
    {
        for (UINT i = 0; i < m_Desc.NumSurfaces; i++)
        {
            if (m_pRootQueue == this && m_CreatedSurfaces[i])
            {
                delete m_CreatedSurfaces[i];
            }
            m_CreatedSurfaces[i] = NULL;
        }
        delete[] m_CreatedSurfaces;
        m_CreatedSurfaces = NULL;
    }

    m_pConsumer = NULL;
    m_pProducer = NULL;

    if (m_IsMultithreaded)
    {
        if (m_hSemaphore)
        {
            CloseHandle(m_hSemaphore);
            m_hSemaphore = NULL;
        }
        DeleteCriticalSection(&m_QueueLock);
    }
    else
    {
        m_nFlushedSurfaces = 0;
    }
}

//-----------------------------------------------------------------------------
ISurfaceQueueDevice* CSurfaceQueue::GetCreatorDevice()
{
    return m_pRootQueue->m_pCreator;
}

//-----------------------------------------------------------------------------
UINT CSurfaceQueue::GetNumQueuesInNetwork()
{
    return m_pRootQueue->m_NumQueuesInNetwork;
}


//-----------------------------------------------------------------------------
UINT CSurfaceQueue::AddQueueToNetwork()
{
    if (m_pRootQueue == this)
    {
        return InterlockedIncrement(&m_NumQueuesInNetwork);
    }
    else
    {
        return m_pRootQueue->AddQueueToNetwork();
    }
}

//-----------------------------------------------------------------------------
UINT CSurfaceQueue::RemoveQueueFromNetwork()
{
    ASSERT(GetNumQueuesInNetwork() > 0);
    if (m_pRootQueue == this)
    {
        return InterlockedDecrement(&m_NumQueuesInNetwork);
    }
    else
    {
        return m_pRootQueue->RemoveQueueFromNetwork();
    }
}

//-----------------------------------------------------------------------------
SharedSurfaceObject* CSurfaceQueue::GetSurfaceObjectFromHandle(HANDLE handle)
{
    // 
    // This does a linear search through the created surfaces for the specific 
    // handle.  When the user enqueues, we get the shared handle from surface
    // and then use the handle to get to the SharedSurfaceObject.  This essentially
    // converts from a "generic d3d surface" to a "surface queue surface".
    //
    // This search is linear with the number of surfaces created.  We expect that
    // number to be small.
    //
    ASSERT(handle);
    ASSERT(m_CreatedSurfaces);
    for (UINT i = 0; i < m_Desc.NumSurfaces; i++)
    {
        if (m_CreatedSurfaces[i]->hSharedHandle == handle)
        {
            return m_CreatedSurfaces[i];
        }
    }
    // The user tried to enqueue an shared surface that was not part of the queue.
    return NULL;
}

//-----------------------------------------------------------------------------
IUnknown* CSurfaceQueue::GetOpenedSurface(const SharedSurfaceObject* pObject) const
{
    // 
    // On OpenConsumer, all of the shared surfaces will be opened by the consuming
    // device and cached.  On dequeue, we simply look in the cache and return the
    // appropriate surface, getting a significant perf bonus over opening/closing
    // the surface on every dequeue/enqueue.
    //
    // This method is also linear with respect to the number of surfaces and a more
    // scalable data structure can be used if the number of surfaces is used.
    //
    ASSERT(pObject);
    ASSERT(m_ConsumerSurfaces);
    for (UINT i = 0; i < m_Desc.NumSurfaces; i++)
    {
        if (m_ConsumerSurfaces[i].pObject == pObject)
        {
            return m_ConsumerSurfaces[i].pSurface;
        }
    }
    return NULL;
}

//-----------------------------------------------------------------------------
HRESULT CSurfaceQueue::AllocateMetaDataBuffers()
{
    // This function allocates the meta data buffers during creation time.
    if (m_Desc.MetaDataSize != 0)
    {
        for (UINT i = 0; i < m_Desc.NumSurfaces; i++)
        {
            m_SurfaceQueue[i].pMetaData = new QUEUE_NOTHROW_SPECIFIER BYTE[m_Desc.MetaDataSize];
            if (m_SurfaceQueue[i].pMetaData == NULL)
            {
                return E_OUTOFMEMORY;
            }
        }
    }
    return S_OK;
}

//-----------------------------------------------------------------------------
HRESULT CSurfaceQueue::CreateSurfaces()
{
    //
    // This function is only called by the root queue to create the surfaces.  
    // The queue has the property that the root queue starts off full (all the
    // surfaces on it are ready for dequeue.  This function will use the creating
    // device to create the shared surfaces and initialize them for dequeue.
    //
    
    HRESULT hr = S_OK;
    ASSERT(m_pRootQueue == this);

    for (UINT i = 0; i < m_Desc.NumSurfaces; i++)
    {
        SharedSurfaceObject* pSurfaceObject = new QUEUE_NOTHROW_SPECIFIER SharedSurfaceObject(m_Desc.Width, m_Desc.Height, m_Desc.Format);
        if (!pSurfaceObject)
        {
            return E_OUTOFMEMORY;
        }

        m_CreatedSurfaces[i] = pSurfaceObject;

        if (FAILED(hr = m_pCreator->CreateSharedSurface(
                                            m_Desc.Width, 
                                            m_Desc.Height,
                                            m_Desc.Format,
                                            &(pSurfaceObject->pSurface),
                                            &(pSurfaceObject->hSharedHandle)
                                            )))
        {
            return hr;
        }

        // Important to note that created surfaces start in the flushed state.  This
        // lets the system start in a state that makes it ready to go.
        m_SurfaceQueue[i].surface = m_CreatedSurfaces[i];
        m_SurfaceQueue[i].surface->state  = SHARED_SURFACE_STATE_FLUSHED;
        m_SurfaceQueue[i].surface->queue  = this;
    }

    return S_OK;
}

//-----------------------------------------------------------------------------
void CSurfaceQueue::CopySurfaceReferences(CSurfaceQueue* pRootQueue)
{
    // This is called by cloned devices.  They simply take a reference
    // to the shared created surfaces.
    ASSERT(m_pRootQueue != this);
    for (UINT i = 0; i < m_Desc.NumSurfaces; i++)
    {
        m_CreatedSurfaces[i] = pRootQueue->m_CreatedSurfaces[i];
    }
}

//-----------------------------------------------------------------------------
HRESULT CSurfaceQueue::Initialize(SURFACE_QUEUE_DESC* pDesc, 
                                      IUnknown* pDevice, 
                                      CSurfaceQueue* pRootQueue)
{
    ASSERT(pDesc);
    ASSERT(pRootQueue);

    HRESULT hr = S_OK;

    m_Desc              = *pDesc;
    m_pRootQueue        = pRootQueue;
    m_IsMultithreaded   = !(m_Desc.Flags & SURFACE_QUEUE_FLAG_SINGLE_THREADED);

    AddQueueToNetwork();
   
    if (m_IsMultithreaded)
    { 
        InitializeCriticalSection(&m_QueueLock);
    }

    // Allocate Queue
    ASSERT(!m_SurfaceQueue);
    m_SurfaceQueue = new QUEUE_NOTHROW_SPECIFIER SharedSurfaceQueueEntry[pDesc->NumSurfaces];
    if (!m_SurfaceQueue)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }
    ZeroMemory(m_SurfaceQueue, sizeof(SharedSurfaceQueueEntry) * pDesc->NumSurfaces);

    // Allocate array to keep track of opened surfaces
    ASSERT(!m_ConsumerSurfaces);
    m_ConsumerSurfaces = new QUEUE_NOTHROW_SPECIFIER SharedSurfaceOpenedMapping[pDesc->NumSurfaces];
    if (!m_ConsumerSurfaces)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }
    ZeroMemory(m_ConsumerSurfaces, sizeof(SharedSurfaceOpenedMapping) * pDesc->NumSurfaces);

    // Allocate created surface tracking list
    ASSERT(!m_CreatedSurfaces);
    m_CreatedSurfaces = new QUEUE_NOTHROW_SPECIFIER SharedSurfaceObject*[pDesc->NumSurfaces];
    if (!m_CreatedSurfaces)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }
    ZeroMemory(m_CreatedSurfaces, sizeof(SharedSurfaceObject*) * pDesc->NumSurfaces);

    // If this is the root queue, create the surfaces
    if (m_pRootQueue == this)
    {
        ASSERT(pDevice);

        hr = CreateDeviceWrapper(pDevice, &m_pCreator);
        if (FAILED(hr))
        {
            ASSERT(m_pCreator == NULL);
            goto cleanup;
        }

        hr = CreateSurfaces();
        if (FAILED(hr))
        {
            goto cleanup;
        }
        m_QueueSize         = pDesc->NumSurfaces;
    }
    else
    {
        // Increment the reference count on the src queue
        m_pRootQueue->AddRef();
        CopySurfaceReferences(pRootQueue); 
        m_QueueSize         = 0;
    }

    if (m_Desc.MetaDataSize)
    {
       if (FAILED(hr = AllocateMetaDataBuffers()))
       {
           goto cleanup;
       }
    }
    
    ASSERT(m_pRootQueue);

    if (m_IsMultithreaded)
    {
        // Create Semaphore for queue synchronization
        m_hSemaphore = CreateSemaphore(NULL, 
                        m_pRootQueue == this ? pDesc->NumSurfaces : 0, 
                        pDesc->NumSurfaces, 
                        NULL);
        if (m_hSemaphore == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto cleanup;
        }

        
        // Initialize the slim reader/writer lock
        InitializeSRWLock(&m_lock);
    }
    else
    {
		m_nFlushedSurfaces = m_pRootQueue == this ? pDesc->NumSurfaces : 0;
    }

cleanup:
    // The object will get destroyed if initialize fails.  Cleanup
    // will happen then.
    return hr;
}

//-----------------------------------------------------------------------------
HRESULT CSurfaceQueue::OpenConsumer(
                    IUnknown*              pDevice,
                    ISurfaceConsumer**     ppConsumer)
{
    if (pDevice == NULL)
    {
        return E_INVALIDARG;
    }
    if (ppConsumer == NULL)
    {
        return E_INVALIDARG;
    }

    *ppConsumer = NULL;

    HRESULT hr = E_FAIL;
    if (m_IsMultithreaded)
    {
        AcquireSRWLockExclusive(&m_lock);
    }

	// 
	// If a consumer exists, we need to bail early. The normal error 
	// path will deallocate the current consumer.  Instead this will 
	// be a no-op for the queue and E_INVALIDARG will be returned.
	//
    if (m_pConsumer)
    {
        if (m_IsMultithreaded)
        {
            ReleaseSRWLockExclusive(&m_lock);
        }
        return E_INVALIDARG;
    }

    m_pConsumer = new QUEUE_NOTHROW_SPECIFIER CSurfaceConsumer(m_IsMultithreaded);
    if (m_pConsumer == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto end;
    }

    hr = m_pConsumer->Initialize(pDevice);
    if (FAILED(hr))
    {
        goto end;
    }

    //
    // For all the surfaces in the queue, we want to open it with the producing device.
    // This guarantees that surfaces are only open at creation time.
    //
    for (UINT i = 0; i < m_Desc.NumSurfaces; i++)
    {
        ASSERT(m_CreatedSurfaces[i]);
        ASSERT(m_ConsumerSurfaces);

        IUnknown*   pSurface = NULL;

        hr = m_pConsumer->GetDevice()->OpenSurface(
                                        m_CreatedSurfaces[i]->hSharedHandle, 
                                        (void**)&pSurface, 
                                        m_Desc.Width, 
                                        m_Desc.Height, 
                                        m_Desc.Format);
        if (FAILED(hr))
        {
            goto end;
        }

        ASSERT(pSurface);
    
        m_ConsumerSurfaces[i].pObject     = m_CreatedSurfaces[i];
        m_ConsumerSurfaces[i].pSurface    = pSurface;
    }

    hr = m_pConsumer->QueryInterface(__uuidof(ISurfaceConsumer), (void**) ppConsumer);
    if (FAILED(hr))
    {
        goto end;
    }

    m_pConsumer->SetQueue(this);

end:
    if (FAILED(hr))
    {
        *ppConsumer = NULL;
        
        if (m_pConsumer)
        {
            if (m_pConsumer->GetDevice())
            {
                for (UINT i = 0; i < m_Desc.NumSurfaces; i++)
                {
                    if (m_ConsumerSurfaces[i].pSurface)
                    {
                        m_ConsumerSurfaces[i].pSurface->Release();
                    }
                }
            }
            
            ZeroMemory(m_ConsumerSurfaces, sizeof(SharedSurfaceOpenedMapping) * m_Desc.NumSurfaces);
 
            delete m_pConsumer;
            m_pConsumer = NULL;
        }
    }

    if (m_IsMultithreaded)
    {
        ReleaseSRWLockExclusive(&m_lock);
    }
    return hr;
}

//-----------------------------------------------------------------------------
HRESULT CSurfaceQueue::OpenProducer(
                    IUnknown*                   pDevice,
                    ISurfaceProducer**     ppProducer)
{
    if (pDevice == NULL)
    {
        return E_INVALIDARG;
    }
    if (ppProducer == NULL)
    {
        return E_INVALIDARG;
    }

    *ppProducer = NULL;

    HRESULT hr = E_FAIL;

    if (m_IsMultithreaded)
    {
        AcquireSRWLockExclusive(&m_lock);
    }

    if (m_pProducer)
    {
        if (m_IsMultithreaded)
        {
            ReleaseSRWLockExclusive(&m_lock);
        }
        return E_INVALIDARG;
    }

    m_pProducer = new QUEUE_NOTHROW_SPECIFIER CSurfaceProducer(m_IsMultithreaded);
    if (m_pProducer == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto end;
    }

    hr = m_pProducer->Initialize(pDevice, m_Desc.NumSurfaces, &m_Desc);
    if (FAILED(hr))
    {
        goto end;
    }
    
    hr = m_pProducer->QueryInterface(__uuidof(ISurfaceProducer), (void**)ppProducer);
    if (FAILED (hr))
    {
        goto end;
    }

    m_pProducer->SetQueue(this);

end:
    if (FAILED(hr))
    {
        *ppProducer = NULL;
        if (m_pProducer)
        {
            delete m_pProducer;
            m_pProducer = NULL;
            *ppProducer = NULL;
        }
    }

    if (m_IsMultithreaded)
    {
        ReleaseSRWLockExclusive(&m_lock);
    }
    
    return hr;
}

//-----------------------------------------------------------------------------
void CSurfaceQueue::RemoveProducer()
{
    if (m_IsMultithreaded)
    {
        AcquireSRWLockExclusive(&m_lock);
    }
    
    ASSERT(m_pProducer);
    m_pProducer = NULL;

    if (m_IsMultithreaded)
    {
        ReleaseSRWLockExclusive(&m_lock);
    }
}

//-----------------------------------------------------------------------------
void CSurfaceQueue::RemoveConsumer()
{
    if (m_IsMultithreaded)
    {
        AcquireSRWLockExclusive(&m_lock);
    }

    ASSERT(m_pConsumer && m_pConsumer->GetDevice());
    for (UINT i = 0; i < m_Desc.NumSurfaces; i++)
    {
        if (m_ConsumerSurfaces[i].pSurface)
        {
            m_ConsumerSurfaces[i].pSurface->Release();
        }
    }
    ZeroMemory(m_ConsumerSurfaces, sizeof(SharedSurfaceOpenedMapping) * m_Desc.NumSurfaces);
    m_pConsumer = NULL; 
    
    if (m_IsMultithreaded)
    {
        ReleaseSRWLockExclusive(&m_lock);
    }
}

//-----------------------------------------------------------------------------
HRESULT CSurfaceQueue::Clone(
                    SURFACE_QUEUE_CLONE_DESC*   pDesc,
                    ISurfaceQueue**        ppQueue)
{
    // Have all the clones originate from the root queue.  This makes tracking
    // referenes easier.
    if (m_pRootQueue != this)
    {
        return m_pRootQueue->Clone(pDesc, ppQueue);
    }

    if (!pDesc)
    {
        return E_INVALIDARG;
    }
    if (!ppQueue)
    {
        return E_INVALIDARG;
    }
    if (pDesc->Flags != 0 && pDesc->Flags != SURFACE_QUEUE_FLAG_SINGLE_THREADED)
    {
        return E_INVALIDARG;
    }

    *ppQueue    = NULL;
    HRESULT hr  = E_FAIL;
   
    if (m_IsMultithreaded)
    { 
        AcquireSRWLockExclusive(&m_lock);
    }

    SURFACE_QUEUE_DESC createDesc = m_Desc;
    createDesc.MetaDataSize = pDesc->MetaDataSize;
    createDesc.Flags = pDesc->Flags;

    CSurfaceQueue* pQueue = new QUEUE_NOTHROW_SPECIFIER CSurfaceQueue();
    if (!pQueue)
    {
        hr = E_OUTOFMEMORY;
        goto end;
    }
    hr = pQueue->Initialize(&createDesc, NULL, this);
    if (FAILED(hr))
    {
        goto end;
    }

    hr = pQueue->QueryInterface(__uuidof(ISurfaceQueue), (void**)ppQueue);

end:
    if (FAILED(hr))
    {
        if (pQueue)
        {
            delete pQueue;
        }
        *ppQueue = NULL;
    }

    if (m_IsMultithreaded)
    {
        ReleaseSRWLockExclusive(&m_lock);
    }
    return hr;
}

//-----------------------------------------------------------------------------
HRESULT CSurfaceQueue::Enqueue(
                            IUnknown*   pSurface, 
                            void*       pBuffer, 
                            UINT        BufferSize, 
                            DWORD       Flags,
                            IUnknown*   pStagingResource,
                            UINT        width,
                            UINT        height
                        )
{
    ASSERT( pSurface );

    if (pBuffer && !BufferSize)
    {
        return E_INVALIDARG;
    }
    if (!pBuffer && BufferSize)
    {
        return E_INVALIDARG;
    }
    if (BufferSize > m_Desc.MetaDataSize)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = E_FAIL;

    if (m_IsMultithreaded)
    {
        AcquireSRWLockShared(&m_lock);
    }

    ASSERT( m_pProducer );
   
    SharedSurfaceQueueEntry QueueEntry;
    HANDLE                  hSharedHandle;
    SharedSurfaceObject*    pSurfaceObject;

    // Require both the producer and consumer to be initialized.
    // This avoids a potential race condition
    if (!m_pProducer || !m_pConsumer)
    {
        hr = E_INVALIDARG;
        goto end;
    }

    
    // Check that the queue is not full.  Enqueuing onto a full queue is
    // not a scenario that makes sense
    if (m_QueueSize == m_Desc.NumSurfaces)
    {
        hr = E_INVALIDARG;
        goto end;
    } 

    // Get the SharedSurfaceObject from the surface
    hr = m_pProducer->GetDevice()->GetSharedHandle(pSurface, &hSharedHandle);
    if (FAILED(hr))
    {
        goto end;
    }

    pSurfaceObject = GetSurfaceObjectFromHandle(hSharedHandle);

    // Validate that this surface is one that can be part of this queue
    if (pSurfaceObject == NULL)
    {
        hr = E_INVALIDARG;
        goto end;
    }

    if (pSurfaceObject->state != SHARED_SURFACE_STATE_DEQUEUED)
    {
        hr = E_INVALIDARG;
        goto end;
    }

    QueueEntry.surface          = pSurfaceObject;
    QueueEntry.pMetaData        = (BYTE*)pBuffer;
    QueueEntry.bMetaDataSize    = BufferSize;
    QueueEntry.pStagingResource = NULL;

    // Copy a small portion of the surface onto the staging surface
    hr = m_pProducer->GetDevice()->CopySurface(pStagingResource, pSurface, width, height);
    if (FAILED(hr))
    {
        goto end;
    }
    
    pSurfaceObject->state = SHARED_SURFACE_STATE_ENQUEUED;
    pSurfaceObject->queue = this;

    //
    // At this point we have succesfully issued the copy to the staging resource.
    // The surface will now must be added to the fifo queue either in the ENQUEUED
    // or FLUSHED state.
    //

    // 
    // Do not attempt to flush the surfaces if the DO_NOT_WAIT flag was used.
    // In these cases, simply add the surface to the FIFO queue as an ENQUEUED surface.
    //
    //
    // Note: m_nEnqueuedSurfaces and m_iEnqueuedHead are protected by the lock in the 
    // SurfaceProducer.  This value is not shared between the Consumer and Producer and 
    // therefore does not need any sychronization in the queue object.
    //
    if (Flags & SURFACE_QUEUE_FLAG_DO_NOT_WAIT)
    { 
        //
        // The surface should go into the ENQUEUED but not FLUSHED state.
        //

        //
        // Queue the entry into the fifo queue along with the staging resource for it
        //
        QueueEntry.pStagingResource = pStagingResource;
        Enqueue(QueueEntry);
        m_nEnqueuedSurfaces++;

        //
        // Since the surface did not flush, set the return to DXGI_ERROR_WAS_STILL_DRAWING
        // and return.
        //
        hr = DXGI_ERROR_WAS_STILL_DRAWING;
        goto end;
    }
    else if (m_nEnqueuedSurfaces)
    {
        //
        // Enqueued was called without the DO_NOT_WAIT flag but there are enqueued surfaces
        // currently not flushed.  First flush the existing surfaces and then perform the
        // current Enqueue.
        //
        hr = Flush(0, NULL);
        ASSERT(SUCCEEDED(hr));
    }

    //
    // Force rendering to complete by locking the staging resource.
    //
    if (FAILED(hr = m_pProducer->GetDevice()->LockSurface(pStagingResource, Flags)))
    {
        goto end;
    }
    if (FAILED(hr = m_pProducer->GetDevice()->UnlockSurface(pStagingResource)))
    {
        goto end;
    }
    ASSERT(QueueEntry.pStagingResource == NULL); 
    //
    // The call to lock the surface completed succesfully meaning the surface if flushed
    // and ready for dequeue.  Mark the surface as such and add it to the fifo queue.
    //
    pSurfaceObject->state = SHARED_SURFACE_STATE_FLUSHED;

    m_iEnqueuedHead = (m_iEnqueuedHead + 1) % m_Desc.NumSurfaces;
    Enqueue(QueueEntry);

    if (m_IsMultithreaded)
    {
        // Increment the semaphore
        ReleaseSemaphore(m_hSemaphore, 1, NULL);
    }
    else
    {
        m_nFlushedSurfaces++;
    }
    

end:
    if (m_IsMultithreaded)
    {
        ReleaseSRWLockShared(&m_lock);
    }
    return hr;
}

//-----------------------------------------------------------------------------
HRESULT CSurfaceQueue::Dequeue(
                            void**                  ppSurface,
                            void*                   pBuffer,
                            UINT*                   BufferSize,
                            DWORD                   dwTimeout  
                        )
{
    if (!pBuffer && BufferSize)
    {
        return E_INVALIDARG;
    }
    if (pBuffer)
    {
       if (!BufferSize || *BufferSize == 0)
       {
            return E_INVALIDARG;
       }
       if (*BufferSize > m_Desc.MetaDataSize)
       {
           return E_INVALIDARG;
       }
    }

    if (m_IsMultithreaded)
    {
        AcquireSRWLockShared(&m_lock);
    }

    SharedSurfaceQueueEntry QueueElement;
    IUnknown*               pSurface    = NULL;
    HRESULT                 hr          = E_FAIL;

    // Require both the producer and consumer to be initialized.
    // This avoids a potential race condition
    if (!m_pProducer || !m_pConsumer)
    {
        hr = E_INVALIDARG;
        goto end;
    }

    if (m_IsMultithreaded)
    {
        // Wait on the semaphore until the queue is not empty
        DWORD dwWait = WaitForSingleObject(m_hSemaphore, dwTimeout);
        switch (dwWait)
        {
            case WAIT_ABANDONED:
                hr = E_FAIL;
                break;
            case WAIT_OBJECT_0:
                hr = S_OK;
                break;
            case WAIT_TIMEOUT:
                hr = HRESULT_FROM_WIN32(WAIT_TIMEOUT);
                break;
            case WAIT_FAILED:
                hr = HRESULT_FROM_WIN32(GetLastError());
                break;
            default:
                hr = E_FAIL;
                break;
        }
    }
    else
    {
        // In the single threaded case, dequeuing on an empty 
        // will return immediately.  The error returned is not
        // *exactly* right but it parallels the multithreaded
        // case.
        if (m_nFlushedSurfaces == 0)
        {
            hr = HRESULT_FROM_WIN32(WAIT_TIMEOUT);
        }
        else
        {
            m_nFlushedSurfaces--;
            hr = S_OK;
        }
    }

    
    // Early return because of timeout or wait error
    if (FAILED(hr))
    {
        goto end;
    }


    //
    // At this point, there must be a surface in the queue ready to be
    // dequeued.  Get a reference to the first surface make sure
    // it is valid.  We don't want the situation where the surface is
    // removed but then fails.
    //

    // At this point there must be an surface in the queue ready to go
    // Dequeue it
    
    Front(QueueElement);

    ASSERT (QueueElement.surface->state == SHARED_SURFACE_STATE_FLUSHED);
    ASSERT (QueueElement.surface->queue == this);

    //
    // Update the state of the surface to dequeued
    //
    QueueElement.surface->state  = SHARED_SURFACE_STATE_DEQUEUED;
    QueueElement.surface->device = m_pConsumer->GetDevice();   
   
    // 
    // Get the surface for the consuming device from the surface object
    //
    pSurface = GetOpenedSurface(QueueElement.surface);

    ASSERT(pSurface);

    pSurface->AddRef();
    *ppSurface = pSurface;

    // 
    // There should be no more failures after here
    //
    
    if (pBuffer && QueueElement.bMetaDataSize)
    {
        memcpy(pBuffer, QueueElement.pMetaData, sizeof(BYTE) * QueueElement.bMetaDataSize);
    }

    //
    // Store the actual number of bytes copied as meta data.
    //
    if (BufferSize)
    {
        *BufferSize = QueueElement.bMetaDataSize;
    }

    //
    // Remove the element from the queue.  We do it at the very end in case there are
    // errors.
    //
    Dequeue(QueueElement);

end:
    if (m_IsMultithreaded)
    {
        ReleaseSRWLockShared(&m_lock);
    }

    return hr;
}

//-----------------------------------------------------------------------------
HRESULT CSurfaceQueue::Flush(
                            DWORD   Flags,
                            UINT*   pRemainingSurfaces
                        )
{
    if (m_IsMultithreaded)
    {
        AcquireSRWLockShared(&m_lock);
    }

    HRESULT hr = S_OK; 
    UINT    uiFlushedSurfaces = 0;
    UINT    index, i;

    // Store this locally for the loop counter.  The loop will change the
    // value of m_nEnqueuedSurfaces.
    UINT    uiEnqueuedSize = m_nEnqueuedSurfaces;

    // Require both the producer and consumer to be initialized.
    // This avoids a potential race condition
    if (!m_pProducer || !m_pConsumer)
    {
        hr = E_INVALIDARG;
        goto end;
    }

    // Iterate over all queue entries starting at the head.
    for (index = m_iEnqueuedHead, i = 0; i < uiEnqueuedSize; i++, index++)
    {
        index = index % m_Desc.NumSurfaces;
        
        SharedSurfaceQueueEntry& queueEntry = m_SurfaceQueue[index];
  
        ASSERT(queueEntry.surface->state == SHARED_SURFACE_STATE_ENQUEUED);
        ASSERT(queueEntry.surface->queue == this);
        ASSERT(queueEntry.pStagingResource);

        IUnknown* pStagingResource = queueEntry.pStagingResource;
        
        // 
        // Attempt to lock the staging surface to see if the rendering
        // is complete.
        //
        hr = m_pProducer->GetDevice()->LockSurface(pStagingResource, Flags);
        if (FAILED(hr))
        {
            //
            // As soon as the first surface is not flushed, skip the remaining
            //
            goto end;
        }

        hr = m_pProducer->GetDevice()->UnlockSurface(pStagingResource);
        ASSERT(SUCCEEDED(hr));

        // When the lock is complete, rendering is complete and the the surface is
        // ready for dequeue
        queueEntry.surface->state = SHARED_SURFACE_STATE_FLUSHED;
        queueEntry.pStagingResource = NULL;

        uiFlushedSurfaces++;
       
        // This is protected by the SurfaceProducer lock.
        m_nEnqueuedSurfaces--;
        m_iEnqueuedHead = (m_iEnqueuedHead + 1) % m_Desc.NumSurfaces;

        if (m_IsMultithreaded)
        {
            // Increment the semaphore count
            ReleaseSemaphore(m_hSemaphore, 1, NULL);
        }
        else
        {
            m_nFlushedSurfaces++;
        }
    }

end:

    if (pRemainingSurfaces)
    {
        *pRemainingSurfaces = m_nEnqueuedSurfaces;
    }
    
    if (m_IsMultithreaded)
    {
        ReleaseSRWLockShared(&m_lock);
    }

    return hr; 
}

//-----------------------------------------------------------------------------
void CSurfaceQueue::Front(SharedSurfaceQueueEntry& entry)
{
    entry = m_SurfaceQueue[m_QueueHead];
}

//-----------------------------------------------------------------------------
void CSurfaceQueue::Dequeue(SharedSurfaceQueueEntry& entry)
{
    // The semaphore protecting access to the queue guarantees that the queue
    // can not be empty.
    if (m_IsMultithreaded)
    {
        EnterCriticalSection(&m_QueueLock);
    }

    entry = m_SurfaceQueue[m_QueueHead];
    m_QueueHead = (m_QueueHead + 1) % m_Desc.NumSurfaces;
    m_QueueSize--;

    if (m_IsMultithreaded)
    {
        LeaveCriticalSection(&m_QueueLock);
    }
}

//-----------------------------------------------------------------------------
void CSurfaceQueue::Enqueue(SharedSurfaceQueueEntry& entry)
{
    //
    // The validation in the queue should guarantee that the queue is not full
    //
    

    if (m_IsMultithreaded)
    {
        EnterCriticalSection(&m_QueueLock);
    }

    UINT end = (m_QueueHead + m_QueueSize) % m_Desc.NumSurfaces;
    m_QueueSize++;
    
    if (m_IsMultithreaded)
    {
        LeaveCriticalSection(&m_QueueLock);
    }

    m_SurfaceQueue[end].surface          = entry.surface;
    m_SurfaceQueue[end].bMetaDataSize    = entry.bMetaDataSize;
    m_SurfaceQueue[end].pStagingResource = entry.pStagingResource;
    if (entry.bMetaDataSize)
    {
        memcpy(m_SurfaceQueue[end].pMetaData, entry.pMetaData, sizeof(BYTE) * entry.bMetaDataSize);
    }

}

