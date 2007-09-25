
#pragma once

// include as less is possible to prevent dependencies
#include "DVDDemuxers/DVDDemux.h"
#include "DVDMessageTracker.h"

class CDVDMsg
{
public:
  enum Message
  {
    NONE = 1000,
    
    
    // messages used in the whole system
    
    GENERAL_RESYNC,                 // 
    GENERAL_FLUSH,                  // 
    GENERAL_STREAMCHANGE,           // 
    GENERAL_SYNCHRONIZE,            // 
    
    
    // player core related messages (cdvdplayer.cpp)
    
    PLAYER_SET_AUDIOSTREAM,         //
    PLAYER_SET_SUBTITLESTREAM,      //
    PLAYER_SET_SUBTITLESTREAM_VISIBLE, //
    PLAYER_SET_STATE,               // restore the dvdplayer to a certain state
    PLAYER_SEEK,                    // 
    PLAYER_SETSPEED,                // set the playback speed
    
    
    // demuxer related messages
    
    DEMUXER_PACKET,                 // data packet
    DEMUXER_RESET,                  // reset the demuxer
    
    
    // video related messages
    
    VIDEO_NOSKIP,                   // next pictures is not to be skipped by the video renderer
    VIDEO_SET_ASPECT,                // set aspectratio of video

    // subtitle related messages
    SUBTITLE_CLUTCHANGE,
  };
  
  CDVDMsg(Message msg)
  {
    m_references = 1;
    m_message = msg;
    
#ifdef DVDDEBUG_MESSAGE_TRACKER
    g_dvdMessageTracker.Register(this);
#endif
  }

  CDVDMsg(Message msg, long references)
  {
    m_references = references;
    m_message = msg;
    
#ifdef DVDDEBUG_MESSAGE_TRACKER
    g_dvdMessageTracker.Register(this);
#endif
  }
  
  virtual ~CDVDMsg()
  {
    assert(m_references == 0);
    
#ifdef DVDDEBUG_MESSAGE_TRACKER
    g_dvdMessageTracker.UnRegister(this);
#endif
  }

  /**
   * checks for message type
   */
  bool IsType(Message msg)
  {
    return (m_message == msg);
  }
  
  Message GetMessageType()
  {
    return m_message;
  }
  
  /**
   * decrease the reference counter by one.   
   */
  long Acquire()
  {
    long count = InterlockedIncrement(&m_references);
    return count;
  }
  
  /**
   * increase the reference counter by one.   
   */
  long Release()
  {
    long count = InterlockedDecrement(&m_references);
    if (count == 0) delete this;
    return count;
  }

  long GetNrOfReferences()
  {
    return m_references;
  }

private:
  long m_references;
  Message m_message;
};

////////////////////////////////////////////////////////////////////////////////
//////
////// GENERAL_ Messages
//////
////////////////////////////////////////////////////////////////////////////////

class CDVDMsgGeneralResync : public CDVDMsg
{
public:
  CDVDMsgGeneralResync(__int64 timestamp, bool clock) : CDVDMsg(GENERAL_RESYNC)  { m_timestamp = timestamp; m_clock = clock; }
  __int64 m_timestamp;
  bool m_clock;
};

class CDVDMsgGeneralFlush : public CDVDMsg
{
public:
  CDVDMsgGeneralFlush() : CDVDMsg(GENERAL_FLUSH)  {}
};

class CDVDStreamInfo;
class CDVDMsgGeneralStreamChange : public CDVDMsg
{
public:
  CDVDMsgGeneralStreamChange(CDVDStreamInfo* pInfo);
  virtual ~CDVDMsgGeneralStreamChange();
  CDVDStreamInfo* GetStreamInfo()       { return m_pInfo; }
private:
  CDVDStreamInfo* m_pInfo;
};

#define SYNCSOURCE_AUDIO  0x00000001
#define SYNCSOURCE_VIDEO  0x00000002
#define SYNCSOURCE_SUB    0x00000004
#define SYNCSOURCE_ALL    (SYNCSOURCE_AUDIO | SYNCSOURCE_VIDEO | SYNCSOURCE_SUB)

class CDVDMsgGeneralSynchronize : public CDVDMsg
{
public:
  CDVDMsgGeneralSynchronize(DWORD timeout, DWORD sources);

  // waits untill all threads is either waiting, released the object.
  // if abort is set somehow
  void Wait(volatile bool *abort, DWORD source); 
private:
  DWORD m_sources;
  long m_objects;
  unsigned int m_timeout;
};

template <typename T>
class CDVDMsgType : public CDVDMsg
{
public:
  CDVDMsgType(Message type, T value) 
    : CDVDMsg(type) 
    , m_value(value)
  {}
  operator T() { return m_value; }
  T m_value;
};

typedef CDVDMsgType<bool> CDVDMsgBool;
typedef CDVDMsgType<int> CDVDMsgInt;

////////////////////////////////////////////////////////////////////////////////
//////
////// PLAYER_ Messages
//////
////////////////////////////////////////////////////////////////////////////////

class CDVDMsgPlayerSetAudioStream : public CDVDMsg
{
public:
  CDVDMsgPlayerSetAudioStream(int streamId) : CDVDMsg(PLAYER_SET_AUDIOSTREAM) { m_streamId = streamId; }
  int GetStreamId()                     { return m_streamId; }
private:
  int m_streamId;
};

class CDVDMsgPlayerSetSubtitleStream : public CDVDMsg
{
public:
  CDVDMsgPlayerSetSubtitleStream(int streamId) : CDVDMsg(PLAYER_SET_SUBTITLESTREAM) { m_streamId = streamId; }
  int GetStreamId()                     { return m_streamId; }
private:
  int m_streamId;
};

class CDVDMsgPlayerSetState : public CDVDMsg
{
public:
  CDVDMsgPlayerSetState(std::string& state) : CDVDMsg(PLAYER_SET_STATE) { m_state = state; }
  std::string GetState()                { return m_state; }
private:
  std::string m_state;
};

class CDVDMsgPlayerSeek : public CDVDMsg
{
public:
  CDVDMsgPlayerSeek(int time, bool backword) : CDVDMsg(PLAYER_SEEK) { m_time = time; m_backword = backword; }
  int GetTime()                { return m_time; }
  bool GetBackword()          { return m_backword; }
private:
  int m_time;
  bool m_backword;
};

////////////////////////////////////////////////////////////////////////////////
//////
////// DEMUXER_ Messages
//////
////////////////////////////////////////////////////////////////////////////////

class CDVDMsgDemuxerPacket : public CDVDMsg
{
public:
  CDVDMsgDemuxerPacket(CDVDDemux::DemuxPacket* pPacket, unsigned int packetSize);
  virtual ~CDVDMsgDemuxerPacket();
  CDVDDemux::DemuxPacket* GetPacket()   { return m_pPacket; }
  unsigned int GetPacketSize()          { return m_packetSize; }
public: // XXX, test : should be private
  CDVDDemux::DemuxPacket* m_pPacket;
  unsigned int m_packetSize;
};

class CDVDMsgDemuxerReset : public CDVDMsg
{
public:
  CDVDMsgDemuxerReset() : CDVDMsg(DEMUXER_RESET)  {}
};

////////////////////////////////////////////////////////////////////////////////
//////
////// VIDEO_ Messages
//////
////////////////////////////////////////////////////////////////////////////////

class CDVDMsgVideoNoSkip : public CDVDMsg
{
public:
  CDVDMsgVideoNoSkip() : CDVDMsg(VIDEO_NOSKIP)  {}
};

class CDVDMsgVideoSetAspect : public CDVDMsg
{
public:
  CDVDMsgVideoSetAspect(float aspect) : CDVDMsg(VIDEO_SET_ASPECT) { m_aspect = aspect; }
  float GetAspect() { return m_aspect; }
private:
  float m_aspect;
};

////////////////////////////////////////////////////////////////////////////////
//////
////// SUBTITLE_ Messages
//////
////////////////////////////////////////////////////////////////////////////////

class CDVDMsgSubtitleClutChange : public CDVDMsg
{
public:
  CDVDMsgSubtitleClutChange(BYTE* data) : CDVDMsg(SUBTITLE_CLUTCHANGE) { memcpy(m_data, data, 16*4); }
  BYTE m_data[16][4];
private:
};
