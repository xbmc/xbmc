#include "stdafx.h"
#include "FileReader.h"
#include "../../utils/SingleLock.h"

CFileReader::CFileReader(unsigned int bufferSize, unsigned int dataToKeepBehind)
{
  m_ringBuffer.Create(bufferSize, dataToKeepBehind);
  m_bufferedDataPos = 0;
  m_readError = false;
}

CFileReader::~CFileReader()
{
  Close();
}

bool CFileReader::Open(const CStdString &strFile)
{
  if (!m_file.Open(strFile))
    return false;

  m_bufferedDataPos = 0;
  m_ringBuffer.Clear();
  m_readError = false;
  // start our thread process to start buffering the file
  if (ThreadHandle() == NULL)
    Create();
  return true;
}

void CFileReader::Close()
{
  // kill our background reader thread
  StopThread();
  // and close the file
  m_file.Close();
}

__int64 CFileReader::GetPosition()
{
  return m_bufferedDataPos;
}

__int64 CFileReader::GetLength()
{
  return m_file.GetLength();
}

int CFileReader::Read(void *out, __int64 size)
{
  char *byteOut = (char *)out;
  // read out of our buffers - we have a separate thread prefetching data
  if (m_bufferedDataPos + size > m_file.GetLength())
    size = m_file.GetLength() - m_bufferedDataPos;
  __int64 sizeleft = size;
  while (sizeleft)
  {
    // read data in from our ring buffer
    unsigned int readAhead = (unsigned int)(m_file.GetPosition() - m_bufferedDataPos);
    if (readAhead)
    {
      unsigned int amountToCopy = (unsigned int)min(readAhead, sizeleft);
      if (m_ringBuffer.ReadBinary(byteOut, amountToCopy))
      {
        m_bufferedDataPos += amountToCopy;
        byteOut += amountToCopy;
        sizeleft -= amountToCopy;
      }
    }
    else
    { // nothing to copy yet - sleep while we wait for our reader thread to read the data in.
      Sleep(1);
    }
  }
  return (int)size;
}

// Seek.  We must seek to the appropriate position and update our buffers (flushing them
// if necessary).
int CFileReader::Seek(__int64 pos, int whence)
{
  CSingleLock lock(m_fileLock);
  // calculate our new position
  __int64 newBufferedDataPos = 0;
  if (whence == SEEK_SET)
    newBufferedDataPos = pos;
  else if (whence == SEEK_END)
    newBufferedDataPos = m_file.GetLength() + pos;
  else if (whence == SEEK_CUR)
    newBufferedDataPos = m_bufferedDataPos + pos;

  // check if we can do this seek immediately within our buffer
  if (m_ringBuffer.SkipBytes((int)(newBufferedDataPos - m_bufferedDataPos)))
    m_bufferedDataPos = newBufferedDataPos;
  else
  { // no valid data exists - flush our buffer and seek
    m_ringBuffer.Clear();
    m_bufferedDataPos = newBufferedDataPos;
    m_file.Seek(m_bufferedDataPos, SEEK_SET);
  }
  return 0;
}

void CFileReader::Process()
{
  while (!m_bStop)
  {
    // calculate whether we have more space to read data in
    if (m_ringBuffer.GetMaxWriteSize())
    {
      // grab a file lock
      CSingleLock lock(m_fileLock);
      unsigned int amountToRead = min(chunk_size, m_ringBuffer.GetMaxWriteSize()/* - m_dataToKeepBehind*/);
      // check the range of our valid data
      if (m_file.Read(m_chunkBuffer, amountToRead))
      {
        if (!m_ringBuffer.WriteBinary(m_chunkBuffer, amountToRead))
        {
          throw 1;  // should never get here!
        }
      }
      else
      { // uh oh!
        m_readError = true;
        break;
      }
    }
    else
    { // no space to read data into right now - sleep
      Sleep(1);
    }
  }
}
