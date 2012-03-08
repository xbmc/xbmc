#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */


#include <objidl.h>
#include "filesystem/File.h"

class XBMCistream : public IStream
{
public:

    XBMCistream();
    ~XBMCistream();

    //
    // IUnknown methods
    //
    HRESULT STDMETHODCALLTYPE QueryInterface( /* [in] */  REFIID riid,
                                              /* [out] */ void **ppvObject );
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    //
    // Methods of IStream
    //
    HRESULT STDMETHODCALLTYPE Read( void *pv, ULONG cb, ULONG *pcbRead );
    HRESULT STDMETHODCALLTYPE Seek( LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition );
    HRESULT STDMETHODCALLTYPE Stat( STATSTG *pstatstg, DWORD grfStatFlag );

    //
    // Non-implemented methods of IStream
    //
    HRESULT STDMETHODCALLTYPE Write( void const *pv, ULONG cb, ULONG *pcbWritten )
    {
        return( E_NOTIMPL );
    }
    HRESULT STDMETHODCALLTYPE SetSize( ULARGE_INTEGER libNewSize )
    {
        return( E_NOTIMPL );
    }
    HRESULT STDMETHODCALLTYPE CopyTo( IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten )
    {
        return( E_NOTIMPL );
    }
    HRESULT STDMETHODCALLTYPE Commit( DWORD grfCommitFlags )
    {
        return( E_NOTIMPL );
    }
    HRESULT STDMETHODCALLTYPE Revert()
    {
        return( E_NOTIMPL );
    }
    HRESULT STDMETHODCALLTYPE LockRegion( ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType )
    {
        return( E_NOTIMPL );
    }
    HRESULT STDMETHODCALLTYPE UnlockRegion( ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType )
    {
        return( E_NOTIMPL );
    }
    HRESULT STDMETHODCALLTYPE Clone( IStream **ppstm )
    {
        return( E_NOTIMPL );
    }

    //
    // CROStream method
    //
    HRESULT Open( /* [in] */ LPCTSTR ptszURL );

protected:

    XFILE::CFile m_file;
    LONG    m_cRef;
};