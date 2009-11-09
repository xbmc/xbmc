//------------------------------------------------------------------------------
// File: AsyncFlt.h
//
// Desc: DirectShow sample code - header file for async filter.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#include <strsafe.h>

// c553f2c0-1529-11d0-b4d1-00805f6cbbea
DEFINE_GUID(CLSID_AsyncSample,
0xc553f2c0, 0x1529, 0x11d0, 0xb4, 0xd1, 0x00, 0x80, 0x5f, 0x6c, 0xbb, 0xea);


//  NOTE:  This filter does NOT support AVI format

//
//  Define an internal filter that wraps the base CBaseReader stuff
//

class CMemStream : public CAsyncStream
{
public:
    CMemStream() :
        m_llPosition(0)
    {
    }

    /*  Initialization */
    void Init(LPBYTE pbData, LONGLONG llLength, DWORD dwKBPerSec = INFINITE)
    {
        m_pbData = pbData;
        m_llLength = llLength;
        m_dwKBPerSec = dwKBPerSec;
        m_dwTimeStart = timeGetTime();
    }

    HRESULT SetPointer(LONGLONG llPos)
    {
        if (llPos < 0 || llPos > m_llLength) {
            return S_FALSE;
        } else {
            m_llPosition = llPos;
            return S_OK;
        }
    }

    HRESULT Read(PBYTE pbBuffer,
                 DWORD dwBytesToRead,
                 BOOL bAlign,
                 LPDWORD pdwBytesRead)
    {
        CAutoLock lck(&m_csLock);
        DWORD dwReadLength;

        /*  Wait until the bytes are here! */
        DWORD dwTime = timeGetTime();

        if (m_llPosition + dwBytesToRead > m_llLength) {
            dwReadLength = (DWORD)(m_llLength - m_llPosition);
        } else {
            dwReadLength = dwBytesToRead;
        }
        DWORD dwTimeToArrive =
            ((DWORD)m_llPosition + dwReadLength) / m_dwKBPerSec;

        if (dwTime - m_dwTimeStart < dwTimeToArrive) {
            Sleep(dwTimeToArrive - dwTime + m_dwTimeStart);
        }

        CopyMemory((PVOID)pbBuffer, (PVOID)(m_pbData + m_llPosition),
                   dwReadLength);

        m_llPosition += dwReadLength;
        *pdwBytesRead = dwReadLength;
        return S_OK;
    }

    LONGLONG Size(LONGLONG *pSizeAvailable)
    {
        LONGLONG llCurrentAvailable =
            static_cast <LONGLONG> (UInt32x32To64((timeGetTime() - m_dwTimeStart),m_dwKBPerSec));
 
        *pSizeAvailable =  m_llLength < llCurrentAvailable ? m_llLength : llCurrentAvailable;
        return m_llLength;
    }

    DWORD Alignment()
    {
        return 1;
    }

    void Lock()
    {
        m_csLock.Lock();
    }

    void Unlock()
    {
        m_csLock.Unlock();
    }

private:
    CCritSec       m_csLock;
    PBYTE          m_pbData;
    LONGLONG       m_llLength;
    LONGLONG       m_llPosition;
    DWORD          m_dwKBPerSec;
    DWORD          m_dwTimeStart;
};

class CAsyncFilter : public CAsyncReader, public IFileSourceFilter
{
public:
    CAsyncFilter(LPUNKNOWN pUnk, HRESULT *phr) :
        CAsyncReader(NAME("Mem Reader"), pUnk, &m_Stream, phr),
        m_pFileName(NULL),
        m_pbData(NULL)
    {
    }

    ~CAsyncFilter()
    {
        delete [] m_pbData;
        delete [] m_pFileName;
    }

    static CUnknown * WINAPI CreateInstance(LPUNKNOWN, HRESULT *);

    DECLARE_IUNKNOWN

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv)
    {
        if (riid == IID_IFileSourceFilter) {
            return GetInterface((IFileSourceFilter *)this, ppv);
        } else {
            return CAsyncReader::NonDelegatingQueryInterface(riid, ppv);
        }
    }

    /*  IFileSourceFilter methods */

    //  Load a (new) file
    STDMETHODIMP Load(LPCOLESTR lpwszFileName, const AM_MEDIA_TYPE *pmt)
    {
        CheckPointer(lpwszFileName, E_POINTER);

        // lstrlenW is one of the few Unicode functions that works on win95
        int cch = lstrlenW(lpwszFileName) + 1;

#ifndef UNICODE
        TCHAR *lpszFileName=0;
        lpszFileName = new char[cch * 2];
        if (!lpszFileName) {
      	    return E_OUTOFMEMORY;
        }
        WideCharToMultiByte(GetACP(), 0, lpwszFileName, -1,
    			lpszFileName, cch, NULL, NULL);
#else
        TCHAR lpszFileName[MAX_PATH]={0};
        (void)StringCchCopy(lpszFileName, NUMELMS(lpszFileName), lpwszFileName);
#endif
        CAutoLock lck(&m_csFilter);

        /*  Check the file type */
        CMediaType cmt;
        if (NULL == pmt) {
            cmt.SetType(&MEDIATYPE_Stream);
            cmt.SetSubtype(&MEDIASUBTYPE_NULL);
        } else {
            cmt = *pmt;
        }

        if (!ReadTheFile(lpszFileName)) {
#ifndef UNICODE
            delete [] lpszFileName;
#endif
            return E_FAIL;
        }

        m_Stream.Init(m_pbData, m_llSize);

        m_pFileName = new WCHAR[cch];

        if (m_pFileName!=NULL)
    	    CopyMemory(m_pFileName, lpwszFileName, cch*sizeof(WCHAR));

        // this is not a simple assignment... pointers and format
        // block (if any) are intelligently copied
    	m_mt = cmt;

        /*  Work out file type */
        cmt.bTemporalCompression = TRUE;	       //???
        cmt.lSampleSize = 1;

        return S_OK;
    }

    // Modeled on IPersistFile::Load
    // Caller needs to CoTaskMemFree or equivalent.

    STDMETHODIMP GetCurFile(LPOLESTR * ppszFileName, AM_MEDIA_TYPE *pmt)
    {
        CheckPointer(ppszFileName, E_POINTER);
        *ppszFileName = NULL;

        if (m_pFileName!=NULL) {
        	DWORD n = sizeof(WCHAR)*(1+lstrlenW(m_pFileName));

            *ppszFileName = (LPOLESTR) CoTaskMemAlloc( n );
            if (*ppszFileName!=NULL) {
                  CopyMemory(*ppszFileName, m_pFileName, n);
            }
        }

        if (pmt!=NULL) {
            CopyMediaType(pmt, &m_mt);
        }

        return NOERROR;
    }

private:
    BOOL CAsyncFilter::ReadTheFile(LPCTSTR lpszFileName);

private:
    LPWSTR     m_pFileName;
    LONGLONG   m_llSize;
    PBYTE      m_pbData;
    CMemStream m_Stream;
};
