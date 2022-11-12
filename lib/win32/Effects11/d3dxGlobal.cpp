//--------------------------------------------------------------------------------------
// File: d3dxGlobal.cpp
//
// Direct3D 11 Effects implementation for helper data structures
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/p/?LinkId=271568
//--------------------------------------------------------------------------------------

#include "pchfx.h"

#include <intsafe.h>

#include <stdio.h>
#include <stdarg.h>

namespace D3DX11Core
{

//////////////////////////////////////////////////////////////////////////
// CMemoryStream - A class to simplify reading binary data
//////////////////////////////////////////////////////////////////////////

CMemoryStream::CMemoryStream() noexcept :
    m_pData(nullptr),
    m_cbData(0),
    m_readPtr(0)
{
}

CMemoryStream::~CMemoryStream()
{
}

_Use_decl_annotations_
HRESULT CMemoryStream::SetData(const void *pData, size_t size)
{
    m_pData = (uint8_t*) pData;
    m_cbData = size;
    m_readPtr = 0;

    return S_OK;
}

_Use_decl_annotations_
HRESULT CMemoryStream::ReadAtOffset(size_t offset, size_t size, void **ppData)
{
    if (offset >= m_cbData)
        return E_FAIL;

    m_readPtr = offset;
    return Read(ppData, size);
}

_Use_decl_annotations_
HRESULT CMemoryStream::ReadAtOffset(size_t offset, LPCSTR *ppString)
{
    if (offset >= m_cbData)
        return E_FAIL;

    m_readPtr = offset;
    return Read(ppString);
}

_Use_decl_annotations_
HRESULT CMemoryStream::Read(void **ppData, size_t size)
{
    size_t temp = m_readPtr + size;

    if (temp < m_readPtr || temp > m_cbData)
        return E_FAIL;

    *ppData = m_pData + m_readPtr;
    m_readPtr = temp;
    return S_OK;
}

_Use_decl_annotations_
HRESULT CMemoryStream::Read(uint32_t *pDword)
{
    uint32_t *pTempDword;
    HRESULT hr;

    hr = Read((void**) &pTempDword, sizeof(uint32_t));
    if (FAILED(hr))
        return E_FAIL;

    *pDword = *pTempDword;
    return S_OK;
}

_Use_decl_annotations_
HRESULT CMemoryStream::Read(LPCSTR *ppString)
{
    size_t iChar=m_readPtr;
    for(; m_pData[iChar]; iChar++)
    {
        if (iChar > m_cbData)
            return E_FAIL;      
    }

    *ppString = (LPCSTR) (m_pData + m_readPtr);
    m_readPtr = iChar;

    return S_OK;
}

size_t CMemoryStream::GetPosition()
{
    return m_readPtr;
}

HRESULT CMemoryStream::Seek(_In_ size_t offset)
{
    if (offset > m_cbData)
        return E_FAIL;

    m_readPtr = offset;
    return S_OK;
}

}

//////////////////////////////////////////////////////////////////////////
// CDataBlock - used to dynamically build up the effect file in memory
//////////////////////////////////////////////////////////////////////////

CDataBlock::CDataBlock() noexcept :
    m_size(0),
    m_maxSize(0),
    m_pData(nullptr),
    m_pNext(nullptr),
    m_IsAligned(false)
{
}

CDataBlock::~CDataBlock()
{
    SAFE_DELETE_ARRAY(m_pData);
    SAFE_DELETE(m_pNext);
}

void CDataBlock::EnableAlignment()
{
    m_IsAligned = true;
}

_Use_decl_annotations_
HRESULT CDataBlock::AddData(const void *pvNewData, uint32_t bufferSize, CDataBlock **ppBlock)
{
    HRESULT hr = S_OK;
    uint32_t bytesToCopy;
    const uint8_t *pNewData = (const uint8_t*) pvNewData;

    if (m_maxSize == 0)
    {
        // This is a brand new DataBlock, fill it up
        m_maxSize = std::max<uint32_t>(8192, bufferSize);

        VN( m_pData = new uint8_t[m_maxSize] );
    }

    assert(m_pData == AlignToPowerOf2(m_pData, c_DataAlignment));

    bytesToCopy = std::min(m_maxSize - m_size, bufferSize);
    memcpy(m_pData + m_size, pNewData, bytesToCopy);
    pNewData += bytesToCopy;
    
    if (m_IsAligned)
    {
        assert(m_size == AlignToPowerOf2(m_size, c_DataAlignment));
        m_size += AlignToPowerOf2(bytesToCopy, c_DataAlignment);
    }
    else
    {
        m_size += bytesToCopy;
    }
    
    bufferSize -= bytesToCopy;
    *ppBlock = this;

    if (bufferSize != 0)
    {
        assert(nullptr == m_pNext); // make sure we're not overwriting anything

        // Couldn't fit all data into this block, spill over into next
        VN( m_pNext = new CDataBlock() );
        if (m_IsAligned)
        {
            m_pNext->EnableAlignment();
        }
        VH( m_pNext->AddData(pNewData, bufferSize, ppBlock) );
    }

lExit:
    return hr;
}

_Use_decl_annotations_
void* CDataBlock::Allocate(uint32_t bufferSize, CDataBlock **ppBlock)
{
    void *pRetValue;
    uint32_t temp = m_size + bufferSize;

    if (temp < m_size)
        return nullptr;

    *ppBlock = this;

    if (m_maxSize == 0)
    {
        // This is a brand new DataBlock, fill it up
        m_maxSize = std::max<uint32_t>(8192, bufferSize);

        m_pData = new uint8_t[m_maxSize];
        if (!m_pData)
            return nullptr;
        memset(m_pData, 0xDD, m_maxSize);
    }
    else if (temp > m_maxSize)
    {
        assert(nullptr == m_pNext); // make sure we're not overwriting anything

        // Couldn't fit data into this block, spill over into next
        m_pNext = new CDataBlock();
        if (!m_pNext)
            return nullptr;
        if (m_IsAligned)
        {
            m_pNext->EnableAlignment();
        }

        return m_pNext->Allocate(bufferSize, ppBlock);
    }

    assert(m_pData == AlignToPowerOf2(m_pData, c_DataAlignment));

    pRetValue = m_pData + m_size;
    if (m_IsAligned)
    {
        assert(m_size == AlignToPowerOf2(m_size, c_DataAlignment));
        m_size = AlignToPowerOf2(temp, c_DataAlignment);
    }
    else
    {
        m_size = temp;
    }

    return pRetValue;
}


//////////////////////////////////////////////////////////////////////////

CDataBlockStore::CDataBlockStore() noexcept :
    m_pFirst(nullptr),
    m_pLast(nullptr),
    m_Size(0),
    m_Offset(0),
    m_IsAligned(false)
{
#ifdef _DEBUG
    m_cAllocations = 0;
#endif
}

CDataBlockStore::~CDataBlockStore()
{
    // Can't just do SAFE_DELETE(m_pFirst) since it blows the stack when deleting long chains of data
    CDataBlock* pData = m_pFirst;
    while(pData)
    {
        CDataBlock* pCurrent = pData;
        pData = pData->m_pNext;
        pCurrent->m_pNext = nullptr;
        delete pCurrent;
    }

    // m_pLast will be deleted automatically
}

void CDataBlockStore::EnableAlignment()
{
    m_IsAligned = true;
}

_Use_decl_annotations_
HRESULT CDataBlockStore::AddString(LPCSTR pString, uint32_t *pOffset)
{
    size_t strSize = strlen(pString) + 1;
    assert( strSize <= 0xffffffff );
    return AddData(pString, (uint32_t)strSize, pOffset);
}

_Use_decl_annotations_
HRESULT CDataBlockStore::AddData(const void *pNewData, uint32_t bufferSize, uint32_t *pCurOffset)
{
    HRESULT hr = S_OK;

    if (bufferSize == 0)
    {        
        if (pCurOffset)
        {
            *pCurOffset = 0;
        }
        goto lExit;
    }

    if (!m_pFirst)
    {
        VN( m_pFirst = new CDataBlock() );
        if (m_IsAligned)
        {
            m_pFirst->EnableAlignment();
        }
        m_pLast = m_pFirst;
    }

    if (pCurOffset)
        *pCurOffset = m_Size + m_Offset;

    VH( m_pLast->AddData(pNewData, bufferSize, &m_pLast) );
    m_Size += bufferSize;

lExit:
    return hr;
}

void* CDataBlockStore::Allocate(_In_ uint32_t bufferSize)
{
    void *pRetValue = nullptr;

#ifdef _DEBUG
    m_cAllocations++;
#endif

    if (!m_pFirst)
    {
        m_pFirst = new CDataBlock();
        if (!m_pFirst)
            return nullptr;

        if (m_IsAligned)
        {
            m_pFirst->EnableAlignment();
        }
        m_pLast = m_pFirst;
    }

    if (FAILED(UIntAdd(m_Size, bufferSize, &m_Size)))
        return nullptr;

    pRetValue = m_pLast->Allocate(bufferSize, &m_pLast);
    if (!pRetValue)
        return nullptr;

    return pRetValue;
}

uint32_t CDataBlockStore::GetSize()
{
    return m_Size;
}


//////////////////////////////////////////////////////////////////////////

static bool s_mute = false;

bool D3DX11DebugMute(bool mute)
{
    bool previous = s_mute;
    s_mute = mute;
    return previous;
}

#ifdef _DEBUG
_Use_decl_annotations_
void __cdecl D3DXDebugPrintf(UINT lvl, LPCSTR szFormat, ...)
{
    if (s_mute)
        return;

    UNREFERENCED_PARAMETER(lvl);

    char strA[4096] = {};
    char strB[4096] = {};

    va_list ap;
    va_start(ap, szFormat);
    vsprintf_s(strA, sizeof(strA), szFormat, ap);
    strA[4095] = '\0';
    va_end(ap);

    sprintf_s(strB, sizeof(strB), "Effects11: %s\r\n", strA);

    strB[4095] = '\0';

    OutputDebugStringA(strB);
}
#endif // _DEBUG
