
#pragma once

#include "stdafx.h"
#include "DVDInputStream.h"

class CDVDInputStreamFFmpeg : public CDVDInputStream
{
public:
  CDVDInputStreamFFmpeg();
  virtual ~CDVDInputStreamFFmpeg();
  virtual bool Open(const char* strFile, const std::string &content);
  virtual void Close();
  virtual int Read(BYTE* buf, int buf_size);
  virtual __int64 Seek(__int64 offset, int whence);
  virtual bool IsEOF();
  virtual __int64 GetLength();
  
protected:
};
