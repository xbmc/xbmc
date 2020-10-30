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

CEncoder::CEncoder(std::shared_ptr<IEncoder> encoder)
{
  m_file = NULL;
  m_dwWriteBufferPointer = 0;
  m_impl = std::move(encoder);
}

CEncoder::~CEncoder()
{
  FileClose();
}

int CEncoder::WriteCallback(void *opaque, const uint8_t *data, int size)
{
  if (opaque)
  {
    CEncoder *encoder = static_cast<CEncoder *>(opaque);
    return encoder->WriteStream(data, size);
  }
  return -1;
}

int64_t CEncoder::SeekCallback(void *opaque, int64_t position, int whence)
{
  if (opaque)
  {
    CEncoder *encoder = static_cast<CEncoder *>(opaque);
    return encoder->FileSeek(position, whence);
  }
  return -1;
}

bool CEncoder::Init(const char* strFile, int iInChannels, int iInRate, int iInBits)
{
  if (strFile == NULL) return false;

  m_dwWriteBufferPointer = 0;
  m_impl->m_strFile = strFile;

  m_impl->m_iInChannels = iInChannels;
  m_impl->m_iInSampleRate = iInRate;
  m_impl->m_iInBitsPerSample = iInBits;

  if (!FileCreate(strFile))
  {
    CLog::Log(LOGERROR, "Error: Cannot open file: %s", strFile);
    return false;
  }

  AddonToKodiFuncTable_AudioEncoder callbacks;
  callbacks.kodiInstance = this;
  callbacks.write = WriteCallback;
  callbacks.seek = SeekCallback;
  return m_impl->Init(callbacks);
}

bool CEncoder::FileCreate(const char* filename)
{
  delete m_file;

  m_file = new XFILE::CFile;
  if (m_file)
    return m_file->OpenForWrite(filename, true);
  return false;
}

bool CEncoder::FileClose()
{
  if (m_file)
  {
    m_file->Close();
    delete m_file;
    m_file = NULL;
  }
  return true;
}

// return total bytes written, or -1 on error
int CEncoder::FileWrite(const void *pBuffer, uint32_t iBytes)
{
  if (!m_file)
    return -1;

  ssize_t dwBytesWritten = m_file->Write(pBuffer, iBytes);
  if (dwBytesWritten <= 0)
    return -1;

  return dwBytesWritten;
}

int64_t CEncoder::FileSeek(int64_t iFilePosition, int iWhence)
{
  if (!m_file)
    return -1;
  FlushStream();
  return m_file->Seek(iFilePosition, iWhence);
}

// write the stream to our writebuffer, and write the buffer to disk if it's full
int CEncoder::WriteStream(const void *pBuffer, uint32_t iBytes)
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

    uint32_t dwBytesRemaining = iBytes - (WRITEBUFFER_SIZE - m_dwWriteBufferPointer);
    // fill up our write buffer and write it to disk
    memcpy(m_btWriteBuffer + m_dwWriteBufferPointer, pBuffer, (WRITEBUFFER_SIZE - m_dwWriteBufferPointer));
    FileWrite(m_btWriteBuffer, WRITEBUFFER_SIZE);
    m_dwWriteBufferPointer = 0;

    // pbtRemaining = pBuffer + bytesWritten
    const uint8_t* pbtRemaining = (const uint8_t *)pBuffer + (iBytes - dwBytesRemaining);
    if (dwBytesRemaining > WRITEBUFFER_SIZE)
    {
      // data is not going to fit in our buffer, just write it to disk
      if (FileWrite(pbtRemaining, dwBytesRemaining) == -1) return -1;
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
int CEncoder::FlushStream()
{
  int iResult;
  if (m_dwWriteBufferPointer == 0) return 0;

  iResult = FileWrite(m_btWriteBuffer, m_dwWriteBufferPointer);
  m_dwWriteBufferPointer = 0;

  return iResult;
}

int CEncoder::Encode(int nNumBytesRead, uint8_t* pbtStream)
{
  int iBytes = m_impl->Encode(nNumBytesRead, pbtStream);

  if (iBytes < 0)
  {
    CLog::Log(LOGERROR, "Internal encoder error: %i", iBytes);
    return 0;
  }
  return 1;
}

bool CEncoder::CloseEncode()
{
  if (!m_impl->Close())
    return false;

  FlushStream();
  FileClose();

  return true;
}
