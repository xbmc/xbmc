/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ZipFile.h"

#include "URL.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <sys/stat.h>

#define ZIP_CACHE_LIMIT 4*1024*1024

using namespace XFILE;

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
  const std::string& strOpts = url.GetOptions();
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
    CLog::Log(LOGERROR, "FileZip: unable to open zip file {}!", url.GetHostName());
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
    std::vector<char> buf(blockSize);
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
          ssize_t iToRead = (iFilePosition - m_iFilePos) > blockSize ? blockSize : iFilePosition - m_iFilePos;
          if (Read(buf.data(), iToRead) != iToRead)
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
        ssize_t iToRead = (iFilePosition - m_iFilePos)>blockSize ? blockSize : iFilePosition - m_iFilePos;
        if (Read(buf.data(), iToRead) != iToRead)
          return -1;
      }
      return m_iFilePos;
      break;

    case SEEK_END:
      // now this is a nasty bastard, possibly takes lotsoftime
      // uncompress, minding m_ZStream.total_out

      while(static_cast<ssize_t>(m_ZStream.total_out) < mZipItem.usize+iFilePosition)
      {
        ssize_t iToRead = (mZipItem.usize + iFilePosition - m_ZStream.total_out > blockSize) ? blockSize : mZipItem.usize + iFilePosition - m_ZStream.total_out;
        if (Read(buf.data(), iToRead) != iToRead)
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
  if (!buffer)
    return -1;

  if (!g_ZipManager.GetZipEntry(url, mZipItem))
  {
    if (url.GetFileName().empty() && CFile::Exists(url.GetHostName()))
    { // when accessing the zip "root" recognize it as a directory
      *buffer = {};
      buffer->st_mode = _S_IFDIR;
      return 0;
    }
    else
      return -1;
  }

  *buffer = {};
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
    size_t iMax = uiBufSize>m_iDataInStringBuffer?m_iDataInStringBuffer:uiBufSize;
    memcpy(lpBuf,m_szStartOfStringBuffer,iMax);
    uiBufSize -= iMax;
    m_iDataInStringBuffer -= iMax;
  }
  if (mZipItem.method == 8) // deflated
  {
    uLong iDecompressed = 0;
    uLong prevOut = m_ZStream.total_out;
    while ((iDecompressed < uiBufSize) && ((m_iZipFilePos < mZipItem.csize) || (m_bFlush)))
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

bool CZipFile::FillBuffer()
{
  ssize_t sToRead = 65535;
  if (m_iZipFilePos+65535 > mZipItem.csize)
    sToRead = mZipItem.csize-m_iZipFilePos;

  if (sToRead <= 0)
    return false; // eof!

  if (mFile.Read(m_szBuffer,sToRead) != sToRead)
    return false;
  m_ZStream.avail_in = static_cast<unsigned int>(sToRead);
  m_ZStream.next_in = reinterpret_cast<Byte*>(m_szBuffer);
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

int CZipFile::UnpackFromMemory(std::string& strDest, const std::string& strInput, bool isGZ)
{
  unsigned int iPos=0;
  int iResult=0;
  while( iPos+LHDR_SIZE < strInput.size() || isGZ)
  {
    if (!isGZ)
    {
      CZipManager::readHeader(strInput.data()+iPos,mZipItem);
      if (mZipItem.header == ZIP_DATA_RECORD_HEADER)
      {
        // this header concerns a file we already processed, so we can just skip it
        iPos += DREC_SIZE;
        continue;
      }
      if (mZipItem.header != ZIP_LOCAL_HEADER)
        return iResult;
      if( (mZipItem.flags & 8) == 8 )
      {
        // if an extended local header (=data record header) is present,
        // the following fields are 0 in the local header and we need to read
        // them from the extended local header

        // search for the extended local header
        unsigned int i = iPos + LHDR_SIZE + mZipItem.flength + mZipItem.elength;
        while (1)
        {
          if (i + DREC_SIZE > strInput.size())
          {
            CLog::Log(LOGERROR, "FileZip: extended local header expected, but not present!");
            return iResult;
          }
          if ((strInput[i] == 0x50) && (strInput[i + 1] == 0x4b) &&
            (strInput[i + 2] == 0x07) && (strInput[i + 3] == 0x08))
            break; // header found
          i++;
        }
        // ZIP is little endian:
        mZipItem.crc32 = static_cast<uint8_t>(strInput[i + 4]) |
                         static_cast<uint8_t>(strInput[i + 5]) << 8 |
                         static_cast<uint8_t>(strInput[i + 6]) << 16 |
                         static_cast<uint8_t>(strInput[i + 7]) << 24;
        mZipItem.csize = static_cast<uint8_t>(strInput[i + 8]) |
                         static_cast<uint8_t>(strInput[i + 9]) << 8 |
                         static_cast<uint8_t>(strInput[i + 10]) << 16 |
                         static_cast<uint8_t>(strInput[i + 11]) << 24;
        mZipItem.usize = static_cast<uint8_t>(strInput[i + 12]) |
                         static_cast<uint8_t>(strInput[i + 13]) << 8 |
                         static_cast<uint8_t>(strInput[i + 14]) << 16 |
                         static_cast<uint8_t>(strInput[i + 15]) << 24;
      }
    }
    if (!InitDecompress())
      return iResult;
    // we have a file - fill the buffer
    char* temp;
    ssize_t toRead=0;
    if (isGZ)
    {
      m_ZStream.avail_in = static_cast<unsigned int>(strInput.size());
      m_ZStream.next_in = const_cast<Bytef*>((const Bytef*)strInput.data());
      temp = new char[8192];
      toRead = 8191;
    }
    else
    {
      m_ZStream.avail_in = mZipItem.csize;
      m_ZStream.next_in = const_cast<Bytef*>((const Bytef*)strInput.data())+iPos+LHDR_SIZE+mZipItem.flength+mZipItem.elength;
      // init m_zipitem
      strDest.reserve(mZipItem.usize);
      temp = new char[mZipItem.usize+1];
      toRead = mZipItem.usize;
    }
    int iCurrResult;
    while((iCurrResult = static_cast<int>(Read(temp, toRead))) > 0)
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

bool CZipFile::DecompressGzip(const std::string& in, std::string& out)
{
  const int windowBits = MAX_WBITS + 16;

  z_stream strm;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;

  int err = inflateInit2(&strm, windowBits);
  if (err != Z_OK)
  {
    CLog::Log(LOGERROR, "FileZip: zlib error {}", err);
    return false;
  }

  const int bufferSize = 16384;
  unsigned char buffer[bufferSize];

  strm.avail_in = static_cast<unsigned int>(in.size());
  strm.next_in = reinterpret_cast<unsigned char*>(const_cast<char*>(in.c_str()));

  do
  {
    strm.avail_out = bufferSize;
    strm.next_out = buffer;
    int err = inflate(&strm, Z_NO_FLUSH);
    switch (err)
    {
      case Z_NEED_DICT:
        err = Z_DATA_ERROR;
        [[fallthrough]];
      case Z_DATA_ERROR:
      case Z_MEM_ERROR:
      case Z_STREAM_ERROR:
        CLog::Log(LOGERROR, "FileZip: failed to decompress. zlib error {}", err);
        inflateEnd(&strm);
        return false;
    }
    int read = bufferSize - strm.avail_out;
    out.append((char*)buffer, read);
  }
  while (strm.avail_out == 0);

  inflateEnd(&strm);
  return true;
}
