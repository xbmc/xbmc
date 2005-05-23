#pragma once
//#include "IFileReader.h"
#include "../../utils/thread.h"
#include "../../utils/CriticalSection.h"
#include "../../filesystem/file.h"
#include "RingHoldBuffer.h"

// A threaded file reader class that reads ahead of the current file position
// using a separate thread.
class CFileReader : public CThread
{
  const static int chunk_size = 16384;
public:
  CFileReader();
  virtual ~CFileReader();

  void Initialize(unsigned int bufferSize);
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
  __int64 m_bufferedDataPos;      // the position our client thinks we're at

  bool    m_readError;

  CCriticalSection m_fileLock;
  
  char m_chunkBuffer[chunk_size];   // buffer that we read chunks of our file into.
  CRingHoldBuffer m_ringBuffer;     // ring buffer that holds our read-in data
};
