
#pragma once

class CDVDInputStream;
enum CodecID;

enum StreamType
{
  STREAM_NONE,   // if unknown
  STREAM_AUDIO,  // audio stream
  STREAM_VIDEO,  // video stream
  STREAM_DATA   // data stream (eg. dvd spu's)
};

/*
 * CDemuxStream
 * Base class for all demuxer streams
 */
class CDemuxStream
{
public:
  CDemuxStream()
  {
    iId = 0;
    codec = (CodecID)0; // CODEC_ID_NONE
    type = STREAM_NONE;
    iDuration = 0;
    pPrivate = NULL;
  }

  virtual void GetStreamInfo(std::string& strInfo)
  {
    strInfo = "";
  }

  int iId; // file stream id
  CodecID codec;
  StreamType type;

  int iDuration; // in seconds
  void* pPrivate; // private pointer or the demuxer
};

class CDemuxStreamVideo : public CDemuxStream
{
public:
  CDemuxStreamVideo() : CDemuxStream()
  {
    iFpsScale = 0;
    iFpsRate = 0;
  }

  int iFpsScale; // scale of 1000 and a rate of 29970 will result in 29.97 fps
  int iFpsRate;
  int iHeight; // height of the stream reported by the demuxer
  int iWidth; // width of the stream reported by the demuxer
};

class CDemuxStreamAudio : public CDemuxStream
{
public:
  CDemuxStreamAudio() : CDemuxStream()
  {
    iChannels = 0;
    iSampleRate = 0;
  }

  int iChannels;
  int iSampleRate;
};

class CDVDDemux
{
public:

  typedef struct DemuxPacket
  {
    BYTE* pData; // data
    int iSize; // data size
    int iStreamId; // integer representing the stream index

    unsigned __int64 pts; // pts in DVD_TIME_BASE
    unsigned __int64 dts; // dts in DVD_TIME_BASE
  }
  DemuxPacket;

  /*
   * Open the demuxer, returns true on success
   */
  virtual bool Open(CDVDInputStream* pInput) = 0;
  /*
   * Dispose, Free all resources
   */
  virtual void Dispose() = 0;
  /*
   * Reset,
   */
  virtual void Reset() = 0;
  /*
   * Read a packet, returns NULL on error
   * 
   */
  virtual DemuxPacket* Read() = 0;
  /*
   * Seek, time in msec calculated from stream start
   */
  virtual bool Seek(int iTime) = 0;

  /*
   * returns the total time in msec
   */
  virtual int GetStreamLenght() = 0;
  /*
   * returns the stream or NULL on error, starting from 0
   */
  virtual CDemuxStream* GetStream(int iStreamId) = 0;
  /*
   * return nr of streams, 0 if none
   */
  virtual int GetNrOfStreams() = 0;
  /*
   * return the the iLogical'th stream of type iType, sorted by internal id
   */
  virtual int GetStreamNoFromLogicalNo(int iLogical, StreamType iType) = 0;

protected:
  CDVDInputStream* m_pInput;

  // global stream info
};
