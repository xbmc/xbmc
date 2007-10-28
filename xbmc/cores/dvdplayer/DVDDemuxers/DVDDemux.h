
#pragma once

class CDVDInputStream;
enum CodecID;

enum StreamType
{
  STREAM_NONE,    // if unknown
  STREAM_AUDIO,   // audio stream
  STREAM_VIDEO,   // video stream
  STREAM_DATA,    // data stream
  STREAM_SUBTITLE // subtitle stream
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
    iPhysicalId = 0;
    codec = (CodecID)0; // CODEC_ID_NONE
    type = STREAM_NONE;
    iDuration = 0;
    pPrivate = NULL;
    ExtraData = NULL;
    ExtraSize = 0;
    language[0] = 0;
    disabled = false;
  }

  virtual void GetStreamInfo(std::string& strInfo)
  {
    strInfo = "";
  }

  virtual void GetStreamName(CStdString& strInfo);

  int iId;         // most of the time starting from 0
  int iPhysicalId; // id
  CodecID codec;
  StreamType type;

  int iDuration; // in mseconds
  void* pPrivate; // private pointer for the demuxer
  void* ExtraData; // extra data for codec to use
  unsigned int ExtraSize; // size of extra data

  char language[4]; // ISO 639 3-letter language code (empty string if undefined)
  bool disabled; // set when stream is disabled. (when no decoder exists)
};

class CDemuxStreamVideo : public CDemuxStream
{
public:
  CDemuxStreamVideo() : CDemuxStream()
  {
    iFpsScale = 0;
    iFpsRate = 0;
    iHeight = 0;
    iWidth = 0;
    fAspect = 0.0;
    type = STREAM_VIDEO;
  }

  int iFpsScale; // scale of 1000 and a rate of 29970 will result in 29.97 fps
  int iFpsRate;
  int iHeight; // height of the stream reported by the demuxer
  int iWidth; // width of the stream reported by the demuxer
  float fAspect; // display aspect of stream
};

class CDemuxStreamAudio : public CDemuxStream
{
public:
  CDemuxStreamAudio() : CDemuxStream()
  {
    iChannels = 0;
    iSampleRate = 0;
    type = STREAM_AUDIO;
  }

  void GetStreamType(std::string& strInfo);

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
    int iGroupId; // the group this data belongs to, used to group data from different streams together
    
    double pts; // pts in DVD_TIME_BASE
    double dts; // dts in DVD_TIME_BASE
  }
  DemuxPacket;

  CDVDDemux() {}
  virtual ~CDVDDemux() {}
  
  /*
   * Open the demuxer, returns true on success
   */
  virtual bool Open(CDVDInputStream* pInput) = 0;
  
  /*
   * Dispose, Free all resources
   */
  virtual void Dispose() = 0;
  
  /*
   * Reset the entire demuxer (same result as closing and opening it)
   */
  virtual void Reset() = 0;
  
  /*
   * Aborts any internal reading that might be stalling main thread
   * NOTICE - this can be called from another thread
   */
  virtual void Abort() = 0;

  /*
   * Flush the demuxer, if any data is kept in buffers, this should be freed now
   */
  virtual void Flush() = 0;
  
  /*
   * Read a packet, returns NULL on error
   * 
   */
  virtual DemuxPacket* Read() = 0;
  
  /*
   * Seek, time in msec calculated from stream start
   */
  virtual bool Seek(int iTime, bool bBackword = false) = 0;

  /*
   * Set the playspeed, if demuxer can handle different
   * speeds of playback
   */
  virtual void SetSpeed(int iSpeed) = 0;

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
   * return nr of audio streams, 0 if none
   */
  int GetNrOfAudioStreams();
  
  /*
   * return nr of video streams, 0 if none
   */
  int GetNrOfVideoStreams();
  
  /*
   * return nr of subtitle streams, 0 if none
   */
  int GetNrOfSubtitleStreams();

  /*
   * return the audio stream, or NULL if it does not exist
   */
  CDemuxStreamAudio* GetStreamFromAudioId(int iAudioIndex);

  /*
   * return the video stream, or NULL if it does not exist
   */
  CDemuxStreamVideo* GetStreamFromVideoId(int iVideoIndex);
  
  /*
   * return the subtitle stream, or NULL if it does not exist
   */
  CDemuxStream* GetStreamFromSubtitleId(int iSubtitleIndex);
  
protected:
  CDVDInputStream* m_pInput;

  // global stream info
};
