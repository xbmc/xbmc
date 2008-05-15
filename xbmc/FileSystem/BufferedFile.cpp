/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "stdafx.h"
#include "BufferedFile.h"
#include "StringUtils.h"

using namespace XFILE;

CBufferedFile::CBufferedFile(IFile* pFile, int iBufferSize) : IFile()
{
  Init(pFile, iBufferSize);
}

CBufferedFile::CBufferedFile(IFile* pFile) : IFile()
{
  Init(pFile, pFile->GetChunkSize());
}

CBufferedFile::~CBufferedFile()
{
  if (m_pFile)
  {
    m_pFile->Close();
    delete m_pFile;
    m_pFile = NULL;
  }
  
  if (m_buffer)
  {
    delete[] m_buffer;
    m_buffer      = NULL;
  }
}

bool CBufferedFile::Init(IFile* pFile, int iBufferSize)
{
  m_pFile           = pFile;
  m_iMaxBufferSize  = iBufferSize;
  m_iChunkSize      = pFile->GetChunkSize();
  
  // checks
  if (m_iMaxBufferSize <= 0)            m_iMaxBufferSize = pFile->GetChunkSize();
  if (m_pFile == NULL)                  CLog::Log(LOGFATAL, "CBufferedFile: NULL pointer given in constructor");
  if (m_iMaxBufferSize < m_iChunkSize)
  {
    CLog::Log(LOGWARNING, "CBufferedFile: incorrect buffer size, resetting");
    m_iMaxBufferSize = m_iChunkSize;
  }
  
  m_buffer = new BYTE[m_iMaxBufferSize];
  m_iBufferSize = 0;
  m_iBufferPos  = 0;
    
  return true;
}

bool CBufferedFile::Open(const CURL& url, bool bBinary)
{
  Close();
  
  m_iBufferSize = 0;
  m_iBufferPos  = 0;
  
  return m_pFile->Open(url, bBinary);
}

void CBufferedFile::Close()
{
  if (m_pFile)
  {
    m_pFile->Close();
  }
  
  m_iBufferSize = 0;
  m_iBufferPos  = 0;
}

/**
 * returns -1 on error, else nr of bytes read
 */
unsigned int CBufferedFile::ReadChunks(BYTE* buffer, int iSize)
{
  int iBytesRead = 0;
  
  while (iSize - iBytesRead >= m_iChunkSize)
  {
    int res = m_pFile->Read(buffer, m_iChunkSize);
    
    if (res >= 0)
    {
      buffer += res;
      iBytesRead += res;
      
      // check for eof
      if (res < m_iChunkSize) return iBytesRead;
    }
    else
    {
      CLog::Log(LOGFATAL, "CBufferedFile::ReadInChunks : error reading chunk, %d", m_iChunkSize);
      return res;
    }
  }
  
  return iBytesRead;
}

/** 
 * CBufferedFile::FillBuffer
 * Fills the cache buffer buffer with data
 * Only read's chunk sizes. This means that if the buffer is 10 bytes greater than chunk size,
 * those last 10 bytes will not be filled.
 *
 * Only call FillBuffer when the buffer is empty!!
 * returns the new buffer size.
 */
unsigned int CBufferedFile::FillBuffer()
{
  if (m_iBufferSize > 0)
  {
    CLog::Log(LOGWARNING, "CBufferedFile::FillBuffer, buffer is not empty, no reading is done");
    return m_iBufferSize;
  }
  
  BYTE* buf = m_buffer;
  m_iBufferPos = 0;
  
  int res = ReadChunks(m_buffer, m_iMaxBufferSize);
  if (res > 0) m_iBufferSize = res;
  
  return m_iBufferSize;
}

/**
 * returns nr of bytes read
 */
unsigned int CBufferedFile::ReadFromBuffer(BYTE* pBuffer, int iSize)
{
  if (iSize > m_iBufferSize) iSize = m_iBufferSize;
  if (iSize > 0)
  {
    fast_memcpy(pBuffer, m_buffer + m_iBufferPos, iSize);

    m_iBufferPos  += iSize;
    m_iBufferSize -= iSize;
  }
  return iSize;
}

unsigned int CBufferedFile::Read(void* lpBuf, __int64 uiBufSize)
{
  int iBytesCopied = 0;
  int iSize = (int)uiBufSize;
  
  // copy contents from buffer
  if (m_iBufferSize > 0)
  {
    iBytesCopied += ReadFromBuffer((BYTE*)lpBuf, iSize);
  
    // if we copied enough data in lpBuf, we are done
    if (iBytesCopied == iSize) return iBytesCopied;
  }
  
  // if we are here, the buffer should be empty
  assert(m_iBufferSize == 0);
  
  // if we need to read more bytes than our buffer is big, do it directly
  // but in chunks.
  int res = ReadChunks((BYTE*)lpBuf + iBytesCopied, iSize - iBytesCopied);
  if (res < 0)
  {
    // error, if bytes are read, return those, else the error.
    if (iBytesCopied > 0) return iBytesCopied;
    else return res;
  }
  iBytesCopied += res;
  
  if (iBytesCopied == iSize) return iBytesCopied;

  // some more data is needed, copy as much data into the buffer as possible
  // so that it can be used the next read
  res = FillBuffer();
  if (res > 0)
  {
    // we have some data, copy it
    iBytesCopied += ReadFromBuffer((BYTE*)lpBuf + iBytesCopied, iSize - iBytesCopied);
  }
  return iBytesCopied;
}

void CBufferedFile::Flush()
{
  m_pFile->Flush();
  
  m_iBufferSize = 0;
  m_iBufferPos  = 0;
}

__int64 CBufferedFile::GetPosition()
{
  __int64 pos = m_pFile->GetPosition();
  pos -= m_iBufferSize;
  
  if (pos < 0)
  {
    CLog::Log(LOGFATAL, "CBufferedFile::GetPosition : error");
    pos = -1;
  }
  return pos;
}

__int64 CBufferedFile::Seek(__int64 iFilePosition, int iWhence)
{
  __int64 res = 0LL;
  
  if (iWhence == SEEK_END)
  {
    res = m_pFile->Seek(iFilePosition, iWhence);
  }
  else if (iWhence == SEEK_SET)
  {
    res = m_pFile->Seek(iFilePosition, iWhence);
  }
  else if (iWhence == SEEK_CUR)
  {
    // determine the current file position
    __int64 pos = iFilePosition - m_iBufferSize;
    if (pos < 0)
    {
      CLog::Log(LOGFATAL, "CBufferedFile::Seek : seek error");
      res = -1;
    }
    else res = m_pFile->Seek(pos, iWhence);
  }

  if (res >= 0)
  {
    // seek succesfull, empty buffer
    m_iBufferSize = 0;
    m_iBufferPos  = 0;
  }
  return res;
}

bool CBufferedFile::ReadString(char *szLine, int iLineLength)
{
  BYTE* pBuffer = (BYTE*)szLine;
  szLine[0] = '\0';
  
  int iParsed = 0;
  
  while (iParsed < iLineLength)
  {
    // data is neded, get it
    if (m_iBufferSize <= 0)
    {
      int res = FillBuffer();
      if (res <= 0)
      {
        if (szLine[0] != '\0') return true; // eof
        return false;
      }
    }
    
    // copy as much data as we can in szLine
    int iSize = (iLineLength - 1) - iParsed; // -1 for '\0'
    if (iSize > m_iBufferSize) iSize = m_iBufferSize;
    
    fast_memcpy(szLine + iParsed, m_buffer + m_iBufferPos, iSize);
    szLine[iSize] = '\0';
    
    // start looking for eol
    // the last char is alway's '\0', which means in the case of
    // '\n' or '\r' we can look 1 position further in the buffer
    for (int i = 0; i < iSize; i++)
    {
      if (szLine[i] == '\n')
      {
        m_iBufferSize -= (i + 1);
        m_iBufferPos += (i + 1);
        if (szLine[i + 1] == '\r')
        {
          m_iBufferSize--;
          m_iBufferPos++;
        }

        szLine[i + 1] = '\0';
        return true;
      }
      else if (szLine[i] == '\r')
      {
        szLine[i] = '\n';
        m_iBufferSize -= (i + 1);
        m_iBufferPos += (i + 1);
        if (szLine[i + 1] == '\n')
        {
          m_iBufferSize--;
          m_iBufferPos++;
        }

        szLine[i + 1] = '\0';
        return true;
      }
    }

    iParsed += iSize;
    m_iBufferSize -= iSize;
    m_iBufferPos  += iSize;
    // no eol found, try again
  }
  
  if (szLine[0] != '\0') return true; // eof
  return false;
}
