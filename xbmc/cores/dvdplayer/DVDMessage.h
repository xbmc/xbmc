#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

// include as less is possible to prevent dependencies
#include "DVDDemuxers/DVDDemux.h"
#include "DVDMessageTracker.h"

#include <assert.h>

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
    GENERAL_DELAY,                  //
    GENERAL_GUI_ACTION,             // gui action of some sort
    GENERAL_EOF,                    // eof of stream
    
    
    // player core related messages (cdvdplayer.cpp)
    
    PLAYER_SET_AUDIOSTREAM,         //
    PLAYER_SET_SUBTITLESTREAM,      //
    PLAYER_SET_SUBTITLESTREAM_VISIBLE, //
    PLAYER_SET_STATE,               // restore the dvdplayer to a certain state
    PLAYER_SET_RECORD,              // set record state
    PLAYER_SEEK,                    // 
    PLAYER_SEEK_CHAPTER,            //
    PLAYER_SETSPEED,                // set the playback speed

    PLAYER_CHANNEL_NEXT,            // switches to next playback channel
    PLAYER_CHANNEL_PREV,            // switches to previous playback channel
    
    // demuxer related messages
    
    DEMUXER_PACKET,                 // data packet
    DEMUXER_RESET,                  // reset the demuxer
    
    
    // video related messages
    
    VIDEO_NOSKIP,                   // next pictures is not to be skipped by the video renderer
    VIDEO_SET_ASPECT,               // set aspectratio of video

    // subtitle related messages
    SUBTITLE_CLUTCHANGE
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
  CDVDMsgGeneralResync(double timestamp, bool clock) : CDVDMsg(GENERAL_RESYNC)  { m_timestamp = timestamp; m_clock = clock; }
  double m_timestamp;
  bool m_clock;
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

  // waits until all threads waiting, released the object
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

typedef CDVDMsgType<bool>   CDVDMsgBool;
typedef CDVDMsgType<int>    CDVDMsgInt;
typedef CDVDMsgType<double> CDVDMsgDouble;

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
  CDVDMsgPlayerSeek(int time, bool backward, bool flush = true, bool accurate = true)
    : CDVDMsg(PLAYER_SEEK)
    , m_time(time)
    , m_backward(backward)
    , m_flush(flush)
    , m_accurate(accurate)
  {}
  int  GetTime()              { return m_time; }
  bool GetBackward()          { return m_backward; }
  bool GetFlush()             { return m_flush; }
  bool GetAccurate()          { return m_accurate; }
private:
  int  m_time;
  bool m_backward;
  bool m_flush;
  bool m_accurate;
};

class CDVDMsgPlayerSeekChapter : public CDVDMsg
{
  public:
    CDVDMsgPlayerSeekChapter(int iChapter)
      : CDVDMsg(PLAYER_SEEK_CHAPTER)
      , m_iChapter(iChapter)
    {}
    
    int GetChapter() const { return m_iChapter; }
    
  private:
    
    int m_iChapter;
};

////////////////////////////////////////////////////////////////////////////////
//////
////// DEMUXER_ Messages
//////
////////////////////////////////////////////////////////////////////////////////

class CDVDMsgDemuxerPacket : public CDVDMsg
{
public:
  CDVDMsgDemuxerPacket(DemuxPacket* packet, bool drop = false);
  virtual ~CDVDMsgDemuxerPacket();
  DemuxPacket* GetPacket()      { return m_packet; }
  unsigned int GetPacketSize()  { if(m_packet) return m_packet->iSize; else return 0; }
  bool         GetPacketDrop()  { return m_drop; }
private:
  DemuxPacket* m_packet;
  bool         m_drop;
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
