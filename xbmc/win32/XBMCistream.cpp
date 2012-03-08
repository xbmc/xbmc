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
#include "XBMCistream.h"


XBMCistream::XBMCistream() :
  m_cRef( 1 )
{
}

XBMCistream::~XBMCistream()
{
  m_file.Close();
}

HRESULT STDMETHODCALLTYPE XBMCistream::QueryInterface( REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject ) 
{
  if( ( IID_IStream == riid ) || ( IID_IUnknown == riid ) )
  {
    *ppvObject = static_cast< IStream* >( this );
    AddRef();
    return( S_OK );
  }

  *ppvObject = NULL;
  return( E_NOINTERFACE );
}

ULONG STDMETHODCALLTYPE XBMCistream::AddRef()
{
  return( InterlockedIncrement( &m_cRef ) );
}

ULONG STDMETHODCALLTYPE XBMCistream::Release()
{
  if( 0 == InterlockedDecrement( &m_cRef ) )
  {
    delete this;
    return( 0 );
  }

  return( m_cRef );
}


HRESULT XBMCistream::Open( LPCTSTR ptszURL )
{
  if (!m_file.Open(ptszURL))
    return E_FAIL;

  return S_OK;
}


HRESULT XBMCistream::Read( void *pv, ULONG cb, ULONG *pcbRead )
{

  *pcbRead = (ULONG) m_file.Read(pv, cb);

  if(pcbRead <= 0)
    return E_FAIL;

  return S_OK;
}

HRESULT XBMCistream::Seek( LARGE_INTEGER dlibMove,
                         DWORD dwOrigin,
                         ULARGE_INTEGER *plibNewPosition )
{
  int iWhence = 0;

  switch( dwOrigin )
  {
      case STREAM_SEEK_SET:
          iWhence = SEEK_SET;
          break;

      case STREAM_SEEK_CUR:
          iWhence = SEEK_CUR;
          break;

      case STREAM_SEEK_END:
          iWhence = SEEK_END;
          break;

      default:
          return( E_INVALIDARG );
  };

  int64_t ipos = m_file.Seek(dlibMove.QuadPart, iWhence);

  if(ipos < 0)
    return E_FAIL;

  if( NULL != plibNewPosition )
    plibNewPosition->QuadPart = ipos;

  return S_OK;
}

HRESULT XBMCistream::Stat( STATSTG *pstatstg, DWORD grfStatFlag )
{
  if( ( NULL == pstatstg ) || ( STATFLAG_NONAME != grfStatFlag ) )
  {
    return( E_INVALIDARG );
  }

  struct __stat64 buffer;

  if(m_file.Stat(&buffer) == -1)
    return E_FAIL;

  memset( pstatstg, 0, sizeof( STATSTG ) );

  pstatstg->type = STGTY_STREAM;
  pstatstg->cbSize.QuadPart = buffer.st_size;

  return S_OK;
}