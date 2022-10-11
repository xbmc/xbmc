/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Encoder.h"

#include "filesystem/File.h"
#include "utils/log.h"

#include <string.h>
#include <utility>

using namespace KODI::CDRIP;

CEncoder::CEncoder() = default;

CEncoder::~CEncoder()
{
  FileClose();
}

bool CEncoder::EncoderInit(const std::string& strFile, int iInChannels, int iInRate, int iInBits)
{
  m_dwWriteBufferPointer = 0;
  m_strFile = strFile;
  m_iInChannels = iInChannels;
  m_iInSampleRate = iInRate;
  m_iInBitsPerSample = iInBits;

  if (!FileCreate(strFile))
  {
    CLog::Log(LOGERROR, "CEncoder::{} - Cannot open file: {}", __func__, strFile);
    return false;
  }

  return Init();
}

ssize_t CEncoder::EncoderEncode(uint8_t* pbtStream, size_t nNumBytesRead)
{
  const int iBytes = Encode(pbtStream, nNumBytesRead);
  if (iBytes < 0)
  {
    CLog::Log(LOGERROR, "CEncoder::{} - Internal encoder error: {}", __func__, iBytes);
    return 0;
  }
  return 1;
}

bool CEncoder::EncoderClose()
{
  if (!Close())
    return false;

  FlushStream();
  FileClose();

  return true;
}

bool CEncoder::FileCreate(const std::string& filename)
{
  m_file = std::make_unique<XFILE::CFile>();
  if (m_file)
    return m_file->OpenForWrite(filename, true);
  return false;
}

bool CEncoder::FileClose()
{
  if (m_file)
  {
    m_file->Close();
    m_file.reset();
  }
  return true;
}

// return total bytes written, or -1 on error
ssize_t CEncoder::FileWrite(const uint8_t* pBuffer, size_t iBytes)
{
  if (!m_file)
    return -1;

  const ssize_t dwBytesWritten = m_file->Write(pBuffer, iBytes);
  if (dwBytesWritten <= 0)
    return -1;

  return dwBytesWritten;
}

ssize_t CEncoder::Seek(ssize_t iFilePosition, int iWhence)
{
  if (!m_file)
    return -1;
  FlushStream();
  return m_file->Seek(iFilePosition, iWhence);
}

// write the stream to our writebuffer, and write the buffer to disk if it's full
ssize_t CEncoder::Write(const uint8_t* pBuffer, size_t iBytes)
{
  if ((WRITEBUFFER_SIZE - m_dwWriteBufferPointer) > iBytes)
  {
    // writebuffer is big enough to fit data
    memcpy(m_btWriteBuffer + m_dwWriteBufferPointer, pBuffer, iBytes);
    m_dwWriteBufferPointer += iBytes;
    return iBytes;
  }
  else
  {
    // buffer is not big enough to fit data
    if (m_dwWriteBufferPointer == 0)
    {
      // nothing in our buffer, just write the entire pBuffer to disk
      return FileWrite(pBuffer, iBytes);
    }

    const size_t dwBytesRemaining = iBytes - (WRITEBUFFER_SIZE - m_dwWriteBufferPointer);
    // fill up our write buffer and write it to disk
    memcpy(m_btWriteBuffer + m_dwWriteBufferPointer, pBuffer,
           (WRITEBUFFER_SIZE - m_dwWriteBufferPointer));
    FileWrite(m_btWriteBuffer, WRITEBUFFER_SIZE);
    m_dwWriteBufferPointer = 0;

    // pbtRemaining = pBuffer + bytesWritten
    const uint8_t* pbtRemaining = pBuffer + (iBytes - dwBytesRemaining);
    if (dwBytesRemaining > WRITEBUFFER_SIZE)
    {
      // data is not going to fit in our buffer, just write it to disk
      if (FileWrite(pbtRemaining, dwBytesRemaining) == -1)
        return -1;
      return iBytes;
    }
    else
    {
      // copy remaining bytes to our currently empty writebuffer
      memcpy(m_btWriteBuffer, pbtRemaining, dwBytesRemaining);
      m_dwWriteBufferPointer = dwBytesRemaining;
      return iBytes;
    }
  }
}

// flush the contents of our writebuffer
ssize_t CEncoder::FlushStream()
{
  if (m_dwWriteBufferPointer == 0)
    return 0;

  const ssize_t iResult = FileWrite(m_btWriteBuffer, m_dwWriteBufferPointer);
  m_dwWriteBufferPointer = 0;

  return iResult;
}
