#include "stdafx.h"
#include "FileReader.h"
#include "../../utils/SingleLock.h"


#define DATA_TO_KEEP_BEHIND 65536

static int sleeptime = 0;

CFileReader::CFileReader()
{
  m_bufferedDataPos = 0;
  m_readError = false;
  m_chunkBuffer = NULL;
}

CFileReader::~CFileReader()
{
  Close();
}

void CFileReader::Initialize(unsigned int bufferSize)
{
  sleeptime = 0;
  m_ringBuffer.Create(bufferSize + DATA_TO_KEEP_BEHIND, DATA_TO_KEEP_BEHIND);
}

void CFileReader::OnClearEvent()
{
  if (OnClear) OnClear();
}

bool CFileReader::Open(const CStdString &strFile, bool autoBuffer, bool preBuffer)
{
  Close();
  if (!m_file.Open(strFile, true, READ_TRUNCATED))
    return false;

  if (!(m_chunk_size = m_file.GetChunkSize()))
    m_chunk_size = 16384;
  if (m_chunk_size > m_ringBuffer.GetMaxWriteSize())
    Initialize(8 * m_chunk_size); //re-initialize with bigger buffersize
  m_chunkBuffer = new char[m_chunk_size];

  m_FileLength = m_file.GetLength();
  m_bufferedDataPos = 0;
  m_ringBuffer.Clear();
  m_ringBuffer.OnClear = MakeDelegate(this, &CFileReader::OnClearEvent);
  m_readError = false;
  if (autoBuffer)
  {
    StartBuffering();
    if (preBuffer)
    {
      unsigned int ms = 0;
      unsigned int minBuffered = (unsigned int)min(((m_ringBuffer.Size() - DATA_TO_KEEP_BEHIND)*0.5f), 32768);
      while ((m_ringBuffer.GetMaxReadSize() < minBuffered) && (ms < 10000))
      {
        Sleep(1);
        ms++;
      }
    }
  }
  else
  {
    //read one chunk to start with
    if (BufferChunk() == -1)
      return false;
  }
  return true;
}

void CFileReader::Close()
{
  m_ringBuffer.OnClear.clear();
  // kill our background reader thread
  StopThread();
  // and close the file
  m_file.Close();
  if (m_chunkBuffer) SAFE_DELETE_ARRAY(m_chunkBuffer);
}

void CFileReader::StartBuffering()
{
  if (ThreadHandle() == NULL)
    Create();
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
  __int64 sizeleft = size;
  while (sizeleft)
  {
    if (m_readError)
    { // uh oh
      CLog::Log(LOGERROR, "FileReader::Read - encountered read error");
      m_readError = false;
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
      if (sleeptime > 50)
      {
        CLog::DebugLog("FileReader: Waited a total of %i ms on data", sleeptime);
        sleeptime = 0;
      }
      if (m_FileLength > 0 && m_bufferedDataPos == m_FileLength)
      { // end of file reached
        return (int)(size - sizeleft);
      }
    }
    else
    { // nothing to copy yet
      //if not autobuffering just buffer a chunk
      if (!ThreadHandle())
      {
        BufferChunk();
      }
      else
      {
        //- sleep while we wait for our reader thread to read the data in.
        // check we don't reach EOF and loop forever
        if (m_FileLength > 0 && m_bufferedDataPos == m_FileLength)
        { // end of file reached
          return (int)(size - sizeleft);
        }
        Sleep(1);
        sleeptime++;
        //ten seconds timeout...
        if (sleeptime >= 10000)
        {
          m_readError = true;
          return 0;
        }
      }
    }
  }
  return (int)size;
}

// Seek.  We must seek to the appropriate position and update our buffers (flushing them
// if necessary).
__int64 CFileReader::Seek(__int64 pos, int whence)
{
  CSingleLock lock(m_fileLock);
  // calculate our new position
  __int64 newBufferedDataPos = 0;
  if (whence == SEEK_SET)
    newBufferedDataPos = pos;
  else if (whence == SEEK_END)
    newBufferedDataPos = m_FileLength + pos;
  else if (whence == SEEK_CUR)
    newBufferedDataPos = m_bufferedDataPos + pos;

  // check if we can do this seek immediately within our buffer
  if (m_ringBuffer.SkipBytes((int)(newBufferedDataPos - m_bufferedDataPos)))
    m_bufferedDataPos = newBufferedDataPos;
  else
  { // no valid data exists - flush our buffer and seek
    m_ringBuffer.Clear();
    m_bufferedDataPos = m_file.Seek(newBufferedDataPos, SEEK_SET);
  }
  return (int)m_bufferedDataPos;
}

int CFileReader::BufferChunk()
{
  // calculate whether we have more space to read data in
  if (m_chunk_size <= m_ringBuffer.GetMaxWriteSize())
  {
    // grab a file lock
    CSingleLock lock(m_fileLock);
    unsigned int amountToRead = m_chunk_size;
    if (m_FileLength > 0)
    {
      if (amountToRead > m_FileLength - m_file.GetPosition())
        amountToRead = (unsigned int)(m_FileLength - m_file.GetPosition());
    }
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
        return 1; // read some more immediately
      }
      else if (m_file.GetPosition() != m_FileLength)
      { // uh oh!
        m_readError = true;
        return -1;
      }
    }
    // amountToRead == 0 is not an error
    // the very last chunk of audio data is already in the buffer
    // so we dont need to read anymore
    /*
    else
    { // uh oh!
      m_readError = true;
      return -1;
    }
    */
  }
  return 0;
}

void CFileReader::Process()
{
  int rc;
  while (!m_bStop)
  {
    rc = BufferChunk();
    if (rc == -1)
      break; //read error
    if (rc == 0)
      // no space to read data into right now, or at end of file - sleep
      Sleep(1);
  }
}

bool CFileReader::SkipNext()
{
  return m_file.SkipNext();
}

int CFileReader::GetCacheLevel()
{
  return m_ringBuffer.GetFillPercentage();
}

bool CFileReader::CanSeek()
{
  return m_file.Seek(0, SEEK_CUR) >= 0;
}
