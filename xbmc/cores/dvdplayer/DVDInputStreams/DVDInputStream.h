
#pragma once

enum DVDStreamType
{
  DVDSTREAM_TYPE_NONE   = -1,
  DVDSTREAM_TYPE_FILE   = 1,
  DVDSTREAM_TYPE_DVD    = 2,
  DVDSTREAM_TYPE_HTTP   = 3,
  DVDSTREAM_TYPE_MEMORY = 4,
  DVDSTREAM_TYPE_FFMPEG = 5,
};

#define DVDSTREAM_BLOCK_SIZE_FILE (2048 * 16)
#define DVDSTREAM_BLOCK_SIZE_DVD  2048

class CDVDInputStream
{
public:
  CDVDInputStream(DVDStreamType m_streamType);
  virtual ~CDVDInputStream();
  virtual bool Open(const char* strFileName, const std::string& content) = 0;
  virtual void Close() = 0;
  virtual int Read(BYTE* buf, int buf_size) = 0;
  virtual __int64 Seek(__int64 offset, int whence) = 0;
  virtual __int64 GetLength() = 0;
  virtual std::string GetContent() { return m_content; };

  const char* GetFileName();
  bool HasExtension(char* sExtension);
  
  int GetBlockSize() { return DVDSTREAM_BLOCK_SIZE_FILE; }
  bool IsStreamType(DVDStreamType type) const { return m_streamType == type; }
  virtual bool IsEOF() = 0;  
  virtual int GetCurrentGroupId() { return 0; }

protected:
  DVDStreamType m_streamType;
  char* m_strFileName;
  std::string m_content;
};
