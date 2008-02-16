
#pragma once


class CDVDInputStream;

// buffered class for subtitle reading

class CDVDSubtitleStream
{
public:
  CDVDSubtitleStream();
  virtual ~CDVDSubtitleStream();
  
  bool Open(const std::string& strFile);
  void Close();
  int Read(BYTE* buf, int buf_size);
  __int64 Seek(__int64 offset, int whence);

  char* ReadLine(char* pBuffer, int iLen);
  //wchar* ReadLineW(wchar* pBuffer, int iLen) { return NULL; };
  
protected:
  CDVDInputStream* m_pInputStream;
  BYTE* m_buffer;
  int   m_iMaxBufferSize;
  int   m_iBufferSize;
  int   m_iBufferPos;
  
  __int64 m_iStreamPos;
};

