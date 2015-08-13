// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "SurfaceQueueImpl.h"

//-----------------------------------------------------------------------------
// CSurfaceConsumer IUnknown implementation
//-----------------------------------------------------------------------------
HRESULT CSurfaceConsumer::QueryInterface(REFIID id, void** ppInterface)
{
    *ppInterface = NULL;
    if (id == __uuidof(ISurfaceConsumer))
    {
        *reinterpret_cast<ISurfaceConsumer**>(ppInterface) = this;
        AddRef();
        return S_OK;
    }
    else if (id == __uuidof(IUnknown))
    {
        *reinterpret_cast<ISurfaceConsumer**>(ppInterface) = this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

ULONG CSurfaceConsumer::AddRef()
{
    InterlockedIncrement(&m_RefCount);
    return m_RefCount;
}

ULONG CSurfaceConsumer::Release()
{
    InterlockedDecrement(&m_RefCount);
    ULONG RefCount = m_RefCount;
    if (m_RefCount == 0)
    {
        delete this;
    };
    return RefCount;
}

//-----------------------------------------------------------------------------
// CSurfaceProducer IUnknown implementation
//-----------------------------------------------------------------------------
HRESULT CSurfaceProducer::QueryInterface(REFIID id, void** ppInterface)
{
    *ppInterface = NULL;
    if (id == __uuidof(ISurfaceProducer))
    {
        *reinterpret_cast<ISurfaceProducer**>(ppInterface) = this;
        AddRef();
        return S_OK;
    }
    else if (id == __uuidof(IUnknown))
    {
        *reinterpret_cast<ISurfaceProducer**>(ppInterface) = this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

ULONG CSurfaceProducer::AddRef()
{
    InterlockedIncrement(&m_RefCount);
    return m_RefCount;
}

ULONG CSurfaceProducer::Release()
{
    InterlockedDecrement(&m_RefCount);
    ULONG RefCount = m_RefCount;
    if (m_RefCount == 0)
    {
        delete this;
    };
    return RefCount;
}

//-----------------------------------------------------------------------------
// CSurfaceQueue IUnknown implementation
//-----------------------------------------------------------------------------
HRESULT CSurfaceQueue::QueryInterface(REFIID id, void** ppInterface)
{
    *ppInterface = NULL;
    if (id == __uuidof(ISurfaceQueue))
    {
        *reinterpret_cast<ISurfaceQueue**>(ppInterface) = this;
        AddRef();
        return S_OK;
    }
    else if (id == __uuidof(IUnknown))
    {
        *reinterpret_cast<ISurfaceQueue**>(ppInterface) = this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

ULONG CSurfaceQueue::AddRef()
{
    InterlockedIncrement(&m_RefCount);
    return m_RefCount;
}

ULONG CSurfaceQueue::Release()
{
    InterlockedDecrement(&m_RefCount);
    ULONG RefCount = m_RefCount;
    if (m_RefCount == 0)
    {
        delete this;
    };
    return RefCount;
}


