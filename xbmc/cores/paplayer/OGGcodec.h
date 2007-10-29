#pragma once
#include "CachingCodec.h"
#include "FileSystem/File.h"
#include "DllVorbisfile.h"

class OGGCodec : public CachingCodec
{
public:
  OGGCodec();
  virtual ~OGGCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();

private:
  static size_t ReadCallback(void *ptr, size_t size, size_t nmemb, void *datasource);
  static int SeekCallback(void *datasource, ogg_int64_t offset, int whence);
  static int CloseCallback(void *datasource);
  static long TellCallback(void *datasource);

  void RemapChannels(short *SampleBuffer, int samples);

  DllVorbisfile m_dll;
  OggVorbis_File m_VorbisFile;
  double m_TimeOffset;
  int m_CurrentStream;
};
