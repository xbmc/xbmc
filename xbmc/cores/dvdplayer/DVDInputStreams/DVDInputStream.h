
#pragma once

#define DVDSTREAM_TYPE_FILE 1
#define DVDSTREAM_TYPE_DVD 2

class CDVDInputStream
{
public:
  CDVDInputStream();
  virtual ~CDVDInputStream();
  virtual bool Open(const char* strFile) = 0;
  virtual void Close() = 0;
  virtual int Read(BYTE* buf, int buf_size) = 0;
  virtual __int64 Seek(__int64 offset, int whence) = 0;

  const char* GetFileName();

  int m_streamType;

protected:
  char* m_strFileName;
};
