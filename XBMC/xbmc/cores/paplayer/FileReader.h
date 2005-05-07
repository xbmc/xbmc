#pragma once
//#include "IFileReader.h"
#include "../../utils/thread.h"
#include "../../utils/CriticalSection.h"
#include "../../filesystem/file.h"

class CFileBuffer
{
public:
  CFileBuffer(unsigned int buffersize)
  {
    lock = false;
    offset = 0;
    size = 0;
    buffer = new BYTE[buffersize];
  }
  ~CFileBuffer()
  {
    if (buffer)
      delete[] buffer;
  };
  void Lock()
  {
    while (lock)
      Sleep(1);
    lock = true;
  }
  void Unlock()
  {
    lock = false;
  }
public:
  __int64 offset;           // offset of start of this buffer into the file
  unsigned int size;        // size in bytes of this buffer
  BYTE *buffer;             // the buffer
private:
  bool lock;                // to allow separate threads to process it
};

// A threaded file reader class that reads ahead of the current file position
// using a separate thread.
class CFileReader : public CThread
{
public:
  CFileReader(unsigned int bufferSize, unsigned int numBuffers);
  virtual ~CFileReader();

  virtual bool Open(const CStdString &strFile);
  virtual void Close();
  virtual int Read(void *out, __int64 size);
  virtual __int64 GetPosition();
  virtual int Seek(__int64 pos, int whence = SEEK_SET);
  virtual __int64 GetLength();

protected:
  int ReadFromBuffers(BYTE *out, __int64 *pos, __int64 *size);
  void ReadIntoBuffers();

  // thread functions
  virtual void OnStartup() {}
  virtual void Process();
  virtual void OnExit() {};

private:
  CFile   m_file;
  __int64 m_filePos;
  vector<CFileBuffer *> m_buffer;
  unsigned int m_bufferSize;
  bool    m_readError;
  // file lock
  CCriticalSection m_fileLock;
};
