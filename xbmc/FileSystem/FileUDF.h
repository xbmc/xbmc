#ifndef _FILEUDF_H
#define _FILEUDF_H

#include "IFile.h"
#include "udf25.h"
#include "RingBuffer.h"

namespace XFILE
{

class CFileUDF : public IFile
{
public:
  CFileUDF();
  virtual ~CFileUDF();
  virtual int64_t GetPosition();
  virtual int64_t GetLength();
  virtual bool Open(const CURL& url);
  virtual bool Exists(const CURL& url);
  virtual int Stat(const CURL& url, struct __stat64* buffer);
  virtual unsigned int Read(void* lpBuf, int64_t uiBufSize);
  virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
  virtual void Close();
protected:
  bool m_bOpened;
  HANDLE m_hFile;
  CRingBuffer m_cache;
  udf25 m_udfIsoReaderLocal;
};
}

#endif
