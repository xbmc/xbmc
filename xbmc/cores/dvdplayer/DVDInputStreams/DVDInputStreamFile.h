
#pragma once

#include "DVDInputStream.h"

class CDVDInputStreamFile : public CDVDInputStream
{
public:
  CDVDInputStreamFile();
  virtual ~CDVDInputStreamFile();
  virtual bool Open(const char* strFile, const std::string &content);
  virtual void Close();
  virtual int Read(BYTE* buf, int buf_size);
  virtual __int64 Seek(__int64 offset, int whence);
  virtual bool IsEOF();
  virtual __int64 GetLength();
  
protected:
  XFILE::CFile* m_pFile;
  bool m_eof;
};
