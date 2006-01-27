
#pragma once

#include "DVDInputStream.h"

class CDVDInputStreamMemory : public CDVDInputStream
{
public:
  CDVDInputStreamMemory();
  virtual ~CDVDInputStreamMemory();
  virtual bool Open(const char* strFile);
  virtual void Close();
  virtual int Read(BYTE* buf, int buf_size);
  virtual __int64 Seek(__int64 offset, int whence);
  virtual bool IsEOF();

protected:
  BYTE* m_pData;
  int   m_iDataSize;
  int   m_iDataPos;
};
