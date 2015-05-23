/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "ZipFile.h"
#include "URL.h"
#include "utils/URIUtils.h"
#include "utils/auto_buffer.h"
#include "utils/log.h"

#include <sys/stat.h>

#define ZIP_CACHE_LIMIT 4*1024*1024

using namespace XFILE;
using namespace std;

CZipFile::CZipFile()
{
  m_szStringBuffer = NULL;
  m_szStartOfStringBuffer = NULL;
  m_iDataInStringBuffer = 0;
  m_bCached = false;
  m_iRead = -1;
}

CZipFile::~CZipFile()
{
  delete[] m_szStringBuffer;
  Close();
}

bool CZipFile::Open(const CURL&url)
{
  std::string strOpts = url.GetOptions();
  CURL url2(url);
  url2.SetOptions("");
  if (!g_ZipManager.GetZipEntry(url2,mZipItem))
    return false;

  if ((mZipItem.flags & 64) == 64)
  {
    CLog::Log(LOGERROR,"FileZip: encrypted file, not supported!");
    return false;
  }

  if ((mZipItem.method != 8) && (mZipItem.method != 0))
  {
    CLog::Log(LOGERROR,"FileZip: unsupported compression method!");
    return false;
  }

  if (mZipItem.method != 0 && mZipItem.usize > ZIP_CACHE_LIMIT && strOpts != "?cache=no")
  {
    if (!CFile::Exists("special://temp/" + URIUtils::GetFileName(url2)))
    {
      url2.SetOptions("?cache=no");
      const CURL pathToUrl("special://temp/" + URIUtils::GetFileName(url2));
      if (!CFile::Copy(url2, pathToUrl))
        return false;
    }
    m_bCached = true;
    return mFile.Open("special://temp/" + URIUtils::GetFileName(url2));
  }

  if (!mFile.Open(url.GetHostName())) // this is the zip-file, always open binary
  {
    CLog::Log(LOGERROR,"FileZip: unable to open zip file %s!",url.GetHostName().c_str());
    return false;
  }
  mFile.Seek(mZipItem.offset,SEEK_SET);
  return InitDecompress();
}

bool CZipFile::InitDecompress()
{
  m_iRead = 1;
  m_iFilePos = 0;
  m_iZipFilePos = 0;
  m_iAvailBuffer = 0;
  m_bFlush = false;
  m_ZStream.zalloc = Z_NULL;
  m_ZStream.zfree = Z_NULL;
  m_ZStream.opaque = Z_NULL;
  if( mZipItem.method != 0 )
  {
    if (inflateInit2(&m_ZStream,-MAX_WBITS) != Z_OK)
    {
      CLog::Log(LOGERROR,"FileZip: error initializing zlib!");
      return false;
    }
  }
  m_ZStream.next_in = (Bytef*)m_szBuffer;
  m_ZStream.avail_in = 0;
  m_ZStream.total_out = 0;

  return true;
}

int64_t CZipFile::GetLength()
{
  return mZipItem.usize;
}

int64_t CZipFile::GetPosition()
{
  if (m_bCached)
    return mFile.GetPosition();

  return m_iFilePos;
}

int64_t CZipFile::Seek(int64_t iFilePosition, int iWhence)
{
  if (m_bCached)
    return mFile.Seek(iFilePosition,iWhence);
  if (mZipItem.method == 0) // this is easy
  {
    int64_t iResult;
    switch (iWhence)
    {
    case SEEK_SET:
      if (iFilePosition > mZipItem.usize)
        return -1;
      m_iFilePos = iFilePosition;
      m_iZipFilePos = m_iFilePos;
      iResult = mFile.Seek(iFilePosition+mZipItem.offset,SEEK_SET)-mZipItem.offset;
      return iResult;
      break;

    case SEEK_CUR:
      if (m_iFilePos+iFilePosition > mZipItem.usize)
        return -1;
      m_iFilePos += iFilePosition;
      m_iZipFilePos = m_iFilePos;
      iResult = mFile.Seek(iFilePosition,SEEK_CUR)-mZipItem.offset;
      return iResult;
      break;

    case SEEK_END:
      if (iFilePosition > mZipItem.usize)
        return -1;
      m_iFilePos = mZipItem.usize+iFilePosition;
      m_iZipFilePos = m_iFilePos;
      iResult = mFile.Seek(mZipItem.offset+mZipItem.usize+iFilePosition,SEEK_SET)-mZipItem.offset;
      return iResult;
      break;
    default:
      return -1;

    }
  }
  // here goes the stupid part..
  if (mZipItem.method == 8)
  {
    static const int blockSize = 128 * 1024;
    XUTILS::auto_buffer buf(blockSize);
    switch (iWhence)
    {
    case SEEK_SET:
      if (iFilePosition == m_iFilePos)
        return m_iFilePos; // mp3reader does this lots-of-times
      if (iFilePosition > mZipItem.usize || iFilePosition < 0)
        return -1;
      // read until position in 128k blocks.. only way to do it due to format.
      // can't start in the middle of data since then we'd have no clue where
      // we are in uncompressed data..
      if (iFilePosition < m_iFilePos)
      {
        m_iFilePos = 0;
        m_iZipFilePos = 0;
        inflateEnd(&m_ZStream);
        inflateInit2(&m_ZStream,-MAX_WBITS); // simply restart zlib
        mFile.Seek(mZipItem.offset,SEEK_SET);
        m_ZStream.next_in = (Bytef*)m_szBuffer;
        m_ZStream.avail_in = 0;
        m_ZStream.total_out = 0;
        while (m_iFilePos < iFilePosition)
        {
          unsigned int iToRead = (iFilePosition - m_iFilePos)>blockSize ? blockSize : (int)(iFilePosition - m_iFilePos);
          if (Read(buf.get(),iToRead) != iToRead)
            return -1;
        }
        return m_iFilePos;
      }
      else // seek forward
        return Seek(iFilePosition-m_iFilePos,SEEK_CUR);
      break;

    case SEEK_CUR:
      if (iFilePosition < 0)
        return Seek(m_iFilePos+iFilePosition,SEEK_SET); // can't rewind stream
      // read until requested position, drop data
      if (m_iFilePos+iFilePosition > mZipItem.usize)
        return -1;
      iFilePosition += m_iFilePos;
      while (m_iFilePos < iFilePosition)
      {
        unsigned int iToRead = (iFilePosition - m_iFilePos)>blockSize ? blockSize : (int)(iFilePosition - m_iFilePos);
        if (Read(buf.get(), iToRead) != iToRead)
          return -1;
      }
      return m_iFilePos;
      break;

    case SEEK_END:
      // now this is a nasty bastard, possibly takes lotsoftime
      // uncompress, minding m_ZStream.total_out

      while( (int)m_ZStream.total_out < mZipItem.usize+iFilePosition)
      {
        unsigned int iToRead = (mZipItem.usize + iFilePosition - m_ZStream.total_out > blockSize) ? blockSize : (int)(mZipItem.usize + iFilePosition - m_ZStream.total_out);
        if (Read(buf.get(), iToRead) != iToRead)
          return -1;
      }
      return m_iFilePos;
      break;
    default:
      return -1;
    }
  }
  return -1;
}

bool CZipFile::Exists(const CURL& url)
{
  SZipEntry item;
  if (g_ZipManager.GetZipEntry(url,item))
    return true;
  return false;
}

int CZipFile::Stat(struct __stat64 *buffer)
{
  int ret;
  struct tm tm = {};

  ret = mFile.Stat(buffer);
  tm.tm_sec = (mZipItem.mod_time & 0x1F) << 1;
  tm.tm_min = (mZipItem.mod_time & 0x7E0) >> 5;
  tm.tm_hour = (mZipItem.mod_time & 0xF800) >> 11;
  tm.tm_mday = (mZipItem.mod_date & 0x1F);
  tm.tm_mon = (mZipItem.mod_date & 0x1E0) >> 5;
  tm.tm_year = (mZipItem.mod_date & 0xFE00) >> 9;
  buffer->st_atime = buffer->st_ctime = buffer->st_mtime = mktime(&tm);

  buffer->st_size = mZipItem.usize;
  buffer->st_dev = (buffer->st_dev << 16) ^ (buffer->st_ino << 16);
  buffer->st_ino ^= mZipItem.crc32;
  return ret;
}

int CZipFile::Stat(const CURL& url, struct __stat64* buffer)
{
  if (!g_ZipManager.GetZipEntry(url, mZipItem))
  {
    if (url.GetFileName().empty() && CFile::Exists(url.GetHostName()))
    { // when accessing the zip "root" recognize it as a directory
      buffer->st_mode = _S_IFDIR;
      return 0;
    }
    else
      return -1;
  }

  memset(buffer, 0, sizeof(struct __stat64));
  buffer->st_gid = 0;
  buffer->st_atime = buffer->st_ctime = mZipItem.mod_time;
  buffer->st_size = mZipItem.usize;
  return 0;
}

ssize_t CZipFile::Read(void* lpBuf, size_t uiBufSize)
{
  if (uiBufSize > SSIZE_MAX)
    uiBufSize = SSIZE_MAX;

  if (m_bCached)
    return mFile.Read(lpBuf,uiBufSize);

  // flush what might be left in the string buffer
  if (m_iDataInStringBuffer > 0)
  {
    size_t iMax = static_cast<size_t>((uiBufSize>m_iDataInStringBuffer?m_iDataInStringBuffer:uiBufSize));
    memcpy(lpBuf,m_szStartOfStringBuffer,iMax);
    uiBufSize -= iMax;
    m_iDataInStringBuffer -= iMax;
  }
  if (mZipItem.method == 8) // deflated
  {
    uLong iDecompressed = 0;
    uLong prevOut = m_ZStream.total_out;
    while ((static_cast<size_t>(iDecompressed) < uiBufSize) && ((m_iZipFilePos < mZipItem.csize) || (m_bFlush)))
    {
      m_ZStream.next_out = (Bytef*)(lpBuf)+iDecompressed;
      m_ZStream.avail_out = static_cast<uInt>(uiBufSize-iDecompressed);
      if (m_bFlush) // need to flush buffer !
      {
        int iMessage = inflate(&m_ZStream,Z_SYNC_FLUSH);
        m_bFlush = ((iMessage == Z_OK) && (m_ZStream.avail_out == 0))?true:false;
        if (!m_ZStream.avail_out) // flush filled buffer, get out of here
        {
          iDecompressed = m_ZStream.total_out-prevOut;
          break;
        }
      }

      if (!m_ZStream.avail_in)
      {
        if (!FillBuffer()) // eof!
        {
          iDecompressed = m_ZStream.total_out-prevOut;
          break;
        }
      }

      int iMessage = inflate(&m_ZStream,Z_SYNC_FLUSH);
      if (iMessage < 0)
      {
        Close();
        return -1; // READ ERROR
      }

      m_bFlush = ((iMessage == Z_OK) && (m_ZStream.avail_out == 0))?true:false; // more info in input buffer

      iDecompressed = m_ZStream.total_out-prevOut;
    }
    m_iFilePos += iDecompressed;
    return static_cast<unsigned int>(iDecompressed);
  }
  else if (mZipItem.method == 0) // uncompressed. just read from file, but mind our boundaries.
  {
    if (uiBufSize+m_iFilePos > mZipItem.csize)
      uiBufSize = mZipItem.csize-m_iFilePos;

    if (uiBufSize == 0)
      return 0; // we are past eof, this shouldn't happen but test anyway

    ssize_t iResult = mFile.Read(lpBuf,uiBufSize);
    if (iResult < 0)
      return -1;
    m_iZipFilePos += iResult;
    m_iFilePos += iResult;
    return iResult;
  }
  else
    return -1; // shouldn't happen. compression method checked in open
}

void CZipFile::Close()
{
  if (mZipItem.method == 8 && !m_bCached && m_iRead != -1)
    inflateEnd(&m_ZStream);

  mFile.Close();
}
/* CHANGED: JM - moved to CFile
bool CZipFile::ReadString(char* szLine, int iLineLength)
{
  if (!m_szStringBuffer)
  {
    m_szStringBuffer = new char[1024]; // 1024 byte long strings per read
    m_szStartOfStringBuffer = m_szStringBuffer;
    m_iDataInStringBuffer = 0;
    m_iRead = 0;
  }

  bool bEof = m_iDataInStringBuffer==0;
  while ((iLineLength > 1) && (m_iRead > -1))
  {
    if (m_iDataInStringBuffer > 0)
    {
      bEof = false;
      m_iRead = 1;
      int iMax = (iLineLength<m_iDataInStringBuffer?iLineLength-1:m_iDataInStringBuffer-1);
      for( int i=0;i<iMax;++i )
      {
        if (m_szStartOfStringBuffer[i] == '\r') // mac or win32 endings
        {
          strncpy(szLine,m_szStartOfStringBuffer,i);
          szLine[i] = '\0';
          m_iDataInStringBuffer -= i+1;
          m_szStartOfStringBuffer += i+1;
          if( m_szStartOfStringBuffer[0] == '\n') // win32 endings
          {
            m_szStartOfStringBuffer++;
            m_iDataInStringBuffer--;
          }
          return true;
        }
        else if (m_szStartOfStringBuffer[i] == '\n') // unix or fucked up win32 endings
        {
          strncpy(szLine,m_szStartOfStringBuffer,i);
          szLine[i] = '\0';
          m_iDataInStringBuffer -= i+1;
          m_szStartOfStringBuffer += i+1;
          if (m_szStartOfStringBuffer[0] == '\r')
          {
            m_szStartOfStringBuffer++;
            m_iDataInStringBuffer--;
          }
          return true;
        }
      }
      strncpy(szLine,m_szStartOfStringBuffer,iMax);
      szLine += iMax;
      iLineLength -= iMax;
      m_iDataInStringBuffer -= iMax;
    }

    if (m_iRead == 1 && (m_iDataInStringBuffer == 1))
    {
      m_szStringBuffer[0] = m_szStringBuffer[1023]; // need to make sure we don't loose any '\r\n' between buffers
      m_iDataInStringBuffer = Read(m_szStringBuffer+1,1023);
    }
    else
      m_iDataInStringBuffer = Read(m_szStringBuffer,1024);
    m_szStartOfStringBuffer = m_szStringBuffer;
    if (m_iDataInStringBuffer)
      m_iRead = 1;
    else
      m_iRead = -1;
  }
  szLine[0] = '\0';
  return !bEof;
}*/

bool CZipFile::FillBuffer()
{
  ssize_t sToRead = 65535;
  if (m_iZipFilePos+65535 > mZipItem.csize)
    sToRead = mZipItem.csize-m_iZipFilePos;

  if (sToRead <= 0)
    return false; // eof!

  if (mFile.Read(m_szBuffer,sToRead) != sToRead)
    return false;
  m_ZStream.avail_in = sToRead;
  m_ZStream.next_in = (Bytef*)m_szBuffer;
  m_iZipFilePos += sToRead;
  return true;
}

void CZipFile::DestroyBuffer(void* lpBuffer, int iBufSize)
{
  if (!m_bFlush)
    return;
  int iMessage = Z_OK;
  while ((iMessage == Z_OK) && (m_ZStream.avail_out == 0))
  {
    m_ZStream.next_out = (Bytef*)lpBuffer;
    m_ZStream.avail_out = iBufSize;
    iMessage = inflate(&m_ZStream,Z_SYNC_FLUSH);
  }
  m_bFlush = false;
}

int CZipFile::UnpackFromMemory(string& strDest, const string& strInput, bool isGZ)
{
  unsigned int iPos=0;
  int iResult=0;
  while( iPos+LHDR_SIZE < strInput.size() || isGZ)
  {
    if (!isGZ)
    {
      CZipManager::readHeader(strInput.data()+iPos,mZipItem);
      if( mZipItem.header != ZIP_LOCAL_HEADER )
        return iResult;
      if( (mZipItem.flags & 8) == 8 )
      {
        CLog::Log(LOGERROR,"FileZip: extended local header, not supported!");
        return iResult;
      }
    }
    if (!InitDecompress())
      return iResult;
    // we have a file - fill the buffer
    char* temp;
    int toRead=0;
    if (isGZ)
    {
      m_ZStream.avail_in = strInput.size();
      m_ZStream.next_in = (Bytef*)strInput.data();
      temp = new char[8192];
      toRead = 8191;
    }
    else
    {
      m_ZStream.avail_in = mZipItem.csize;
      m_ZStream.next_in = (Bytef*)strInput.data()+iPos+LHDR_SIZE+mZipItem.flength+mZipItem.elength;
      // init m_zipitem
      strDest.reserve(mZipItem.usize);
      temp = new char[mZipItem.usize+1];
      toRead = mZipItem.usize;
    }
    int iCurrResult;
    while( (iCurrResult=Read(temp,toRead)) > 0)
    {
      strDest.append(temp,temp+iCurrResult);
      iResult += iCurrResult;
    }
    Close();
    delete[] temp;
    iPos += LHDR_SIZE+mZipItem.flength+mZipItem.elength+mZipItem.csize;
    if (isGZ)
      break;
  }

  return iResult;
}


