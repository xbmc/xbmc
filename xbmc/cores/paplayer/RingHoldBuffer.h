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

#ifndef __RingHoldBuffer_h
#define __RingHoldBuffer_h
#include "utils/CriticalSection.h"
#include "utils/SingleLock.h"
#include "utils/log.h"
#include "utils/FastDelegate.h"
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

using namespace fastdelegate;

typedef FastDelegate<void()> EventHandler;


class CRingHoldBuffer
{
protected:
  ///////////////////////////////////////////////////////////////////
  // Protected Member Variables
  //
  char * m_pBuf;
  unsigned int m_nBufSize;       // the size of the ring buffer
  unsigned int m_nBehindSize;    // the size of the data to save behind the read pointer
  unsigned int m_iBehindAmount;  // amount of data behind the pointer
  unsigned int m_iAheadAmount;   // amount of data ahead of the pointer
  unsigned int m_iReadPtr;       // the read pointer

  CRITICAL_SECTION m_critSection;
public:
   EventHandler OnClear;

  ///////////////////////////////////////////////////////////////////
  // Constructor
  //
  CRingHoldBuffer()
  {
    InitializeCriticalSection(&m_critSection);
    m_pBuf = NULL;
    m_nBufSize = 0;
    m_nBehindSize = 0;
    m_iReadPtr = 0;
    m_iBehindAmount = 0;
    m_iAheadAmount = 0;
  }

  ///////////////////////////////////////////////////////////////////
  // Destructor
  //
  virtual ~CRingHoldBuffer()
  {
    Destroy();
    DeleteCriticalSection(&m_critSection);
  }

  ///////////////////////////////////////////////////////////////////
  // Method: Create
  // Purpose: Initializes the ring buffer for use.
  // Parameters:
  //     [in] iBufSize -- maximum size of the ring buffer
  //          iSaveSize -- maximum size to try and keep in the buffer behind the read pointer
  //
  // Return Value: TRUE if successful, otherwise FALSE.
  //
  BOOL Create( unsigned int iBufSize, unsigned int iSaveSize )
  {
    BOOL bResult = FALSE;

    ::EnterCriticalSection(&m_critSection );
    try
    {
  //    CLog::Log(LOGERROR, "RingBuffer@%x::Create", this);
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
      m_iAheadAmount = 0;
      m_iBehindAmount = 0;
      // sanity check the save size
      if (iSaveSize <= 1) iSaveSize = 1;
      if (iSaveSize >= m_nBufSize/2) iSaveSize = m_nBufSize/2;
      m_nBehindSize = iSaveSize; 
      ::LeaveCriticalSection(&m_critSection );
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "Exception in CRingHoldBuffer::Create().  Likely caused by allocating too much memory (%i bytes)", iBufSize);
      return false;
    }
    return bResult;
  }

  unsigned int Size()
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
//    CLog::Log(LOGERROR, "RingBuffer@%x::Destroy", this);
    if ( m_pBuf )
      delete [] m_pBuf;

    m_pBuf = NULL;
    m_nBufSize = 0;
    m_nBehindSize = 0;
    m_iBehindAmount = 0;
    m_iAheadAmount = 0;
    m_iReadPtr = 0;
    ::LeaveCriticalSection(&m_critSection );
  }

  void Clear()
  {
    ::EnterCriticalSection(&m_critSection );
    m_iBehindAmount = 0;
    m_iAheadAmount = 0;
    m_iReadPtr = 0;
    if (OnClear) OnClear();
    ::LeaveCriticalSection(&m_critSection );
  }
  ///////////////////////////////////////////////////////////////////
  // Method: GetMaxReadSize
  // Purpose: Returns the amount of data (in bytes) available for
  //     reading from the buffer.
  // Parameters: (None)
  // Return Value: Amount of data (in bytes) available for reading.
  //
  unsigned int GetMaxReadSize()
  {
    unsigned int iBytes = 0;
    ::EnterCriticalSection(&m_critSection );
    if ( m_pBuf )
    {
      iBytes = m_iAheadAmount;
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
  unsigned int GetMaxWriteSize()
  {
    unsigned int iBytes = 0;
    ::EnterCriticalSection(&m_critSection );
    if ( m_pBuf )
    {
      // we can fill up to the total buffer size minus the amount
      // we want to save behind the read pointer.
      if (m_nBufSize > m_nBehindSize + m_iAheadAmount)
        iBytes = m_nBufSize - m_nBehindSize - m_iAheadAmount;
      else
        iBytes = 0;
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
  BOOL WriteBinary( char * pBuf, unsigned int nBufLen )
  {
//    CLog::Log(LOGDEBUG, "RingHoldBuffer::WriteBinary %i bytes, ReadPos=%i, BehindAmount=%i, AheadAmount=%i", nBufLen, m_iReadPtr, m_iBehindAmount, m_iAheadAmount);
    ::EnterCriticalSection(&m_critSection );
    BOOL bResult = FALSE;
    {
      if ( nBufLen <= GetMaxWriteSize() )
      {
        // check if we're going to write over our saved data
        if (m_iBehindAmount + m_iAheadAmount + nBufLen > m_nBufSize)
          m_iBehindAmount = m_nBufSize - (m_iAheadAmount + nBufLen);
        // easy case, no wrapping
        unsigned int iWritePtr = m_iReadPtr + m_iAheadAmount;
        if (iWritePtr >= m_nBufSize)
          iWritePtr -= m_nBufSize;
        if ( iWritePtr + nBufLen < m_nBufSize )
        {
          CopyMemory( &m_pBuf[iWritePtr], pBuf, nBufLen );
        }
        else // harder case we need to wrap
        {
          unsigned int iFirstChunkSize = m_nBufSize - iWritePtr;
          unsigned int iSecondChunkSize = nBufLen - iFirstChunkSize;

          CopyMemory( &m_pBuf[iWritePtr], pBuf, iFirstChunkSize );
          CopyMemory( &m_pBuf[0], &pBuf[iFirstChunkSize], iSecondChunkSize );
        }
        m_iAheadAmount += nBufLen;
        bResult = TRUE;
      }
    }
    ::LeaveCriticalSection(&m_critSection );
//    CLog::Log(LOGDEBUG, "RingHoldBuffer::WriteBinary Done %i bytes, ReadPos=%i, BehindAmount=%i, AheadAmount=%i, return=%s", nBufLen, m_iReadPtr, m_iBehindAmount, m_iAheadAmount, bResult ? "true" : "false");
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
  BOOL ReadBinary( char * pBuf, unsigned int nBufLen )
  {
//    CLog::Log(LOGDEBUG, "RingHoldBuffer::ReadBinary %i bytes, ReadPos=%i, BehindAmount=%i, AheadAmount=%i", nBufLen, m_iReadPtr, m_iBehindAmount, m_iAheadAmount);
    ::EnterCriticalSection(&m_critSection );
    BOOL bResult = FALSE;
    {
      if ( nBufLen <= GetMaxReadSize() )
      {
        // easy case, no wrapping
        if ( m_iReadPtr + nBufLen < m_nBufSize )
        {
          CopyMemory( pBuf, &m_pBuf[m_iReadPtr], nBufLen );
          m_iReadPtr += nBufLen;
        }
        else // harder case, buffer wraps
        {
          unsigned int iFirstChunkSize = m_nBufSize - m_iReadPtr;
          unsigned int iSecondChunkSize = nBufLen - iFirstChunkSize;

          CopyMemory( pBuf, &m_pBuf[m_iReadPtr], iFirstChunkSize );
          CopyMemory( &pBuf[iFirstChunkSize], &m_pBuf[0], iSecondChunkSize );

          m_iReadPtr = iSecondChunkSize;
        }
        m_iBehindAmount += nBufLen;
        m_iAheadAmount -= nBufLen;
        bResult = TRUE;

      }
    }
    ::LeaveCriticalSection(&m_critSection );
//    CLog::Log(LOGDEBUG, "RingHoldBuffer::ReadBinary Done %i bytes, ReadPos=%i, BehindAmount=%i, AheadAmount=%i, return=%s", nBufLen, m_iReadPtr, m_iBehindAmount, m_iAheadAmount, bResult ? "true" : "false");
    return bResult;

  }

  BOOL PeakBinary( char * pBuf, unsigned int nBufLen )
  {
    ::EnterCriticalSection(&m_critSection );
    BOOL bResult = FALSE;
    int iPrevReadPtr = m_iReadPtr;
    {
      if ( nBufLen <= GetMaxReadSize() )
      {
        // easy case, no wrapping
        if ( m_iReadPtr + nBufLen < m_nBufSize )
        {
          CopyMemory( pBuf, &m_pBuf[m_iReadPtr], nBufLen );
          m_iReadPtr += nBufLen;
        }
        else // harder case, buffer wraps
        {
          unsigned int iFirstChunkSize = m_nBufSize - m_iReadPtr;
          unsigned int iSecondChunkSize = nBufLen - iFirstChunkSize;

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

  BOOL SkipBytes( int nBufLen ) // signed as we can go both ways
  {
//    CLog::Log(LOGDEBUG, "RingHoldBuffer::SkipBytes %i bytes, ReadPos=%i, BehindAmount=%i, AheadAmount=%i", nBufLen, m_iReadPtr, m_iBehindAmount, m_iAheadAmount);
    ::EnterCriticalSection(&m_critSection );
    BOOL bResult = FALSE;
    {
      // if we're going backwards, check we've got enough data behind us
      if ( nBufLen < 0 && m_iBehindAmount >= (unsigned int)(-nBufLen) )
      {
        if ((unsigned int)(-nBufLen) > m_iReadPtr) // wrap
          m_iReadPtr = m_nBufSize + m_iReadPtr + nBufLen;
        else
          m_iReadPtr -= -nBufLen;
        m_iAheadAmount -= nBufLen;
        m_iBehindAmount += nBufLen;
        bResult = TRUE;
      }
      else if ( 0 < nBufLen && (unsigned int)nBufLen < m_iAheadAmount )
      { // if we're going forwards...
        // easy case, no wrapping
        if ( m_iReadPtr + (unsigned int)nBufLen < m_nBufSize )
        {
          m_iReadPtr += nBufLen;
        }
        else // harder case, buffer wraps
        {
          unsigned int iFirstChunkSize = m_nBufSize - m_iReadPtr;
          unsigned int iSecondChunkSize = nBufLen - iFirstChunkSize;

          m_iReadPtr = iSecondChunkSize;
        }
        m_iAheadAmount -= nBufLen;
        m_iBehindAmount += nBufLen;
        bResult = TRUE;
      }
      else if (nBufLen == 0)
        bResult = TRUE;

    }
    ::LeaveCriticalSection(&m_critSection );
//    CLog::Log(LOGDEBUG, "RingHoldBuffer::SkipBytes Done %i bytes, ReadPos=%i, BehindAmount=%i, AheadAmount=%i, return=%s", nBufLen, m_iReadPtr, m_iBehindAmount, m_iAheadAmount, bResult ? "true" : "false");
    return bResult;
  }

  //Percentage filled, excluding the behindsize.
  int GetFillPercentage()
  {
    return (int)(100*(GetMaxReadSize()/(float)(m_nBufSize - m_nBehindSize)));
  }
};


#endif//__RingHoldBuffer_h

///////////////////////////////////////////////////////////////////////////////
// End of file
///////////////////////////////////////////////////////////////////////////////
