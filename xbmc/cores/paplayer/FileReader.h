#pragma once
//#include "IFileReader.h"
#include "../../utils/thread.h"
#include "../../utils/CriticalSection.h"
#include "../../filesystem/file.h"

// A threaded file reader class that reads ahead of the current file position
// using a separate thread.
class CFileReader : public CThread
{
public:
  CFileReader(unsigned int bufferSize, unsigned int dataToKeepBehind, unsigned int chunkSize);
  virtual ~CFileReader();

  virtual bool Open(const CStdString &strFile);
  virtual void Close();
  virtual int Read(void *out, __int64 size);
  virtual __int64 GetPosition();
  virtual int Seek(__int64 pos, int whence = SEEK_SET);
  virtual __int64 GetLength();

protected:
  // thread functions
  virtual void OnStartup() {}
  virtual void Process();
  virtual void OnExit() {};

private:
  CFile   m_file;
  __int64 m_bufferedDataStart;    // the earliest piece of data in our buffer that is considered valid
  __int64 m_bufferedDataPos;      // the position our client thinks we're at
  // __int64 m_bufferedDataEnd;   // the current file position - no need to store this.

  unsigned int m_readFromPos;     // the position in the buffer that corresponds to the data
                                  // at m_bufferedDataPos in the file.  From this we calculate
                                  // where in the buffer we should read from and write to.

  // our buffer
  BYTE *m_buffer;
  unsigned int m_bufferSize;        // buffer size
  unsigned int m_dataToKeepBehind;  // amount of data to keep behind the current data position
  unsigned int m_chunkSize;         // chunk size to read at a time.

  bool    m_readError;
  // file lock
  CCriticalSection m_fileLock;
};
