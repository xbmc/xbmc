
#pragma once

#include "DVDInputStream.h"
#include "..\..\..\filesystem\file.h"

class CDVDInputStreamFile : public CDVDInputStream
{
public:
  CDVDInputStreamFile();
  virtual ~CDVDInputStreamFile();
  virtual bool Open(const char* strFile);
  virtual void Close();
  virtual int  Read(BYTE* buf, int buf_size);
  virtual int  Seek(__int64 offset, int whence);
  
protected:
  CFile* m_pFile;
};
