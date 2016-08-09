//--------------------------------------------------------------------------------------
// File: D3DXGlobal.h
//
// Direct3D 11 Effects helper defines and data structures
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/p/?LinkId=271568
//--------------------------------------------------------------------------------------

#pragma once

#include <assert.h>
#include <string.h>

namespace D3DX11Debug
{
    // Helper sets a D3D resource name string (used by PIX and debug layer leak reporting).
    inline void SetDebugObjectName(_In_ ID3D11DeviceChild* resource, _In_z_ const char *name )
    {
        #if !defined(NO_D3D11_DEBUG_NAME) && ( defined(_DEBUG) || defined(PROFILE) )
            resource->SetPrivateData( WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name );
        #else
            UNREFERENCED_PARAMETER(resource);
            UNREFERENCED_PARAMETER(name);
        #endif
    }

    template<UINT TNameLength>
    inline void SetDebugObjectName(_In_ ID3D11DeviceChild* resource, _In_z_ const char (&name)[TNameLength])
    {
        #if !defined(NO_D3D11_DEBUG_NAME) && ( defined(_DEBUG) || defined(PROFILE) )
            resource->SetPrivateData(WKPDID_D3DDebugObjectName, TNameLength - 1, name);
        #else
            UNREFERENCED_PARAMETER(resource);
            UNREFERENCED_PARAMETER(name);
        #endif
    }
}

using namespace D3DX11Debug;

#define SAFE_RELEASE(p)       { if (p) { (p)->Release();  (p) = nullptr; } }
#define SAFE_ADDREF(p)        { if (p) { (p)->AddRef(); } }

#define SAFE_DELETE_ARRAY(p)  { delete [](p); p = nullptr; }
#define SAFE_DELETE(p)        { delete (p); p = nullptr;  }

#if FXDEBUG
#define __BREAK_ON_FAIL       { __debugbreak(); }
#else
#define __BREAK_ON_FAIL 
#endif

#define VA(x, action) { hr = (x); if (FAILED(hr)) { action; __BREAK_ON_FAIL;                     return hr;  } }
#define VNA(x,action) {           if (!(x))       { action; __BREAK_ON_FAIL; hr = E_OUTOFMEMORY; goto lExit; } }
#define VBA(x,action) {           if (!(x))       { action; __BREAK_ON_FAIL; hr = E_FAIL;        goto lExit; } }
#define VHA(x,action) { hr = (x); if (FAILED(hr)) { action; __BREAK_ON_FAIL;                     goto lExit; } }

#define V(x)          { VA (x, 0) }
#define VN(x)         { VNA(x, 0) }
#define VB(x)         { VBA(x, 0) }
#define VH(x)         { VHA(x, 0) }

#define VBD(x,str)         { VBA(x, DPF(1,str)) }
#define VHD(x,str)         { VHA(x, DPF(1,str)) }

#define VEASSERT(x)   { hr = (x); if (FAILED(hr)) { __BREAK_ON_FAIL; assert(!#x);                     goto lExit; } }
#define VNASSERT(x)   {           if (!(x))       { __BREAK_ON_FAIL; assert(!#x); hr = E_OUTOFMEMORY; goto lExit; } }

#define D3DX11FLTASSIGN(a,b)    { *reinterpret_cast< UINT32* >(&(a)) = *reinterpret_cast< UINT32* >(&(b)); }

// Preferred data alignment -- must be a power of 2!
static const uint32_t c_DataAlignment = sizeof(UINT_PTR);

inline uint32_t AlignToPowerOf2(uint32_t Value, uint32_t Alignment)
{
    assert((Alignment & (Alignment - 1)) == 0);
    // to align to 2^N, add 2^N - 1 and AND with all but lowest N bits set
    _Analysis_assume_(Alignment > 0 && Value < MAXDWORD - Alignment);
    return (Value + Alignment - 1) & (~(Alignment - 1));
}

inline void * AlignToPowerOf2(void *pValue, UINT_PTR Alignment)
{
    assert((Alignment & (Alignment - 1)) == 0);
    // to align to 2^N, add 2^N - 1 and AND with all but lowest N bits set
    return (void *)(((UINT_PTR)pValue + Alignment - 1) & (~((UINT_PTR)Alignment - 1)));
}

namespace D3DX11Core
{

//////////////////////////////////////////////////////////////////////////
// CMemoryStream - A class to simplify reading binary data
//////////////////////////////////////////////////////////////////////////
class CMemoryStream
{
    uint8_t *m_pData;
    size_t  m_cbData;
    size_t  m_readPtr;

public:
    HRESULT SetData(_In_reads_bytes_(size) const void *pData, _In_ size_t size);

    HRESULT Read(_Out_ uint32_t *pUint);
    HRESULT Read(_Outptr_result_buffer_(size) void **ppData, _In_ size_t size);
    HRESULT Read(_Outptr_ LPCSTR *ppString);

    HRESULT ReadAtOffset(_In_ size_t offset, _In_ size_t size, _Outptr_result_buffer_(size) void **ppData);
    HRESULT ReadAtOffset(_In_ size_t offset, _Outptr_result_z_ LPCSTR *ppString);

    size_t  GetPosition();
    HRESULT Seek(_In_ size_t offset);

    CMemoryStream();
    ~CMemoryStream();
};

}

#if defined(_DEBUG) && !defined(_M_X64)

namespace D3DX11Debug
{

// This variable indicates how many ticks to go before rolling over
// all of the timer variables in the FX system.
// It is read from the system registry in debug builds; in retail the high bit is simply tested.

_declspec(selectany) unsigned int g_TimerRolloverCount = 0x80000000;
}

#endif // _DEBUG && !_M_X64


//////////////////////////////////////////////////////////////////////////
// CEffectVector - A vector implementation
//////////////////////////////////////////////////////////////////////////

template<class T> class CEffectVector
{
protected:
#if _DEBUG
    T       *m_pCastData; // makes debugging easier to have a casted version of the data
#endif // _DEBUG

    uint8_t    *m_pData;
    uint32_t    m_MaxSize;
    uint32_t    m_CurSize;

    HRESULT Grow()
    {
        return Reserve(m_CurSize + 1);
    }

    HRESULT Reserve(_In_ uint32_t DesiredSize)
    {
        if (DesiredSize > m_MaxSize)
        {
            uint8_t *pNewData;
            uint32_t newSize = std::max(m_MaxSize * 2, DesiredSize);

            if (newSize < 16)
                newSize = 16;

            if ((newSize < m_MaxSize) || (newSize < m_CurSize) || (newSize >= UINT_MAX / sizeof(T)))
            {
                m_hLastError = E_OUTOFMEMORY;
                return m_hLastError;
            }

            pNewData = new uint8_t[newSize * sizeof(T)];
            if (pNewData == nullptr)
            {
                m_hLastError = E_OUTOFMEMORY;
                return m_hLastError;
            }

            if (m_pData)
            {
                memcpy(pNewData, m_pData, m_CurSize * sizeof(T));
                delete []m_pData;
            }

            m_pData = pNewData;
            m_MaxSize = newSize;
        }
#if _DEBUG
        m_pCastData = (T*) m_pData;
#endif // _DEBUG
        return S_OK;
    }

public:
    HRESULT m_hLastError;

    CEffectVector<T>() : m_hLastError(S_OK), m_pData(nullptr), m_CurSize(0), m_MaxSize(0)
    {
#if _DEBUG
        m_pCastData = nullptr;
#endif // _DEBUG
    }

    ~CEffectVector<T>()
    {
        Clear();
    }

    // cleanly swaps two vectors -- useful for when you want
    // to reallocate a vector and copy data over, then swap them back
    void SwapVector(_Out_ CEffectVector<T> &vOther)
    {
        uint8_t tempData[sizeof(*this)];

        memcpy(tempData, this, sizeof(*this));
        memcpy(this, &vOther, sizeof(*this));
        memcpy(&vOther, tempData, sizeof(*this));
    }

    HRESULT CopyFrom(_In_ const CEffectVector<T> &vOther)
    {
        HRESULT hr = S_OK;
        Clear();
        VN( m_pData = new uint8_t[vOther.m_MaxSize * sizeof(T)] );
        
        m_CurSize = vOther.m_CurSize;
        m_MaxSize = vOther.m_MaxSize;
        m_hLastError = vOther.m_hLastError;

        for (size_t i = 0; i < m_CurSize; ++ i)
        {
            ((T*)m_pData)[i] = ((T*)vOther.m_pData)[i];
        }

lExit:

#if _DEBUG
        m_pCastData = (T*) m_pData;
#endif // _DEBUG

        return hr;
    }

    void Clear()
    {
        Empty();
        SAFE_DELETE_ARRAY(m_pData);
        m_MaxSize = 0;
#if _DEBUG
        m_pCastData = nullptr;
#endif // _DEBUG
    }

    void ClearWithoutDestructor()
    {
        m_CurSize = 0;
        m_hLastError = S_OK;
        SAFE_DELETE_ARRAY(m_pData);
        m_MaxSize = 0;

#if _DEBUG
        m_pCastData = nullptr;
#endif // _DEBUG
    }

    void Empty()
    {
       
        // manually invoke destructor on all elements
        for (size_t i = 0; i < m_CurSize; ++ i)
        {   
            ((T*)m_pData + i)->~T();
        }
        m_CurSize = 0;
        m_hLastError = S_OK;
    }

    T* Add()
    {
        if (FAILED(Grow()))
            return nullptr;

        // placement new
        return new((T*)m_pData + (m_CurSize ++)) T;
    }

    T* AddRange(_In_ uint32_t count)
    {
        if (m_CurSize + count < m_CurSize)
        {
            m_hLastError = E_OUTOFMEMORY;
            return nullptr;
        }

        if (FAILED(Reserve(m_CurSize + count)))
            return nullptr;

        T *pData = (T*)m_pData + m_CurSize;
        for (size_t i = 0; i < count; ++ i)
        {
            new(pData + i) T;
        }
        m_CurSize += count;
        return pData;
    }

    HRESULT Add(_In_ const T& var)
    {
        if (FAILED(Grow()))
            return m_hLastError;

        memcpy((T*)m_pData + m_CurSize, &var, sizeof(T));
        m_CurSize++;

        return S_OK;
    }

    HRESULT AddRange(_In_reads_(count) const T *pVar, _In_ uint32_t count)
    {
        if (m_CurSize + count < m_CurSize)
        {
            m_hLastError = E_OUTOFMEMORY;
            return m_hLastError;
        }

        if (FAILED(Reserve(m_CurSize + count)))
            return m_hLastError;

        memcpy((T*)m_pData + m_CurSize, pVar, count * sizeof(T));
        m_CurSize += count;

        return S_OK;
    }

    HRESULT Insert(_In_ const T& var, _In_ uint32_t index)
    {
        assert(index < m_CurSize);
        
        if (FAILED(Grow()))
            return m_hLastError;

        memmove((T*)m_pData + index + 1, (T*)m_pData + index, (m_CurSize - index) * sizeof(T));
        memcpy((T*)m_pData + index, &var, sizeof(T));
        m_CurSize++;

        return S_OK;
    }

    HRESULT InsertRange(_In_reads_(count) const T *pVar, _In_ uint32_t index, _In_ uint32_t count)
    {
        assert(index < m_CurSize);
        
        if (m_CurSize + count < m_CurSize)
        {
            m_hLastError = E_OUTOFMEMORY;
            return m_hLastError;
        }

        if (FAILED(Reserve(m_CurSize + count)))
            return m_hLastError;

        memmove((T*)m_pData + index + count, (T*)m_pData + index, (m_CurSize - index) * sizeof(T));
        memcpy((T*)m_pData + index, pVar, count * sizeof(T));
        m_CurSize += count;

        return S_OK;
    }

    inline T& operator[](_In_ size_t index)
    {
        assert(index < m_CurSize);
        return ((T*)m_pData)[index];
    }

    // Deletes element at index and shifts all other values down
    void Delete(_In_ uint32_t index)
    {
        assert(index < m_CurSize);

        -- m_CurSize;
        memmove((T*)m_pData + index, (T*)m_pData + index + 1, (m_CurSize - index) * sizeof(T));
    }

    // Deletes element at index and moves the last element into its place
    void QuickDelete(_In_ uint32_t index)
    {
        assert(index < m_CurSize);

        -- m_CurSize;
        memcpy((T*)m_pData + index, (T*)m_pData + m_CurSize, sizeof(T));
    }

    inline uint32_t GetSize() const
    {
        return m_CurSize;
    }

    inline T* GetData() const
    {
        return (T*)m_pData;
    }

    uint32_t FindIndexOf(_In_ const void *pEntry) const
    {
        for (size_t i = 0; i < m_CurSize; ++ i)
        {   
            if (((T*)m_pData + i) == pEntry)
                return i;
        }

        return -1;
    }

    void Sort(int (__cdecl *pfnCompare)(const void *pElem1, const void *pElem2))
    {
        qsort(m_pData, m_CurSize, sizeof(T), pfnCompare);
    }
};

//////////////////////////////////////////////////////////////////////////
// CEffectVectorOwner - implements a vector of ptrs to objects. The vector owns the objects.
//////////////////////////////////////////////////////////////////////////
template<class T> class CEffectVectorOwner : public CEffectVector<T*>
{
public:
    ~CEffectVectorOwner<T>()
    {
        Clear();

        for (size_t i=0; i<m_CurSize; i++)
            SAFE_DELETE(((T**)m_pData)[i]);

        SAFE_DELETE_ARRAY(m_pData);
    }

    void Clear()
    {
        Empty();
        SAFE_DELETE_ARRAY(m_pData);
        m_MaxSize = 0;
    }

    void Empty()
    {
        // manually invoke destructor on all elements
        for (size_t i = 0; i < m_CurSize; ++ i)
        {
            SAFE_DELETE(((T**)m_pData)[i]);
        }
        m_CurSize = 0;
        m_hLastError = S_OK;
    }

    void Delete(_In_ uint32_t index)
    {
        assert(index < m_CurSize);

        SAFE_DELETE(((T**)m_pData)[index]);

        CEffectVector<T*>::Delete(index);
    }
};

//////////////////////////////////////////////////////////////////////////
// Checked uint32_t, uint64_t
// Use CheckedNumber only with uint32_t and uint64_t
//////////////////////////////////////////////////////////////////////////
template <class T, T MaxValue> class CheckedNumber
{
    T       m_Value;
    bool    m_bValid;

public:
    CheckedNumber<T, MaxValue>() : m_Value(0), m_bValid(true)
    {
    }

    CheckedNumber<T, MaxValue>(const T &value) : m_Value(value), m_bValid(true)
    {
    }

    CheckedNumber<T, MaxValue>(const CheckedNumber<T, MaxValue> &value) : m_bValid(value.m_bValid), m_Value(value.m_Value)
    {
    }

    CheckedNumber<T, MaxValue> &operator+(const CheckedNumber<T, MaxValue> &other)
    {
        CheckedNumber<T, MaxValue> Res(*this);
        return Res+=other;
    }

    CheckedNumber<T, MaxValue> &operator+=(const CheckedNumber<T, MaxValue> &other)
    {
        if (!other.m_bValid)
        {
            m_bValid = false;
        }
        else
        {
            m_Value += other.m_Value;

            if (m_Value < other.m_Value)
                m_bValid = false;
        }

        return *this;
    }

    CheckedNumber<T, MaxValue> &operator*(const CheckedNumber<T, MaxValue> &other)
    {
        CheckedNumber<T, MaxValue> Res(*this);
        return Res*=other;
    }

    CheckedNumber<T, MaxValue> &operator*=(const CheckedNumber<T, MaxValue> &other)
    {
        if (!other.m_bValid)
        {
            m_bValid = false;
        }
        else
        {
            if (other.m_Value != 0)
            {
                if (m_Value > MaxValue / other.m_Value)
                {
                    m_bValid = false;
                }
            }
            m_Value *= other.m_Value;
        }

        return *this;
    }

    HRESULT GetValue(_Out_ T *pValue)
    {
        if (!m_bValid)
        {
            *pValue = uint32_t(-1);
            return E_FAIL;
        }

        *pValue = m_Value;
        return S_OK;
    }
};

typedef CheckedNumber<uint32_t, _UI32_MAX> CCheckedDword;
typedef CheckedNumber<uint64_t, _UI64_MAX> CCheckedDword64;


//////////////////////////////////////////////////////////////////////////
// Data Block Store - A linked list of allocations
//////////////////////////////////////////////////////////////////////////

class CDataBlock
{
protected:
    uint32_t    m_size;
    uint32_t    m_maxSize;
    uint8_t     *m_pData;
    CDataBlock  *m_pNext;

    bool        m_IsAligned;        // Whether or not to align the data to c_DataAlignment

public:
    // AddData appends an existing use buffer to the data block
    HRESULT AddData(_In_reads_bytes_(bufferSize) const void *pNewData, _In_ uint32_t bufferSize, _Outptr_ CDataBlock **ppBlock);

    // Allocate reserves bufferSize bytes of contiguous memory and returns a pointer to the user
    _Success_(return != nullptr)
    void*   Allocate(_In_ uint32_t bufferSize, _Outptr_ CDataBlock **ppBlock);

    void    EnableAlignment();

    CDataBlock();
    ~CDataBlock();

    friend class CDataBlockStore;
};


class CDataBlockStore
{
protected:
    CDataBlock  *m_pFirst;
    CDataBlock  *m_pLast;
    uint32_t    m_Size;
    uint32_t    m_Offset;           // m_Offset gets added to offsets returned from AddData & AddString. Use this to set a global for the entire string block
    bool        m_IsAligned;        // Whether or not to align the data to c_DataAlignment

public:
#if _DEBUG
    uint32_t    m_cAllocations;
#endif

public:
    HRESULT AddString(_In_z_ LPCSTR pString, _Inout_ uint32_t *pOffset);
        // Writes a null-terminated string to buffer

    HRESULT AddData(_In_reads_bytes_(bufferSize) const void *pNewData, _In_ uint32_t bufferSize, _Inout_ uint32_t *pOffset);
        // Writes data block to buffer

    // Memory allocator support
    void*   Allocate(_In_ uint32_t bufferSize);
    uint32_t GetSize();
    void    EnableAlignment();

    CDataBlockStore();
    ~CDataBlockStore();
};

// Custom allocator that uses CDataBlockStore
// The trick is that we never free, so we don't have to keep as much state around
// Use PRIVATENEW in CEffectLoader

inline void* __cdecl operator new(_In_ size_t s, _In_ CDataBlockStore &pAllocator)
{
#ifdef _M_X64
    assert( s <= 0xffffffff );
#endif
    return pAllocator.Allocate( (uint32_t)s );
}

inline void __cdecl operator delete(_In_opt_ void* p, _In_ CDataBlockStore &pAllocator)
{
    UNREFERENCED_PARAMETER(p);
    UNREFERENCED_PARAMETER(pAllocator);
}


//////////////////////////////////////////////////////////////////////////
// Hash table
//////////////////////////////////////////////////////////////////////////

#define HASH_MIX(a,b,c) \
{ \
    a -= b; a -= c; a ^= (c>>13); \
    b -= c; b -= a; b ^= (a<<8); \
    c -= a; c -= b; c ^= (b>>13); \
    a -= b; a -= c; a ^= (c>>12);  \
    b -= c; b -= a; b ^= (a<<16); \
    c -= a; c -= b; c ^= (b>>5); \
    a -= b; a -= c; a ^= (c>>3);  \
    b -= c; b -= a; b ^= (a<<10); \
    c -= a; c -= b; c ^= (b>>15); \
}

static uint32_t ComputeHash(_In_reads_bytes_(cbToHash) const uint8_t *pb, _In_ uint32_t cbToHash)
{
    uint32_t cbLeft = cbToHash;

    uint32_t a;
    uint32_t b;
    a = b = 0x9e3779b9; // the golden ratio; an arbitrary value

    uint32_t c = 0;

    while (cbLeft >= 12)
    {
        const uint32_t *pdw = reinterpret_cast<const uint32_t *>(pb);

        a += pdw[0];
        b += pdw[1];
        c += pdw[2];

        HASH_MIX(a,b,c);
        pb += 12; 
        cbLeft -= 12;
    }

    c += cbToHash;

    switch(cbLeft) // all the case statements fall through
    {
    case 11: c+=((uint32_t) pb[10] << 24);
    case 10: c+=((uint32_t) pb[9]  << 16);
    case 9 : c+=((uint32_t) pb[8]  <<  8);
        // the first byte of c is reserved for the length
    case 8 : b+=((uint32_t) pb[7]  << 24);
    case 7 : b+=((uint32_t) pb[6]  << 16);
    case 6 : b+=((uint32_t) pb[5]  <<  8);
    case 5 : b+=pb[4];
    case 4 : a+=((uint32_t) pb[3]  << 24);
    case 3 : a+=((uint32_t) pb[2]  << 16);
    case 2 : a+=((uint32_t) pb[1]  <<  8);
    case 1 : a+=pb[0];
    }

    HASH_MIX(a,b,c);

    return c;
}

static uint32_t ComputeHashLower(_In_reads_bytes_(cbToHash) const uint8_t *pb, _In_ uint32_t cbToHash)
{
    uint32_t cbLeft = cbToHash;

    uint32_t a;
    uint32_t b;
    a = b = 0x9e3779b9; // the golden ratio; an arbitrary value
    uint32_t c = 0;

    while (cbLeft >= 12)
    {
        uint8_t pbT[12];
        for( size_t i = 0; i < 12; i++ )
            pbT[i] = (uint8_t)tolower(pb[i]);

        uint32_t *pdw = reinterpret_cast<uint32_t *>(pbT);

        a += pdw[0];
        b += pdw[1];
        c += pdw[2];

        HASH_MIX(a,b,c);
        pb += 12; 
        cbLeft -= 12;
    }

    c += cbToHash;

    uint8_t pbT[12];
    for( size_t i = 0; i < cbLeft; i++ )
        pbT[i] = (uint8_t)tolower(pb[i]);

    switch(cbLeft) // all the case statements fall through
    {
    case 11: c+=((uint32_t) pbT[10] << 24);
    case 10: c+=((uint32_t) pbT[9]  << 16);
    case 9 : c+=((uint32_t) pbT[8]  <<  8);
        // the first byte of c is reserved for the length
    case 8 : b+=((uint32_t) pbT[7]  << 24);
    case 7 : b+=((uint32_t) pbT[6]  << 16);
    case 6 : b+=((uint32_t) pbT[5]  <<  8);
    case 5 : b+=pbT[4];
    case 4 : a+=((uint32_t) pbT[3]  << 24);
    case 3 : a+=((uint32_t) pbT[2]  << 16);
    case 2 : a+=((uint32_t) pbT[1]  <<  8);
    case 1 : a+=pbT[0];
    }

    HASH_MIX(a,b,c);

    return c;
}

static uint32_t ComputeHash(_In_z_ LPCSTR pString)
{
    return ComputeHash(reinterpret_cast<const uint8_t*>(pString), (uint32_t)strlen(pString));
}


// 1) these numbers are prime
// 2) each is slightly less than double the last
// 4) each is roughly in between two powers of 2;
//    (2^n hash table sizes are VERY BAD; they effectively truncate your
//     precision down to the n least significant bits of the hash)
static const uint32_t c_PrimeSizes[] = 
{
    11,
    23,
    53,
    97,
    193,
    389,
    769,
    1543,
    3079,
    6151,
    12289,
    24593,
    49157,
    98317,
    196613,
    393241,
    786433,
    1572869,
    3145739,
    6291469,
    12582917,
    25165843,
    50331653,
    100663319,
    201326611,
    402653189,
    805306457,
    1610612741,
};

template<typename T, bool (*pfnIsEqual)(const T &Data1, const T &Data2)>
class CEffectHashTable
{
protected:

    struct SHashEntry
    {
        uint32_t    Hash;
        T           Data;
        SHashEntry  *pNext;
    };

    // Array of hash entries
    SHashEntry  **m_rgpHashEntries;
    uint32_t    m_NumHashSlots;
    uint32_t    m_NumEntries;
    bool        m_bOwnHashEntryArray;

public:
    class CIterator
    {
        friend class CEffectHashTable;

    protected:
        SHashEntry **ppHashSlot;
        SHashEntry *pHashEntry;

    public:
        T GetData()
        {
            assert(pHashEntry != 0);
            _Analysis_assume_(pHashEntry != 0);
            return pHashEntry->Data;
        }

        uint32_t GetHash()
        {
            assert(pHashEntry != 0);
            _Analysis_assume_(pHashEntry != 0);
            return pHashEntry->Hash;
        }
    };

    CEffectHashTable() : m_rgpHashEntries(nullptr), m_NumHashSlots(0), m_NumEntries(0), m_bOwnHashEntryArray(false)
    {
    }

    HRESULT Initialize(_In_ const CEffectHashTable *pOther)
    {
        HRESULT hr = S_OK;
        SHashEntry **rgpNewHashEntries = nullptr;
        uint32_t valuesMigrated = 0;
        uint32_t actualSize;

        Cleanup();

        actualSize = pOther->m_NumHashSlots;
        VN( rgpNewHashEntries = new SHashEntry*[actualSize] );

        ZeroMemory(rgpNewHashEntries, sizeof(SHashEntry*) * actualSize);

        // Expensive operation: rebuild the hash table
        CIterator iter, nextIter;
        pOther->GetFirstEntry(&iter);
        while (!pOther->PastEnd(&iter))
        {
            uint32_t index = iter.GetHash() % actualSize;

            // we need to advance to the next element
            // before we seize control of this element and move
            // it to the new table
            nextIter = iter;
            pOther->GetNextEntry(&nextIter);

            // seize this hash entry, migrate it to the new table
            SHashEntry *pNewEntry;
            VN( pNewEntry = new SHashEntry );
            
            pNewEntry->pNext = rgpNewHashEntries[index];
            pNewEntry->Data = iter.pHashEntry->Data;
            pNewEntry->Hash = iter.pHashEntry->Hash;
            rgpNewHashEntries[index] = pNewEntry;

            iter = nextIter;
            ++ valuesMigrated;
        }

        assert(valuesMigrated == pOther->m_NumEntries);

        m_rgpHashEntries = rgpNewHashEntries;
        m_NumHashSlots = actualSize;
        m_NumEntries = pOther->m_NumEntries;
        m_bOwnHashEntryArray = true;
        rgpNewHashEntries = nullptr;

lExit:
        SAFE_DELETE_ARRAY( rgpNewHashEntries );
        return hr;
    }

protected:
    void CleanArray()
    {
        if (m_bOwnHashEntryArray)
        {
            SAFE_DELETE_ARRAY(m_rgpHashEntries);
            m_bOwnHashEntryArray = false;
        }
    }

public:
    void Cleanup()
    {
        for (size_t i = 0; i < m_NumHashSlots; ++ i)
        {
            SHashEntry *pCurrentEntry = m_rgpHashEntries[i];
            SHashEntry *pTempEntry;
            while (nullptr != pCurrentEntry)
            {
                pTempEntry = pCurrentEntry->pNext;
                SAFE_DELETE(pCurrentEntry);
                pCurrentEntry = pTempEntry;
                -- m_NumEntries;
            }
        }
        CleanArray();
        m_NumHashSlots = 0;
        assert(m_NumEntries == 0);
    }

    ~CEffectHashTable()
    {
        Cleanup();
    }

    static uint32_t GetNextHashTableSize(_In_ uint32_t DesiredSize)
    {
        // figure out the next logical size to use
        for (size_t i = 0; i < _countof(c_PrimeSizes); ++i )
        {
            if (c_PrimeSizes[i] >= DesiredSize)
            {
                return c_PrimeSizes[i];
            }
        }

        return DesiredSize;
    }
    
    // O(n) function
    // Grows to the next suitable size (based off of the prime number table)
    // DesiredSize is merely a suggestion
    HRESULT Grow(_In_ uint32_t DesiredSize,
                 _In_ uint32_t ProvidedArraySize = 0,
                 _In_reads_opt_(ProvidedArraySize) void** ProvidedArray = nullptr,
                 _In_ bool OwnProvidedArray = false)
    {
        HRESULT hr = S_OK;
        SHashEntry **rgpNewHashEntries = nullptr;
        uint32_t valuesMigrated = 0;
        uint32_t actualSize;

        VB( DesiredSize > m_NumHashSlots );

        actualSize = GetNextHashTableSize(DesiredSize);

        if (ProvidedArray &&
            ProvidedArraySize >= actualSize)
        {
            rgpNewHashEntries = reinterpret_cast<SHashEntry**>(ProvidedArray);
        }
        else
        {
            OwnProvidedArray = true;
            
            VN( rgpNewHashEntries = new SHashEntry*[actualSize] );
        }
        
        ZeroMemory(rgpNewHashEntries, sizeof(SHashEntry*) * actualSize);

        // Expensive operation: rebuild the hash table
        CIterator iter, nextIter;
        GetFirstEntry(&iter);
        while (!PastEnd(&iter))
        {
            uint32_t index = iter.GetHash() % actualSize;

            // we need to advance to the next element
            // before we seize control of this element and move
            // it to the new table
            nextIter = iter;
            GetNextEntry(&nextIter);

            // seize this hash entry, migrate it to the new table
            iter.pHashEntry->pNext = rgpNewHashEntries[index];
            rgpNewHashEntries[index] = iter.pHashEntry;

            iter = nextIter;
            ++ valuesMigrated;
        }

        assert(valuesMigrated == m_NumEntries);

        CleanArray();
        m_rgpHashEntries = rgpNewHashEntries;
        m_NumHashSlots = actualSize;
        m_bOwnHashEntryArray = OwnProvidedArray;

lExit:
        return hr;
    }

    HRESULT AutoGrow()
    {
        // arbitrary heuristic -- grow if 1:1
        if (m_NumEntries >= m_NumHashSlots)
        {
            // grows this hash table so that it is roughly 50% full
            return Grow(m_NumEntries * 2 + 1);
        }
        return S_OK;
    }

#if _DEBUG
    void PrintHashTableStats()
    {
        if (m_NumHashSlots == 0)
        {
            DPF(0, "Uninitialized hash table!");
            return;
        }
        
        float variance = 0.0f;
        float mean = (float)m_NumEntries / (float)m_NumHashSlots;
        uint32_t unusedSlots = 0;

        DPF(0, "Hash table slots: %d, Entries in table: %d", m_NumHashSlots, m_NumEntries);

        for (size_t i = 0; i < m_NumHashSlots; ++ i)
        {
            uint32_t entries = 0;
            SHashEntry *pCurrentEntry = m_rgpHashEntries[i];

            while (nullptr != pCurrentEntry)
            {
                SHashEntry *pCurrentEntry2 = m_rgpHashEntries[i];
                
                // check other hash entries in this slot for hash collisions or duplications
                while (pCurrentEntry2 != pCurrentEntry)
                {
                    if (pCurrentEntry->Hash == pCurrentEntry2->Hash)
                    {
                        if (pfnIsEqual(pCurrentEntry->Data, pCurrentEntry2->Data))
                        {
                            assert(0);
                            DPF(0, "Duplicate entry (identical hash, identical data) found!");
                        }
                        else
                        {
                            DPF(0, "Hash collision (hash: %d)", pCurrentEntry->Hash);
                        }
                    }
                    pCurrentEntry2 = pCurrentEntry2->pNext;
                }

                pCurrentEntry = pCurrentEntry->pNext;
                ++ entries;
            }

            if (0 == entries)
            {
                ++ unusedSlots;
            }
            
            // mean must be greater than 0 at this point
            variance += (float)entries * (float)entries / mean;
        }

        variance /= std::max(1.0f, (m_NumHashSlots - 1));
        variance -= (mean * mean);

        DPF(0, "Mean number of entries per slot: %f, Standard deviation: %f, Unused slots; %d", mean, variance, unusedSlots);
    }
#endif // _DEBUG

    // S_OK if element is found, E_FAIL otherwise
    HRESULT FindValueWithHash(_In_ T Data, _In_ uint32_t Hash, _Out_ CIterator *pIterator)
    {
        assert(m_NumHashSlots > 0);

        uint32_t index = Hash % m_NumHashSlots;
        SHashEntry *pEntry = m_rgpHashEntries[index];
        while (nullptr != pEntry)
        {
            if (Hash == pEntry->Hash && pfnIsEqual(pEntry->Data, Data))
            {
                pIterator->ppHashSlot = m_rgpHashEntries + index;
                pIterator->pHashEntry = pEntry;
                return S_OK;
            }
            pEntry = pEntry->pNext;
        }
        return E_FAIL;
    }

    // S_OK if element is found, E_FAIL otherwise
    HRESULT FindFirstMatchingValue(_In_ uint32_t Hash, _Out_ CIterator *pIterator)
    {
        assert(m_NumHashSlots > 0);

        uint32_t index = Hash % m_NumHashSlots;
        SHashEntry *pEntry = m_rgpHashEntries[index];
        while (nullptr != pEntry)
        {
            if (Hash == pEntry->Hash)
            {
                pIterator->ppHashSlot = m_rgpHashEntries + index;
                pIterator->pHashEntry = pEntry;
                return S_OK;
            }
            pEntry = pEntry->pNext;
        }
        return E_FAIL;
    }

    // Adds data at the specified hash slot without checking for existence
    HRESULT AddValueWithHash(_In_ T Data, _In_ uint32_t Hash)
    {
        HRESULT hr = S_OK;

        assert(m_NumHashSlots > 0);

        SHashEntry *pHashEntry;
        uint32_t index = Hash % m_NumHashSlots;

        VN( pHashEntry = new SHashEntry );
        pHashEntry->pNext = m_rgpHashEntries[index];
        pHashEntry->Data = Data;
        pHashEntry->Hash = Hash;
        m_rgpHashEntries[index] = pHashEntry;

        ++ m_NumEntries;

lExit:
        return hr;
    }

    // Iterator code:
    //
    // CMyHashTable::CIterator myIt;
    // for (myTable.GetFirstEntry(&myIt); !myTable.PastEnd(&myIt); myTable.GetNextEntry(&myIt)
    // { myTable.GetData(&myIt); }
    void GetFirstEntry(_Out_ CIterator *pIterator)
    {
        SHashEntry **ppEnd = m_rgpHashEntries + m_NumHashSlots;
        pIterator->ppHashSlot = m_rgpHashEntries;
        while (pIterator->ppHashSlot < ppEnd)
        {
            if (nullptr != *(pIterator->ppHashSlot))
            {
                pIterator->pHashEntry = *(pIterator->ppHashSlot);
                return;
            }
            ++ pIterator->ppHashSlot;
        }
    }

    bool PastEnd(_Inout_ CIterator *pIterator)
    {
        SHashEntry **ppEnd = m_rgpHashEntries + m_NumHashSlots;
        assert(pIterator->ppHashSlot >= m_rgpHashEntries && pIterator->ppHashSlot <= ppEnd);
        return (pIterator->ppHashSlot == ppEnd);
    }

    void GetNextEntry(_Inout_ CIterator *pIterator)
    {
        SHashEntry **ppEnd = m_rgpHashEntries + m_NumHashSlots;
        assert(pIterator->ppHashSlot >= m_rgpHashEntries && pIterator->ppHashSlot <= ppEnd);
        assert(pIterator->pHashEntry != 0);
        _Analysis_assume_(pIterator->pHashEntry != 0);

        pIterator->pHashEntry = pIterator->pHashEntry->pNext;
        if (nullptr != pIterator->pHashEntry)
        {
            return;
        }

        ++ pIterator->ppHashSlot;
        while (pIterator->ppHashSlot < ppEnd)
        {
            pIterator->pHashEntry = *(pIterator->ppHashSlot);
            if (nullptr != pIterator->pHashEntry)
            {
                return;
            }
            ++ pIterator->ppHashSlot;
        }
        // hit the end of the list, ppHashSlot == ppEnd
    }

    void RemoveEntry(_Inout_ CIterator *pIterator)
    {
        SHashEntry *pTemp;
        SHashEntry **ppPrev;
        SHashEntry **ppEnd = m_rgpHashEntries + m_NumHashSlots;

        assert(pIterator && !PastEnd(pIterator));
        ppPrev = pIterator->ppHashSlot;
        pTemp = *ppPrev;
        while (pTemp)
        {
            if (pTemp == pIterator->pHashEntry)
            {
                *ppPrev = pTemp->pNext;
                pIterator->ppHashSlot = ppEnd;
                delete pTemp;
                return;
            }
            ppPrev = &pTemp->pNext;
            pTemp = pTemp->pNext;
        }

        // Should never get here
        assert(0);
    }

};

// Allocates the hash slots on the regular heap (since the
// hash table can grow), but all hash entries are allocated on
// a private heap

template<typename T, bool (*pfnIsEqual)(const T &Data1, const T &Data2)>
class CEffectHashTableWithPrivateHeap : public CEffectHashTable<T, pfnIsEqual>
{
protected:
    CDataBlockStore *m_pPrivateHeap;

public:
    CEffectHashTableWithPrivateHeap()
    {
        m_pPrivateHeap = nullptr;
    }

    void Cleanup()
    {
        CleanArray();
        m_NumHashSlots = 0;
        m_NumEntries = 0;
    }

    ~CEffectHashTableWithPrivateHeap()
    {
        Cleanup();
    }

    // Call this only once
    void SetPrivateHeap(_In_ CDataBlockStore *pPrivateHeap)
    {
        assert(nullptr == m_pPrivateHeap);
        m_pPrivateHeap = pPrivateHeap;
    }

    // Adds data at the specified hash slot without checking for existence
    HRESULT AddValueWithHash(_In_ T Data, _In_ uint32_t Hash)
    {
        HRESULT hr = S_OK;

        assert(m_pPrivateHeap);
        _Analysis_assume_(m_pPrivateHeap);
        assert(m_NumHashSlots > 0);

        SHashEntry *pHashEntry;
        uint32_t index = Hash % m_NumHashSlots;

        VN( pHashEntry = new(*m_pPrivateHeap) SHashEntry );
        pHashEntry->pNext = m_rgpHashEntries[index];
        pHashEntry->Data = Data;
        pHashEntry->Hash = Hash;
        m_rgpHashEntries[index] = pHashEntry;

        ++ m_NumEntries;

lExit:
        return hr;
    }
};
