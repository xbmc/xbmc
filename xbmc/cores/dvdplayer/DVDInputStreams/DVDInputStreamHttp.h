
#pragma once

#include "DVDInputStream.h"
#include "../../../FileSystem/FileCurl.h"
#include "../../../utils/HttpHeader.h"

class CDVDInputStreamHttp : public CDVDInputStream
{
public:
  CDVDInputStreamHttp();
  virtual ~CDVDInputStreamHttp();
  virtual bool Open(const char* strFile, const std::string& content);
  virtual void Close();
  virtual int Read(BYTE* buf, int buf_size);
  virtual __int64 Seek(__int64 offset, int whence);
  virtual bool IsEOF();
  virtual __int64 GetLength();
  
  CHttpHeader* GetHttpHeader();
  
protected:
  XFILE::CFileCurl* m_pFile;
  CHttpHeader m_httpHeader;
  bool m_eof;
};

