
#pragma once

#include "DVDDemux.h"

// XXX, remove ffmpeg dependency
#include "..\ffmpeg\ffmpeg.h"

struct AVFormatContext;
struct AVPacket;
struct AVStream;

class CDemuxStreamVideoFFmpeg : public CDemuxStreamVideo
{
public:
  virtual void GetStreamInfo(std::string& strInfo);
};

class CDemuxStreamAudioFFmpeg : public CDemuxStreamAudio
{
public:
  virtual void GetStreamInfo(std::string& strInfo);
};

#define FFMPEG_FILE_BUFFER_SIZE   32768 // default reading size for ffmpeg
#define FFMPEG_DVDNAV_BUFFER_SIZE 2048  // for dvd's

class CDVDDemuxFFmpeg : public CDVDDemux
{
public:
  CDVDDemuxFFmpeg();
  ~CDVDDemuxFFmpeg();

  bool Open(CDVDInputStream* pInput);
  void Dispose();
  void Reset();

  CDVDDemux::DemuxPacket* Read();

  bool Seek(int iTime);
  int GetStreamLenght();
  CDemuxStream* GetStream(int iStreamId);
  int GetNrOfStreams();
  int GetStreamNoFromLogicalNo(int iLogical, StreamType iType);

  AVFormatContext* m_pFormatContext;

protected:
  int ReadFrame(AVPacket *packet);
  void AddStream(int iId);
  void Lock();
  void Unlock();

  bool ContextInit(const char* strFile, BYTE* buffer, int iBufferSize);
  void ContextDeInit();

  CRITICAL_SECTION m_critSection;
  // #define MAX_STREAMS 20 // from avformat.h
  CDemuxStream* m_streams[20]; // maximum number of streams that ffmpeg can handle

  BYTE m_ffmpegBuffer[FFMPEG_FILE_BUFFER_SIZE];
  ByteIOContext m_ioContext;
  URLContext* m_pUrlContext;
};
