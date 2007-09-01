
#pragma once

#include "DVDDemux.h"
#include "../../ffmpeg/DllAvFormat.h"
#include "../../ffmpeg/DllAvCodec.h"

class CDemuxStreamVideoFFmpeg : public CDemuxStreamVideo
{
public:
  CDemuxStreamVideoFFmpeg(DllAvCodec* pDll) : CDemuxStreamVideo()
  {
    m_pDll = pDll;
  }
  
  virtual void GetStreamInfo(std::string& strInfo);
  
private:
  DllAvCodec*  m_pDll;
};


class CDemuxStreamAudioFFmpeg : public CDemuxStreamAudio
{
public:
  CDemuxStreamAudioFFmpeg(DllAvCodec* pDll) : CDemuxStreamAudio()
  {
    m_pDll = pDll;
    previous_dts = 0LL;
  }

  virtual void GetStreamInfo(std::string& strInfo);
  __int64 previous_dts;
  
private:
  DllAvCodec*  m_pDll;
};

#define FFMPEG_FILE_BUFFER_SIZE   32768 // default reading size for ffmpeg
#define FFMPEG_DVDNAV_BUFFER_SIZE 2048  // for dvd's

class CDVDDemuxFFmpeg : public CDVDDemux
{
public:
  CDVDDemuxFFmpeg();
  virtual ~CDVDDemuxFFmpeg();

  bool Open(CDVDInputStream* pInput);
  void Dispose();
  void Reset();
  void Flush();
  void Abort();
  void SetSpeed(int iSpeed);

  CDVDDemux::DemuxPacket* Read();

  bool Seek(int iTime);
  int GetStreamLenght();
  CDemuxStream* GetStream(int iStreamId);
  int GetNrOfStreams();

  AVFormatContext* m_pFormatContext;

protected:
  int ReadFrame(AVPacket *packet);
  void AddStream(int iId);
  void Lock()   { EnterCriticalSection(&m_critSection); }
  void Unlock() { LeaveCriticalSection(&m_critSection); }

  bool ContextInit(const char* strFile, int chunksize, bool seekable);
  __int64 ConvertTimestamp(__int64 pts, int den, int num);

  CRITICAL_SECTION m_critSection;
  // #define MAX_STREAMS 42 // from avformat.h
  CDemuxStream* m_streams[MAX_STREAMS]; // maximum number of streams that ffmpeg can handle

  ByteIOContext m_ioContext;
  
  DllAvFormat m_dllAvFormat;
  DllAvCodec  m_dllAvCodec;
  DllAvUtil   m_dllAvUtil;
  
  __int64 m_iCurrentPts; // used for stream length estimation
  bool m_bDiscardDts;
  int m_speed;
};
