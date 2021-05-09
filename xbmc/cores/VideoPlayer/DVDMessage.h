/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItem.h"
#include "cores/IPlayer.h"

#include <atomic>
#include <string.h>
#include <string>

struct DemuxPacket;

class CDVDMsg
{
public:
  // clang-format off
  enum Message
  {
    NONE = 1000,

    // messages used in the whole system
    GENERAL_RESYNC,                 //
    GENERAL_FLUSH,                  // flush all buffers
    GENERAL_RESET,                  // reset codecs for new data
    GENERAL_PAUSE,
    GENERAL_STREAMCHANGE,           //
    GENERAL_SYNCHRONIZE,            //
    GENERAL_GUI_ACTION,             // gui action of some sort
    GENERAL_EOF,                    // eof of stream

    // player core related messages (cVideoPlayer.cpp)
    PLAYER_SET_AUDIOSTREAM,         //
    PLAYER_SET_VIDEOSTREAM,         //
    PLAYER_SET_SUBTITLESTREAM,      //
    PLAYER_SET_SUBTITLESTREAM_VISIBLE, //
    PLAYER_SET_STATE,               // restore the VideoPlayer to a certain state
    PLAYER_SET_PROGRAM,
    PLAYER_SET_UPDATE_STREAM_DETAILS, // player should update file item stream details with its current streams
    PLAYER_SEEK,                    //
    PLAYER_SEEK_CHAPTER,            //
    PLAYER_SETSPEED,                // set the playback speed
    PLAYER_REQUEST_STATE,
    PLAYER_OPENFILE,
    PLAYER_STARTED,                 // sent whenever a sub player has finished it's first frame after open
    PLAYER_AVCHANGE,                // signal a change in audio, video or subtitle parameters
    PLAYER_ABORT,
    PLAYER_REPORT_STATE,
    PLAYER_FRAME_ADVANCE,
    PLAYER_DISPLAY_RESET,           // report display reset event

    // demuxer related messages
    DEMUXER_PACKET,                 // data packet
    DEMUXER_RESET,                  // reset the demuxer

    // video related messages
    VIDEO_SET_ASPECT,               // set aspectratio of video
    VIDEO_DRAIN,                    // wait for decoder to output last frame

    // subtitle related messages
    SUBTITLE_CLUTCHANGE,
    SUBTITLE_ADDFILE
  };
  // clang-format on

  explicit CDVDMsg(Message msg)
  {
    m_message = msg;
  }

  virtual ~CDVDMsg() = default;

  /**
   * checks for message type
   */
  inline bool IsType(Message msg)
  {
    return (m_message == msg);
  }

  inline Message GetMessageType()
  {
    return m_message;
  }

private:
  Message m_message;
};

////////////////////////////////////////////////////////////////////////////////
//////
////// GENERAL_ Messages
//////
////////////////////////////////////////////////////////////////////////////////

#define SYNCSOURCE_AUDIO  0x01
#define SYNCSOURCE_VIDEO  0x02
#define SYNCSOURCE_PLAYER 0x04
#define SYNCSOURCE_ANY    0x08

class CDVDMsgGeneralSynchronizePriv;
class CDVDMsgGeneralSynchronize : public CDVDMsg
{
public:
  CDVDMsgGeneralSynchronize(unsigned int timeout, unsigned int sources);
 ~CDVDMsgGeneralSynchronize() override;

  // waits until all threads waiting, released the object
  // if abort is set somehow
  bool Wait(unsigned int ms, unsigned int source);
  void Wait(std::atomic<bool>& abort, unsigned int source);

private:
  class CDVDMsgGeneralSynchronizePriv* m_p;
};

template <typename T>
class CDVDMsgType : public CDVDMsg
{
public:
  CDVDMsgType(Message type, const T &value)
    : CDVDMsg(type)
    , m_value(value)
  {}

  ~CDVDMsgType() override = default;

  operator T() { return m_value; }
  T m_value;
};

typedef CDVDMsgType<bool> CDVDMsgBool;
typedef CDVDMsgType<int> CDVDMsgInt;
typedef CDVDMsgType<double> CDVDMsgDouble;

////////////////////////////////////////////////////////////////////////////////
//////
////// PLAYER_ Messages
//////
////////////////////////////////////////////////////////////////////////////////

class CDVDMsgPlayerSetAudioStream : public CDVDMsg
{
public:
  explicit CDVDMsgPlayerSetAudioStream(int streamId) : CDVDMsg(PLAYER_SET_AUDIOSTREAM) { m_streamId = streamId; }
  ~CDVDMsgPlayerSetAudioStream() override = default;

  int GetStreamId() { return m_streamId; }
private:
  int m_streamId;
};

class CDVDMsgPlayerSetVideoStream : public CDVDMsg
{
public:
  explicit CDVDMsgPlayerSetVideoStream(int streamId) : CDVDMsg(PLAYER_SET_VIDEOSTREAM) { m_streamId = streamId; }
  ~CDVDMsgPlayerSetVideoStream() override = default;

  int GetStreamId() const { return m_streamId; }
private:
  int m_streamId;
};

class CDVDMsgPlayerSetSubtitleStream : public CDVDMsg
{
public:
  explicit CDVDMsgPlayerSetSubtitleStream(int streamId) : CDVDMsg(PLAYER_SET_SUBTITLESTREAM) { m_streamId = streamId; }
  ~CDVDMsgPlayerSetSubtitleStream() override = default;

  int GetStreamId() { return m_streamId; }
private:
  int m_streamId;
};

class CDVDMsgPlayerSetState : public CDVDMsg
{
public:
  explicit CDVDMsgPlayerSetState(const std::string& state) : CDVDMsg(PLAYER_SET_STATE), m_state(state) {}
  ~CDVDMsgPlayerSetState() override = default;

  std::string GetState() { return m_state; }
private:
  std::string m_state;
};

class CDVDMsgPlayerSeek : public CDVDMsg
{
public:
  struct CMode
  {
    double time = 0;
    bool relative = false;
    bool backward = false;
    bool accurate = true;
    bool sync = true;
    bool restore = true;
    bool trickplay = false;
  };

  explicit CDVDMsgPlayerSeek(CDVDMsgPlayerSeek::CMode mode) : CDVDMsg(PLAYER_SEEK),
    m_mode(mode)
  {}
  ~CDVDMsgPlayerSeek() override = default;

  double GetTime() { return m_mode.time; }
  bool GetRelative() { return m_mode.relative; }
  bool GetBackward() { return m_mode.backward; }
  bool GetAccurate() { return m_mode.accurate; }
  bool GetRestore() { return m_mode.restore; }
  bool GetTrickPlay() { return m_mode.trickplay; }
  bool GetSync() { return m_mode.sync; }

private:
  CMode m_mode;
};

class CDVDMsgPlayerSeekChapter : public CDVDMsg
{
  public:
    explicit CDVDMsgPlayerSeekChapter(int iChapter)
      : CDVDMsg(PLAYER_SEEK_CHAPTER)
      , m_iChapter(iChapter)
    {}
    ~CDVDMsgPlayerSeekChapter() override = default;

    int GetChapter() const { return m_iChapter; }

  private:

    int m_iChapter;
};

class CDVDMsgPlayerSetSpeed : public CDVDMsg
{
public:
  struct SpeedParams
  {
    int m_speed;
    bool m_isTempo;
  };

  explicit CDVDMsgPlayerSetSpeed(SpeedParams params)
  : CDVDMsg(PLAYER_SETSPEED)
  , m_params(params)
  {}
  ~CDVDMsgPlayerSetSpeed() override = default;

  int GetSpeed() const { return m_params.m_speed; }
  bool IsTempo() const { return m_params.m_isTempo; }

private:

  SpeedParams m_params;

};

class CDVDMsgOpenFile : public CDVDMsg
{
public:
  struct FileParams
  {
    CFileItem m_item;
    CPlayerOptions m_options;
  };

  explicit CDVDMsgOpenFile(const FileParams &params)
  : CDVDMsg(PLAYER_OPENFILE)
  , m_params(params)
  {}
  ~CDVDMsgOpenFile() override = default;

  CFileItem& GetItem() { return m_params.m_item; }
  CPlayerOptions& GetOptions() { return m_params.m_options; }

private:

  FileParams m_params;
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
  ~CDVDMsgDemuxerPacket() override;
  DemuxPacket* GetPacket() { return m_packet; }
  unsigned int GetPacketSize();
  bool GetPacketDrop() { return m_drop; }
  DemuxPacket* m_packet;
  bool m_drop;
};

class CDVDMsgDemuxerReset : public CDVDMsg
{
public:
  CDVDMsgDemuxerReset() : CDVDMsg(DEMUXER_RESET)  {}
  ~CDVDMsgDemuxerReset() override = default;
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
  explicit CDVDMsgSubtitleClutChange(uint8_t* data) : CDVDMsg(SUBTITLE_CLUTCHANGE) { memcpy(m_data, data, 16*4); }
  ~CDVDMsgSubtitleClutChange() override = default;

  uint8_t m_data[16][4];
};
