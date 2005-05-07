#include "stdafx.h"
#include "FileReader.h"
#include "../../utils/SingleLock.h"

CFileReader::CFileReader(unsigned int bufferSize, unsigned int dataToKeepBehind, unsigned int chunkSize)
{
  m_bufferSize = bufferSize;
  m_chunkSize = chunkSize;
  m_dataToKeepBehind = dataToKeepBehind;
  m_buffer = new BYTE[m_bufferSize];
  m_bufferedDataStart = 0;
  m_bufferedDataPos = 0;
  m_readFromPos = 0;
  m_readError = false;
}

CFileReader::~CFileReader()
{
  Close();
  if (m_buffer)
    delete[] m_buffer;
}

bool CFileReader::Open(const CStdString &strFile)
{
  if (!m_file.Open(strFile))
    return false;

  m_bufferedDataStart = 0;
  m_bufferedDataPos = 0;
  m_readFromPos = 0;
  m_readError = false;
  // start our thread process to start buffering the file
  if (ThreadHandle() == NULL)
    Create();
  return true;
}

void CFileReader::Close()
{
  // kill our background reader thread
  m_bStop = true;
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
  BYTE *byteOut = (BYTE *)out;
  // read out of our buffers - we have a separate thread prefetching data
  if (m_bufferedDataPos + size > m_file.GetLength())
    size = m_file.GetLength() - m_bufferedDataPos;
  __int64 sizeleft = size;
  while (sizeleft)
  {
    // read data in from our buffer
    unsigned int readAhead = (unsigned int)(m_file.GetPosition() - m_bufferedDataPos);
    if (readAhead)
    {
      unsigned int amountToCopy = (unsigned int)min(readAhead, sizeleft);
      if (m_readFromPos + amountToCopy > m_bufferSize)
      { // we must wrap around in our circular buffer
        unsigned int copyAmount = m_bufferSize - m_readFromPos;
        memcpy(byteOut, m_buffer + m_readFromPos, copyAmount);
        m_readFromPos = 0;
        m_bufferedDataPos += copyAmount;
        byteOut += copyAmount;
        sizeleft -= copyAmount;
        amountToCopy -= copyAmount;
      }
      memcpy(byteOut, m_buffer + m_readFromPos, amountToCopy);
      m_readFromPos += amountToCopy;
      m_bufferedDataPos += amountToCopy;
      byteOut += amountToCopy;
      sizeleft -= amountToCopy;
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

  if (newBufferedDataPos < m_file.GetPosition() && newBufferedDataPos >= m_bufferedDataStart)
  { // have valid data - just update our file position
    m_readFromPos += (unsigned int)(newBufferedDataPos - m_bufferedDataPos);
    if (m_readFromPos >= m_bufferSize)
      m_readFromPos -= m_bufferSize;
    m_bufferedDataPos = newBufferedDataPos;
  }
  else
  { // no valid data exists - flush our buffer and seek
    m_readFromPos = 0;
    m_bufferedDataStart = newBufferedDataPos;
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
    unsigned int dataAhead = (unsigned int)(m_file.GetPosition() - m_bufferedDataPos);
    if (dataAhead < m_bufferSize - m_dataToKeepBehind)
    {
      // grab a file lock
      CSingleLock lock(m_fileLock);
      unsigned int amountToRead = min(m_chunkSize, m_bufferSize - m_dataToKeepBehind - dataAhead);
      // check the range of our valid data
      if (m_file.GetPosition() + amountToRead - m_bufferedDataStart > m_bufferSize)
      { // we're gonna be going over data that is over the start - move our start
        // forward to accomodate.
        m_bufferedDataStart = m_file.GetPosition() + amountToRead - m_bufferSize;
      }
      // work out where in the ringbuffer to read the data into
      unsigned int inBufferPos = m_readFromPos + dataAhead;
      if (inBufferPos > m_bufferSize)
        inBufferPos -= m_bufferSize;  // wraparound
      // do the read
      if (inBufferPos + amountToRead > m_bufferSize)
      {
        unsigned int readAmount = m_bufferSize - inBufferPos;
        m_file.Read(m_buffer + inBufferPos, readAmount);
        amountToRead -= readAmount;
        inBufferPos = 0;
      }
      m_file.Read(m_buffer + inBufferPos, amountToRead);
    }
    else
    { // no space to read data into right now - sleep
      Sleep(1);
    }
  }
}
