/*
 *  Copyright (C) 2005-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDMessageQueue.h"
#include "IVideoPlayer.h"
#include "Interface/TimingConstants.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <memory>
#include <string>
#include <vector>

#include <taglib/id3v1tag.h>
#include <taglib/id3v2tag.h>
#include <taglib/tstringlist.h>

namespace PVR
{
class CPVRChannel;
class CPVRRadioRDSInfoTag;
} // namespace PVR

class CVideoPlayerAudioID3 : public IDVDStreamPlayer, private CThread
{
public:
  explicit CVideoPlayerAudioID3(CProcessInfo& processInfo);
  ~CVideoPlayerAudioID3() override;

  bool CheckStream(const CDVDStreamInfo& hints);
  void Flush();
  void WaitForBuffers();

  bool OpenStream(CDVDStreamInfo hints) override;
  void CloseStream(bool bWaitForBuffers) override;
  void SendMessage(std::shared_ptr<CDVDMsg> pMsg, int priority = 0) override;
  void FlushMessages() override;
  bool IsInited() const override;
  bool AcceptsData() const override;
  bool IsStalled() const override;

protected:
  void OnExit() override;
  void Process() override;

private:
  void ResetCache();
  void ProcessID3(const unsigned char* data, unsigned int length) const;
  void ProcessID3v1(const TagLib::ID3v1::Tag* tag) const;
  void ProcessID3v2(const TagLib::ID3v2::Tag* tag) const;
  void ClearPlayingTitle(MUSIC_INFO::CMusicInfoTag* currentMusic) const;

  static std::vector<std::string> GetID3v2StringList(const TagLib::ID3v2::FrameList& frameList);
  static std::vector<std::string> StringListToVectorString(const TagLib::StringList& stringList);

  int m_speed = DVD_PLAYSPEED_NORMAL;
  CCriticalSection m_critSection;
  CDVDMessageQueue m_messageQueue;
  std::shared_ptr<PVR::CPVRRadioRDSInfoTag> m_currentRadiotext;
  std::shared_ptr<PVR::CPVRChannel> m_currentPVRChannel;

  const std::string m_cacheDir = "special://temp/audio_id3";
  mutable struct TOGGLE_DATA
  {
    int toogleId = 0;
    std::string lastExt;
    std::string lastMD5;
  } m_ImageTypeList[2];
};
