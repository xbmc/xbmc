//------------------------------------------------------------------------------
// File: AsyncIo.cpp
//
// Desc: DirectShow sample code - base library with I/O functionality.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


#include <streams.h>
#include "asyncio.h"

// --- CAsyncRequest ---


// implementation of CAsyncRequest representing a single
// outstanding request. All the i/o for this object is done
// in the Complete method.


// init the params for this request.
// Read is not issued until the complete call
HRESULT
CAsyncRequest::Request(
    CAsyncIo *pIo,
    CAsyncStream *pStream,
    LONGLONG llPos,
    LONG lLength,
    BOOL bAligned,
    BYTE* pBuffer,
    LPVOID pContext,    // filter's context
    DWORD_PTR dwUser)   // downstream filter's context
{
    m_pIo = pIo;
    m_pStream = pStream;
    m_llPos = llPos;
    m_lLength = lLength;
    m_bAligned = bAligned;
    m_pBuffer = pBuffer;
    m_pContext = pContext;
    m_dwUser = dwUser;
    m_hr = VFW_E_TIMEOUT;   // not done yet

    return S_OK;
}


// issue the i/o if not overlapped, and block until i/o complete.
// returns error code of file i/o
//
//
HRESULT
CAsyncRequest::Complete()
{
    m_pStream->Lock();

    m_hr = m_pStream->SetPointer(m_llPos);
    if(S_OK == m_hr)
    {
        DWORD dwActual;

        m_hr = m_pStream->Read(m_pBuffer, m_lLength, m_bAligned, &dwActual);
        if(m_hr == OLE_S_FIRST)
        {
            if(m_pContext)
            {
                IMediaSample *pSample = reinterpret_cast<IMediaSample *>(m_pContext);
                pSample->SetDiscontinuity(TRUE);
                m_hr = S_OK;
            }
        }

        if(FAILED(m_hr))
        {
        }
        else if(dwActual != (DWORD)m_lLength)
        {
            // tell caller size changed - probably because of EOF
            m_lLength = (LONG) dwActual;
            m_hr = S_FALSE;
        }
        else
        {
            m_hr = S_OK;
        }
    }

    m_pStream->Unlock();
    return m_hr;
}


// --- CAsyncIo ---

// note - all events created manual reset

CAsyncIo::CAsyncIo(CAsyncStream *pStream)
         : m_hThread(NULL),
           m_evWork(TRUE),
           m_evDone(TRUE),
           m_evStop(TRUE),
           m_listWork(NAME("Work list")),
           m_listDone(NAME("Done list")),
           m_bFlushing(FALSE),
           m_cItemsOut(0),
           m_bWaiting(FALSE),
           m_pStream(pStream)
{

}


CAsyncIo::~CAsyncIo()
{
    // move everything to the done list
    BeginFlush();

    // shutdown worker thread
    CloseThread();

    // empty the done list
    POSITION pos = m_listDone.GetHeadPosition();
    while(pos)
    {
        CAsyncRequest* pRequest = m_listDone.GetNext(pos);
        delete pRequest;
    }

    m_listDone.RemoveAll();
}


// ready for async activity - call this before calling Request.
//
// start the worker thread if we need to
//
// !!! use overlapped i/o if possible
HRESULT
CAsyncIo::AsyncActive(void)
{
    return StartThread();
}

// call this when no more async activity will happen before
// the next AsyncActive call
//
// stop the worker thread if active
HRESULT
CAsyncIo::AsyncInactive(void)
{
    return CloseThread();
}


// add a request to the queue.
HRESULT
CAsyncIo::Request(
                LONGLONG llPos,
                LONG lLength,
                BOOL bAligned,
                BYTE * pBuffer,
                LPVOID pContext,
                DWORD_PTR dwUser)
{
    if(bAligned)
    {
        if(!IsAligned(llPos) ||
            !IsAligned(lLength) ||
            !IsAligned((LONG_PTR) pBuffer))
        {
            return VFW_E_BADALIGN;
        }
    }

    CAsyncRequest* pRequest = new CAsyncRequest;
    if (!pRequest)
        return E_OUTOFMEMORY;

    HRESULT hr = pRequest->Request(this,
                                   m_pStream,
                                   llPos,
                                   lLength,
                                   bAligned,
                                   pBuffer,
                                   pContext,
                                   dwUser);
    if(SUCCEEDED(hr))
    {
        // might fail if flushing
        hr = PutWorkItem(pRequest);
    }

    if(FAILED(hr))
    {
        delete pRequest;
    }

    return hr;
}


// wait for the next request to complete
HRESULT
CAsyncIo::WaitForNext(
    DWORD dwTimeout,
    LPVOID     * ppContext,
    DWORD_PTR  * pdwUser,
    LONG       * pcbActual)
{
    CheckPointer(ppContext,E_POINTER);
    CheckPointer(pdwUser,E_POINTER);
    CheckPointer(pcbActual,E_POINTER);

    // some errors find a sample, others don't. Ensure that
    // *ppContext is NULL if no sample found
    *ppContext = NULL;

    // wait until the event is set, but since we are not
    // holding the critsec when waiting, we may need to re-wait
    for(;;)
    {
        if(!m_evDone.Wait(dwTimeout))
        {
            // timeout occurred
            return VFW_E_TIMEOUT;
        }

        // get next event from list
        CAsyncRequest* pRequest = GetDoneItem();
        if(pRequest)
        {
            // found a completed request

            // check if ok
            HRESULT hr = pRequest->GetHResult();
            if(hr == S_FALSE)
            {
                // this means the actual length was less than
                // requested - may be ok if he aligned the end of file
                if((pRequest->GetActualLength() +
                    pRequest->GetStart()) == Size())
                {
                    hr = S_OK;
                }
                else
                {
                    // it was an actual read error
                    hr = E_FAIL;
                }
            }

            // return actual bytes read
            *pcbActual = pRequest->GetActualLength();

            // return his context
            *ppContext = pRequest->GetContext();
            *pdwUser = pRequest->GetUser();

            delete pRequest;
            return hr;
        }
        else
        {
            //  Hold the critical section while checking the list state
            CAutoLock lck(&m_csLists);
            if(m_bFlushing && !m_bWaiting)
            {
                // can't block as we are between BeginFlush and EndFlush

                // but note that if m_bWaiting is set, then there are some
                // items not yet complete that we should block for.

                return VFW_E_WRONG_STATE;
            }
        }

        // done item was grabbed between completion and
        // us locking m_csLists.
    }
}


// perform a synchronous read request on this thread.
// Need to hold m_csFile while doing this (done in request object)
HRESULT
CAsyncIo::SyncReadAligned(
                        LONGLONG llPos,
                        LONG lLength,
                        BYTE * pBuffer,
                        LONG * pcbActual,
                        PVOID pvContext)
{
    CheckPointer(pcbActual,E_POINTER);

    if(!IsAligned(llPos) ||
        !IsAligned(lLength) ||
        !IsAligned((LONG_PTR) pBuffer))
    {
        return VFW_E_BADALIGN;
    }

    CAsyncRequest request;

    HRESULT hr = request.Request(this,
                                m_pStream,
                                llPos,
                                lLength,
                                TRUE,
                                pBuffer,
                                pvContext,
                                0);
    if(FAILED(hr))
        return hr;

    hr = request.Complete();

    // return actual data length
    *pcbActual = request.GetActualLength();
    return hr;
}


HRESULT
CAsyncIo::Length(LONGLONG *pllTotal, LONGLONG *pllAvailable)
{
    CheckPointer(pllTotal,E_POINTER);

    *pllTotal = m_pStream->Size(pllAvailable);
    return S_OK;
}


// cancel all items on the worklist onto the done list
// and refuse further requests or further WaitForNext calls
// until the end flush
//
// WaitForNext must return with NULL only if there are no successful requests.
// So Flush does the following:
// 1. set m_bFlushing ensures no more requests succeed
// 2. move all items from work list to the done list.
// 3. If there are any outstanding requests, then we need to release the
//    critsec to allow them to complete. The m_bWaiting as well as ensuring
//    that we are signalled when they are all done is also used to indicate
//    to WaitForNext that it should continue to block.
// 4. Once all outstanding requests are complete, we force m_evDone set and
//    m_bFlushing set and m_bWaiting false. This ensures that WaitForNext will
//    not block when the done list is empty.
HRESULT
CAsyncIo::BeginFlush()
{
    // hold the lock while emptying the work list
    {
        CAutoLock lock(&m_csLists);

        // prevent further requests being queued.
        // Also WaitForNext will refuse to block if this is set
        // unless m_bWaiting is also set which it will be when we release
        // the critsec if there are any outstanding).
        m_bFlushing = TRUE;

        CAsyncRequest * preq;
        while((preq = GetWorkItem()) != 0)
        {
            preq->Cancel();
            PutDoneItem(preq);
        }

        // now wait for any outstanding requests to complete
        if(m_cItemsOut > 0)
        {
            // can be only one person waiting
            ASSERT(!m_bWaiting);

            // this tells the completion routine that we need to be
            // signalled via m_evAllDone when all outstanding items are
            // done. It also tells WaitForNext to continue blocking.
            m_bWaiting = TRUE;
        }
        else
        {
            // all done

            // force m_evDone set so that even if list is empty,
            // WaitForNext will not block
            // don't do this until we are sure that all
            // requests are on the done list.
            m_evDone.Set();
            return S_OK;
        }
    }

    ASSERT(m_bWaiting);

    // wait without holding critsec
    for(;;)
    {
        m_evAllDone.Wait();
        {
            // hold critsec to check
            CAutoLock lock(&m_csLists);

            if(m_cItemsOut == 0)
            {
                // now we are sure that all outstanding requests are on
                // the done list and no more will be accepted
                m_bWaiting = FALSE;

                // force m_evDone set so that even if list is empty,
                // WaitForNext will not block
                // don't do this until we are sure that all
                // requests are on the done list.
                m_evDone.Set();

                return S_OK;
            }
        }
    }
}


// end a flushing state
HRESULT
CAsyncIo::EndFlush()
{
    CAutoLock lock(&m_csLists);

    m_bFlushing = FALSE;

    ASSERT(!m_bWaiting);

    // m_evDone might have been set by BeginFlush - ensure it is
    // set IFF m_listDone is non-empty
    if(m_listDone.GetCount() > 0)
    {
        m_evDone.Set();
    }
    else
    {
        m_evDone.Reset();
    }

    return S_OK;
}


// start the thread
HRESULT
CAsyncIo::StartThread(void)
{
    if(m_hThread)
    {
        return S_OK;
    }

    // clear the stop event before starting
    m_evStop.Reset();

    DWORD dwThreadID;
    m_hThread = CreateThread(NULL,
                            0,
                            InitialThreadProc,
                            this,
                            0,
                            &dwThreadID);
    if(!m_hThread)
    {
        DWORD dwErr = GetLastError();
        return HRESULT_FROM_WIN32(dwErr);
    }

    return S_OK;
}


// stop the thread and close the handle
HRESULT
CAsyncIo::CloseThread(void)
{
    // signal the thread-exit object
    m_evStop.Set();

    if(m_hThread)
    {
        WaitForSingleObject(m_hThread, INFINITE);
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }

    return S_OK;
}


// manage the list of requests. hold m_csLists and ensure
// that the (manual reset) event hevList is set when things on
// the list but reset when the list is empty.
// returns null if list empty
CAsyncRequest*
CAsyncIo::GetWorkItem()
{
    CAutoLock lck(&m_csLists);
    CAsyncRequest * preq  = m_listWork.RemoveHead();

    // force event set correctly
    if(m_listWork.GetCount() == 0)
    {
        m_evWork.Reset();
    }

    return preq;
}


// get an item from the done list
CAsyncRequest*
CAsyncIo::GetDoneItem()
{
    CAutoLock lock(&m_csLists);
    CAsyncRequest * preq  = m_listDone.RemoveHead();

    // force event set correctly if list now empty
    // or we're in the final stages of flushing
    // Note that during flushing the way it's supposed to work is that
    // everything is shoved on the Done list then the application is
    // supposed to pull until it gets nothing more
    //
    // Thus we should not set m_evDone unconditionally until everything
    // has moved to the done list which means we must wait until
    // cItemsOut is 0 (which is guaranteed by m_bWaiting being TRUE).

    if(m_listDone.GetCount() == 0 &&
        (!m_bFlushing || m_bWaiting))
    {
        m_evDone.Reset();
    }

    return preq;
}


// put an item on the work list - fail if bFlushing
HRESULT
CAsyncIo::PutWorkItem(CAsyncRequest* pRequest)
{
    CAutoLock lock(&m_csLists);
    HRESULT hr;

    if(m_bFlushing)
    {
        hr = VFW_E_WRONG_STATE;
    }
    else if(m_listWork.AddTail(pRequest))
    {
        // event should now be in a set state - force this
        m_evWork.Set();

        // start the thread now if not already started
        hr = StartThread();

    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return(hr);
}


// put an item on the done list - ok to do this when
// flushing
HRESULT
CAsyncIo::PutDoneItem(CAsyncRequest* pRequest)
{
    ASSERT(CritCheckIn(&m_csLists));

    if(m_listDone.AddTail(pRequest))
    {
        // event should now be in a set state - force this
        m_evDone.Set();
        return S_OK;
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}


// called on thread to process any active requests
void
CAsyncIo::ProcessRequests(void)
{
    // lock to get the item and increment the outstanding count
    CAsyncRequest * preq = NULL;

    for(;;)
    {
        {
            CAutoLock lock(&m_csLists);

            preq = GetWorkItem();
            if(preq == NULL)
            {
                // done
                return;
            }

            // one more item not on the done or work list
            m_cItemsOut++;

            // release critsec
        }

        preq->Complete();

        // regain critsec to replace on done list
        {
            CAutoLock l(&m_csLists);

            PutDoneItem(preq);

            if(--m_cItemsOut == 0)
            {
                if(m_bWaiting)
                    m_evAllDone.Set();
            }
        }
    }
}


// the thread proc - assumes that DWORD thread param is the
// this pointer
DWORD
CAsyncIo::ThreadProc(void)
{
    HANDLE ahev[] = {m_evStop, m_evWork};

    for(;;)
    {
        DWORD dw = WaitForMultipleObjects(2,
                                          ahev,
                                          FALSE,
                                          INFINITE);
        if(dw == WAIT_OBJECT_0+1)
        {
            // requests need processing
            ProcessRequests();
        }
        else
        {
            // any error or stop event - we should exit
            return 0;
        }
    }
}


// perform a synchronous read request on this thread.
// may not be aligned - so we will have to buffer.
HRESULT
CAsyncIo::SyncRead(
                LONGLONG llPos,
                LONG lLength,
                BYTE * pBuffer)
{
    if(IsAligned(llPos) &&
        IsAligned(lLength) &&
        IsAligned((LONG_PTR) pBuffer))
    {
        LONG cbUnused;
        return SyncReadAligned(llPos, lLength, pBuffer, &cbUnused, NULL);
    }

    // not aligned with requirements - use buffered file handle.
    //!!! might want to fix this to buffer the data ourselves?

    CAsyncRequest request;

    HRESULT hr = request.Request(this,
                                m_pStream,
                                llPos,
                                lLength,
                                FALSE,
                                pBuffer,
                                NULL,
                                0);

    if(FAILED(hr))
    {
        return hr;
    }

    return request.Complete();
}


//  Return the alignment
HRESULT
CAsyncIo::Alignment(LONG *pAlignment)
{
    CheckPointer(pAlignment,E_POINTER);

    *pAlignment = Alignment();
    return S_OK;
}


