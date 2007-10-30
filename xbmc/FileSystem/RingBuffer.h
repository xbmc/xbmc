//////////////////////////////////////////////////////////////////////
// RingBuffer.h: Interface and implementation of CRingBuffer.
//////////////////////////////////////////////////////////////////////
//
// CRingBuffer - An MFC ring buffer class.
//
// Purpose:
//     This class was designed to be a fast way of collecting incoming
//     data from a socket, then retreiving it one text line at a time.
//     It intentionally does no dynamic buffer allocation/deallocation
//     during runtime, so as to avoid memory fragmentation and other
//     issues. It is currently used in POP3 and SMTP servers created
//     by Stardust Software.
//
// Author:
//     Written by Larry Antram (larrya@sdust.com)
//     Copyright © 2001-2002 Stardust Software. All Rights Reserved.
//     http://www.stardustsoftware.com
//
// Legal Notice:
//     This code may be used in compiled form in any way you desire
//     and may be redistributed unmodified by any means PROVIDING it
//     is not sold for profit without the authors written consent, and
//     providing that this entire notice and the authors name and all
//     copyright notices remains intact.
//
//     This file is provided "as is" with no expressed or implied
//     warranty. The author accepts no liability for any damage/loss
//     of business that this product may cause.
//
// History:
//     Dec 23 2001 - Initial creation.
//     May 15 2002 - Posted to CodeProject.com.
//
// Sample Usage:
//
//     //
//     // Initialization
//     //
//
//     using namespace Stardust;
//
//     #define INCOMING_RING_BUFFER_SIZE (1024*16) // or whatever
//
//     CRingBuffer m_ringbuf;
//     m_ringbuf.Create( INCOMING_RING_BUFFER_SIZE + 1 );
//
//     char m_chInBuf[ INCOMING_BUFFER_SIZE + 1 ];
//
//     ...
//
//     //
//     // Then later, upon receiving data...
//     //
//
//     int iNumberOfBytesRead = 0;
//
//     while( READ_INCOMING(m_chInBuf,INCOMING_BUFFER_SIZE,&iNumberOfBytesRead,0)
//     && iNumberOfBytesRead > 0 )
//     {
//          // add incoming data to the ring buffer
//          m_ringbuf.WriteBinary(m_chInBuf,iNumberOfBytesRead);
//
//          // pull it back out one line at a time, and distribute it
//          CString strLine;
//          while( m_ringbuf.ReadTextLine(strLine) )
//          {
//              strLine.TrimRight("\r\n");
//
//              if( !ON_INCOMING( strLine ) )
//                  return FALSE;
//          }
//     }
//
//     // Fall out, until more incoming data is available...
//
// Notes:
//     In the above example, READ_INCOMING and ON_INCOMING are
//     placeholders for functions, that read and accept incoming data,
//     respectively.
//
//     READ_INCOMING receives raw data from the socket.
//
//     ON_INCOMING accepts incoming data from the socket, one line at
//     a time.
//
//////////////////////////////////////////////////////////////////////

#ifndef __RingBuffer_h
#define __RingBuffer_h
#include "../utils/CriticalSection.h"
#include "../utils/SingleLock.h"
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CRingBuffer
{
protected:
  ///////////////////////////////////////////////////////////////////
  // Protected Member Variables
  //
  char * m_pBuf;
  int m_nBufSize;   // the size of the ring buffer
  int m_iReadPtr;   // the read pointer
  int m_iWritePtr;  // the write pointer

  CRITICAL_SECTION m_critSection;
public:
  ///////////////////////////////////////////////////////////////////
  // Constructor
  //
  CRingBuffer()
  {
    InitializeCriticalSection(&m_critSection);
    m_pBuf = NULL;
    //m_pTmpBuf = NULL;
    m_nBufSize = 0;
    m_iReadPtr = 0;
    m_iWritePtr = 0;
  }

  ///////////////////////////////////////////////////////////////////
  // Destructor
  //
  virtual ~CRingBuffer()
  {
    Destroy();
  }

  ///////////////////////////////////////////////////////////////////
  // Method: Create
  // Purpose: Initializes the ring buffer for use.
  // Parameters:
  //     [in] iBufSize -- maximum size of the ring buffer
  // Return Value: TRUE if successful, otherwise FALSE.
  //
  BOOL Create( int iBufSize )
  {
    BOOL bResult = FALSE;

    ::EnterCriticalSection(&m_critSection );
    if ( m_pBuf )
      delete [] m_pBuf;

    m_pBuf = NULL;

    m_pBuf = new char[ iBufSize ];
    if ( m_pBuf )
    {
      m_nBufSize = iBufSize;
      ZeroMemory( m_pBuf, m_nBufSize );

      bResult = TRUE;
    }
    m_iReadPtr = 0;
    m_iWritePtr = 0;
    ::LeaveCriticalSection(&m_critSection );
    return bResult;
  }

  int Size()
  {
    return m_nBufSize;
  }

  ///////////////////////////////////////////////////////////////////
  // Method: Destroy
  // Purpose: Cleans up ring buffer by freeing memory and resetting
  //     member variables to original state.
  // Parameters: (None)
  // Return Value: (None)
  //
  void Destroy()
  {
    ::EnterCriticalSection(&m_critSection );
    if ( m_pBuf )
      delete [] m_pBuf;

    m_pBuf = NULL;
    m_nBufSize = 0;
    m_iReadPtr = 0;
    m_iWritePtr = 0;
    ::LeaveCriticalSection(&m_critSection );
    DeleteCriticalSection(&m_critSection);
  }

  void Clear()
  {
    ::EnterCriticalSection(&m_critSection );
    m_iReadPtr = 0;
    m_iWritePtr = 0;
    ::LeaveCriticalSection(&m_critSection );
  }
  ///////////////////////////////////////////////////////////////////
  // Method: GetMaxReadSize
  // Purpose: Returns the amount of data (in bytes) available for
  //     reading from the buffer.
  // Parameters: (None)
  // Return Value: Amount of data (in bytes) available for reading.
  //
  int GetMaxReadSize()
  {
    int iBytes = 0;
    ::EnterCriticalSection(&m_critSection );
    if ( m_pBuf )
    {
      if ( m_iReadPtr == m_iWritePtr )
      {
        iBytes = 0;
      }

      else if ( m_iReadPtr < m_iWritePtr )
        iBytes = m_iWritePtr - m_iReadPtr;

      else if ( m_iReadPtr > m_iWritePtr )
        iBytes = (m_nBufSize - m_iReadPtr) + m_iWritePtr;
    }
    ::LeaveCriticalSection(&m_critSection );
    return iBytes;
  }

  ///////////////////////////////////////////////////////////////////
  // Method: GetMaxWriteSize
  // Purpose: Returns the amount of space (in bytes) available for
  //     writing into the buffer.
  // Parameters: (None)
  // Return Value: Amount of space (in bytes) available for writing.
  //
  int GetMaxWriteSize()
  {
    int iBytes = 0;
    ::EnterCriticalSection(&m_critSection );
    if ( m_pBuf )
    {
      if ( m_iReadPtr == m_iWritePtr )
        iBytes = m_nBufSize;

      if ( m_iWritePtr < m_iReadPtr )
        iBytes = m_iReadPtr - m_iWritePtr;

      if ( m_iWritePtr > m_iReadPtr )
        iBytes = (m_nBufSize - m_iWritePtr) + m_iReadPtr;
    }
    ::LeaveCriticalSection(&m_critSection );
    return iBytes;
  }

  ///////////////////////////////////////////////////////////////////
  // Method: WriteBinary
  // Purpose: Writes binary data into the ring buffer.
  // Parameters:
  //     [in] pBuf - Pointer to the data to write.
  //     [in] nBufLen - Size of the data to write (in bytes).
  // Return Value: TRUE upon success, otherwise FALSE.
  //
  BOOL WriteBinary( char * pBuf, int nBufLen )
  {
    ::EnterCriticalSection(&m_critSection );
    BOOL bResult = FALSE;
    {
      if ( nBufLen < GetMaxWriteSize() )
      {
        // easy case, no wrapping
        if ( m_iWritePtr + nBufLen < m_nBufSize )
        {
          CopyMemory( &m_pBuf[m_iWritePtr], pBuf, nBufLen );
          m_iWritePtr += nBufLen;
        }
        else // harder case we need to wrap
        {
          int iFirstChunkSize = m_nBufSize - m_iWritePtr;
          int iSecondChunkSize = nBufLen - iFirstChunkSize;

          CopyMemory( &m_pBuf[m_iWritePtr], pBuf, iFirstChunkSize );
          CopyMemory( &m_pBuf[0], &pBuf[iFirstChunkSize], iSecondChunkSize );

          m_iWritePtr = iSecondChunkSize;
        }
        bResult = TRUE;
      }
    }
    ::LeaveCriticalSection(&m_critSection );
    return bResult;
  }

  ///////////////////////////////////////////////////////////////////
  // Method: ReadBinary
  // Purpose: Reads (and extracts) data from the ring buffer.
  // Parameters:
  //     [in/out] pBuf - Pointer to where read data will be stored.
  //     [in] nBufLen - Size of the data to be read (in bytes).
  // Return Value: TRUE upon success, otherwise FALSE.
  //
  BOOL ReadBinary( char * pBuf, int nBufLen )
  {
    ::EnterCriticalSection(&m_critSection );
    BOOL bResult = FALSE;
    {
      if ( nBufLen <= GetMaxReadSize() )
      {
        // easy case, no wrapping
        if ( m_iReadPtr + nBufLen <= m_nBufSize )
        {
          CopyMemory( pBuf, &m_pBuf[m_iReadPtr], nBufLen );
          m_iReadPtr += nBufLen;
        }
        else // harder case, buffer wraps
        {
          int iFirstChunkSize = m_nBufSize - m_iReadPtr;
          int iSecondChunkSize = nBufLen - iFirstChunkSize;

          CopyMemory( pBuf, &m_pBuf[m_iReadPtr], iFirstChunkSize );
          CopyMemory( &pBuf[iFirstChunkSize], &m_pBuf[0], iSecondChunkSize );

          m_iReadPtr = iSecondChunkSize;
        }
        bResult = TRUE;
      }
    }
    ::LeaveCriticalSection(&m_critSection );
    return bResult;
  }

  BOOL PeakBinary( char * pBuf, int nBufLen )
  {
    ::EnterCriticalSection(&m_critSection );
    BOOL bResult = FALSE;
    int iPrevReadPtr = m_iReadPtr;
    {
      if ( nBufLen <= GetMaxReadSize() )
      {
        // easy case, no wrapping
        if ( m_iReadPtr + nBufLen <= m_nBufSize )
        {
          CopyMemory( pBuf, &m_pBuf[m_iReadPtr], nBufLen );
          m_iReadPtr += nBufLen;
        }
        else // harder case, buffer wraps
        {
          int iFirstChunkSize = m_nBufSize - m_iReadPtr;
          int iSecondChunkSize = nBufLen - iFirstChunkSize;

          CopyMemory( pBuf, &m_pBuf[m_iReadPtr], iFirstChunkSize );
          CopyMemory( &pBuf[iFirstChunkSize], &m_pBuf[0], iSecondChunkSize );

          m_iReadPtr = iSecondChunkSize;
        }
        bResult = TRUE;
      }
    }
    m_iReadPtr = iPrevReadPtr;
    ::LeaveCriticalSection(&m_critSection );
    return bResult;
  }

  BOOL SkipBytes( int nBufLen )
  {
    ::EnterCriticalSection(&m_critSection );
    BOOL bResult = FALSE;
    {
      if ( nBufLen <= GetMaxReadSize() )
      {
        // easy case, no wrapping
        if ( m_iReadPtr + nBufLen <= m_nBufSize )
        {
          m_iReadPtr += nBufLen;
        }
        else // harder case, buffer wraps
        {
          int iFirstChunkSize = m_nBufSize - m_iReadPtr;
          int iSecondChunkSize = nBufLen - iFirstChunkSize;

          m_iReadPtr = iSecondChunkSize;
        }
        bResult = TRUE;
      }
    }
    ::LeaveCriticalSection(&m_critSection );
    return bResult;
  }
};


#endif//__RingBuffer_h

///////////////////////////////////////////////////////////////////////////////
// End of file
///////////////////////////////////////////////////////////////////////////////
