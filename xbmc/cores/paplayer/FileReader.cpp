#include "stdafx.h"
#include "FileReader.h"
#include "../../utils/SingleLock.h"

#define READ_FINISHED -1
#define READ_NO_DATA_AVAILABLE 0
#define READ_GOT_DATA 1

CFileReader::CFileReader(unsigned int bufferSize, unsigned int numBuffers)
{
  m_bufferSize = bufferSize;
  for (unsigned int i = 0; i < numBuffers; i++)
  {
    CBuffer *buffer = new CBuffer(m_bufferSize);
    if (buffer)
      m_buffer.push_back(buffer);
  }
  m_filePos = 0;
  m_readError = false;
}

CFileReader::~CFileReader()
{
  Close();
  for (unsigned int i = 0; i < m_buffer.size(); i++)
  {
    if (m_buffer[i])
      delete m_buffer[i];
    m_buffer[i] = NULL;
  }
  m_buffer.clear();
}

bool CFileReader::Open(const CStdString &strFile)
{
  if (!m_file.Open(strFile))
    return false;
  m_filePos = 0;
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
  return m_filePos;
}

__int64 CFileReader::GetLength()
{
  return m_file.GetLength();
}

int CFileReader::Read(void *out, __int64 size)
{
  // read out of our buffers - we have a separate thread prefetching data
  if (m_filePos + size > m_file.GetLength())
    size = m_file.GetLength() - m_filePos;
  __int64 sizeleft = size;
  __int64 pos = 0;
  while (sizeleft)
  {
    // wait for our other thread - give it a breather by sleeping
    int ret = ReadFromBuffers((BYTE *)out, &pos, &sizeleft);
    if (ret == READ_FINISHED)
      break;  // we are done
    else if (ret == READ_NO_DATA_AVAILABLE)
    { // sleep to give our input thread time to finish reading more data in
      Sleep(1);
    }
    if (m_readError)
      return 0;
  }
  return (int)size;
}

// Reads data from our buffers, updating pos and size respectively.
// returns true if it's taken some data - false otherwise.
// pos is the byte position in our output buffer (altered as we read data in)
// size is the amount of bytes left to read (altered as we read data in)
int CFileReader::ReadFromBuffers(BYTE *out, __int64 *pos, __int64 *size)
{
  if (!*size) return READ_FINISHED;
  // run through our buffers and check whether they have relavent data
  for (unsigned int i = 0; i < m_buffer.size(); i++)
  {
    if (m_buffer[i]->offset <= m_filePos && m_filePos < m_buffer[i]->offset + m_buffer[i]->size)
    { // have data in this buffer to read from
      m_buffer[i]->Lock();
      // copy to output buffer
      unsigned int amount = (unsigned int)min(m_buffer[i]->offset + m_buffer[i]->size - m_filePos, *size);
      memcpy(out + *pos, m_buffer[i]->buffer + m_filePos - m_buffer[i]->offset, amount);
      m_filePos += amount;
      *pos += amount;
      *size -= amount;
      // if our buffer is empty show it
      // TODO: Buffer empty should probably be handled by our reader thread, so that
      // we can keep some buffers both behind and forward of where we are to allow for
      // nicer rewinding.  Currently we only keep data forward of where we are (excluding
      // the current buffer).
      if (m_filePos == m_buffer[i]->offset + m_buffer[i]->size)
        m_buffer[i]->size = 0;
      m_buffer[i]->Unlock();
      return READ_GOT_DATA;
    }
  }
  return READ_NO_DATA_AVAILABLE;
}

// Seek.  We must seek to the appropriate position and update our buffers (flushing them
// if necessary).
// TODO: This could be optimized as follows:
// 1. Set our new real file position to our virtual file position.
// 2. See if we have a single valid buffer (as is done currently, but using the real file position variable).
// 3. If so, mark it as valid, and increment our real file pointer accordingly, and go to 2.
// 4. If not, remove all non-valid buffers.
int CFileReader::Seek(__int64 pos, int whence)
{
  CSingleLock lock(m_fileLock);
  // calculate our position
  if (whence == SEEK_SET)
    m_filePos = pos;
  else if (whence == SEEK_END)
    m_filePos = m_file.GetLength() + pos;
  else if (whence == SEEK_CUR)
    m_filePos += pos;
  __int64 newFilePos = m_filePos;
  // flush any out of range buffers - both forward and backward.
  for (unsigned int i = 0; i < m_buffer.size(); i++)
  {
    if ((m_buffer[i]->offset + m_buffer[i]->size < m_filePos) || (m_filePos < m_buffer[i]->offset))
    { // m_buffer[i] is out of range
      // grab a lock
      m_buffer[i]->Lock();
      // and flush it out
      m_buffer[i]->size = 0;
      m_buffer[i]->offset = m_filePos - m_bufferSize;
      m_buffer[i]->Unlock();
    }
    else
    { // this buffer has valid data - let's update our real file position accordingly
      if (newFilePos < m_buffer[i]->offset + m_buffer[i]->size)
        newFilePos = m_buffer[i]->offset + m_buffer[i]->size;
    }
  }
  // and do the seek
  m_file.Seek(newFilePos, SEEK_SET);
  return 0;
}

void CFileReader::Process()
{
  while (!m_bStop)
  {
    ReadIntoBuffers();
  }
}

// Called via our reader thread to fill up buffers as necessary.
void CFileReader::ReadIntoBuffers()
{
  __int64 amountLeft = m_file.GetLength() - m_file.GetPosition();
  if (!amountLeft)
  {
    Sleep(1);
    return;
  }
  // run through our buffers and see if we have some free.
  unsigned int numfreebuffers = 0;
  unsigned int *freebuffer = new unsigned int[m_buffer.size()];
  for (unsigned int i = 0; i < m_buffer.size(); i++)
  {
    if (!m_buffer[i]->size)
      freebuffer[numfreebuffers++] = i;
  }
  if (numfreebuffers)
  { // have free buffers - find the oldest one
    CSingleLock lock(m_fileLock);
    unsigned int oldest = freebuffer[0];
    for (unsigned int i = 1; i < numfreebuffers; i++)
      if (m_buffer[oldest]->offset > m_buffer[freebuffer[i]]->offset) oldest = freebuffer[i];
    // and fill it up
    m_buffer[oldest]->Lock();
    int amount = (int)min(amountLeft, m_bufferSize);
    m_buffer[oldest]->offset = m_file.GetPosition();
    if (!m_file.Read(m_buffer[oldest]->buffer, amount))
      m_readError = true;
    m_buffer[oldest]->size = amount;
    m_buffer[oldest]->Unlock();
  }
  else
  {
    // nothing to do at this stage - let's sleep
    Sleep(1);
  }
  delete[] freebuffer;
}