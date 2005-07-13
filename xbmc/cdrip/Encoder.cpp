#include "../stdafx.h"
#include "Encoder.h"
#include "..\util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

bool CEncoder::Init(const char* strFile, int iInChannels, int iInRate, int iInBits)
{
  if (strFile == NULL) return false;

  m_dwWriteBufferPointer = 0;
  m_strFile = strFile;

  m_iInChannels = iInChannels;
  m_iInSampleRate = iInRate;
  m_iInBitsPerSample = iInBits;

  if (!FileCreate(strFile))
  {
    CLog::Log(LOGERROR, "Error: Cannot open file: %s", strFile);
    return false;
  }
  return true;
}

bool CEncoder::FileCreate(const char* filename)
{
  m_hFile = CreateFile(filename, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

  return (m_hFile != INVALID_HANDLE_VALUE);
}

bool CEncoder::FileClose()
{
  if (m_hFile != INVALID_HANDLE_VALUE) CloseHandle(m_hFile);

  m_hFile = INVALID_HANDLE_VALUE;
  return true;
}

// return total bytes written, or -1 on error
int CEncoder::FileWrite(LPCVOID pBuffer, DWORD iBytes)
{
  DWORD dwBytesWritten;

  if (m_hFile == INVALID_HANDLE_VALUE) return -1;

  if (!WriteFile(m_hFile, pBuffer, iBytes, &dwBytesWritten, NULL)) dwBytesWritten = -1;

  return dwBytesWritten;
}

// write the stream to our writebuffer, and write the buffer to disk if it's full
int CEncoder::WriteStream(LPCVOID pBuffer, DWORD iBytes)
{
  if ((WRITEBUFFER_SIZE - m_dwWriteBufferPointer) > iBytes)
  {
    // writebuffer is big enough to fit data
    fast_memcpy(m_btWriteBuffer + m_dwWriteBufferPointer, pBuffer, iBytes);
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

    DWORD dwBytesRemaining = iBytes - (WRITEBUFFER_SIZE - m_dwWriteBufferPointer);
    // fill up our write buffer and write it to disk
    fast_memcpy(m_btWriteBuffer + m_dwWriteBufferPointer, pBuffer, (WRITEBUFFER_SIZE - m_dwWriteBufferPointer));
    FileWrite(m_btWriteBuffer, WRITEBUFFER_SIZE);
    m_dwWriteBufferPointer = 0;

    // pbtRemaining = pBuffer + bytesWritten
    BYTE* pbtRemaining = (BYTE*)pBuffer + (iBytes - dwBytesRemaining);
    if (dwBytesRemaining > WRITEBUFFER_SIZE)
    {
      // data is not going to fit in our buffer, just write it to disk
      if (FileWrite(pbtRemaining, dwBytesRemaining) == -1) return -1;
      return iBytes;
    }
    else
    {
      // copy remaining bytes to our currently empty writebuffer
      fast_memcpy(m_btWriteBuffer, pbtRemaining, dwBytesRemaining);
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
