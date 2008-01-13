#ifndef DVDPLAYER_CODEC_H_
#define DVDPLAYER_CODEC_H_

#include "ICodec.h"
#include "FileSystem/File.h"

#include "DVDDemuxers/DVDDemux.h"
#include "DVDCodecs/Audio/DVDAudioCodec.h"
#include "DVDInputStreams/DVDInputStream.h"

class DVDPlayerCodec : public ICodec
{
public:
  DVDPlayerCodec();
  virtual ~DVDPlayerCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();
  virtual bool CanSeek();
  
private:
  __int64 m_iDataPos;  
  CDVDDemux* m_pDemuxer;
  CDVDInputStream* m_pInputStream;
  CDVDAudioCodec* m_pAudioCodec;
  int m_nAudioStream;
  
  int m_audioLen;
  int m_audioPos;
  BYTE* m_audioData;
};

#endif
