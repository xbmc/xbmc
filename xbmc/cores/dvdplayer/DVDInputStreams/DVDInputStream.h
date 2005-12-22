
#pragma once

enum DVDStreamType
{
  DVDSTREAM_TYPE_NONE   = -1,
  DVDSTREAM_TYPE_FILE   = 1,
  DVDSTREAM_TYPE_DVD    = 2,
  DVDSTREAM_TYPE_HTTP   = 3,
  DVDSTREAM_TYPE_MEMORY = 4
};

#define DVDSTREAM_BLOCK_SIZE_FILE (2048 * 16)
#define DVDSTREAM_BLOCK_SIZE_DVD  2048

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
  bool HasExtension(char* sExtension);
  
  int GetBlockSize() { return DVDSTREAM_BLOCK_SIZE_FILE; }
  bool IsStreamType(DVDStreamType type) { return m_streamType == type; }
  bool IsEOF() { return m_bEOF; }  
  

protected:
  DVDStreamType m_streamType;
  bool m_bEOF;
  char* m_strFileName;
};
