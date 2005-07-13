#include "../../stdafx.h"
#include "FileReader.h"
#include "../../utils/SingleLock.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define DATA_TO_KEEP_BEHIND 65536

CFileReader::CFileReader()
{
  m_bufferedDataPos = 0;
  m_readError = false;
}

CFileReader::~CFileReader()
{
  Close();
}

void CFileReader::Initialize(unsigned int bufferSize)
{
  m_ringBuffer.Create(bufferSize + DATA_TO_KEEP_BEHIND, DATA_TO_KEEP_BEHIND);
}

bool CFileReader::Open(const CStdString &strFile)
{
  Close();
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

static int sleeptime = 0;

int CFileReader::Read(void *out, __int64 size)
{
  char *byteOut = (char *)out;
  // read out of our buffers - we have a separate thread prefetching data
  __int64 sizeleft = size;
  while (sizeleft)
  {
    if (m_readError)
    { // uh oh
      CLog::Log(LOGERROR, "FileReader::Read - encountered read error");
      return 0;
    }
    // read data in from our ring buffer
    unsigned int readAhead = m_ringBuffer.GetMaxReadSize();
    if (readAhead)
    {
      unsigned int amountToCopy = (unsigned int)min(readAhead, sizeleft);
      if (m_ringBuffer.ReadBinary(byteOut, amountToCopy))
      {
        m_bufferedDataPos += amountToCopy;
        byteOut += amountToCopy;
        sizeleft -= amountToCopy;
      }
      if (sleeptime)
      {
        CLog::Log(LOGDEBUG, "FileReader: Waited a total of %i ms on data", sleeptime);
        sleeptime = 0;
      }
      if (m_bufferedDataPos == m_file.GetLength())
      { // end of file reached
        return (int)(size - sizeleft);
      }
    }
    else
    { // nothing to copy yet - sleep while we wait for our reader thread to read the data in.
      // check we don't reach EOF and loop forever
      if (m_bufferedDataPos == m_file.GetLength())
      { // end of file reached
        return (int)(size - sizeleft);
      }
      Sleep(1);
      sleeptime++;
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
  return (int)m_bufferedDataPos;
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
      unsigned int amountToRead = min(chunk_size, m_ringBuffer.GetMaxWriteSize());
      if (amountToRead > m_file.GetLength() - m_file.GetPosition())
        amountToRead = (unsigned int)(m_file.GetLength() - m_file.GetPosition());
      // check the range of our valid data
      if (amountToRead)
      {
        unsigned int amountRead = m_file.Read(m_chunkBuffer, amountToRead);
        if (amountRead > 0)
        {
          if (!m_ringBuffer.WriteBinary(m_chunkBuffer, amountRead))
          {
            throw 1;  // should never get here!
          }
          continue; // read some more immediately
        }
        else if (m_file.GetPosition() != m_file.GetLength())
        { // uh oh!
          m_readError = true;
          break;
        }
      }
    }
    // no space to read data into right now, or at end of file - sleep
    Sleep(1);
  }
}
