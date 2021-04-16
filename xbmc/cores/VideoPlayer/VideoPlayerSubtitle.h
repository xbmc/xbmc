/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDDemuxSPU.h"
#include "DVDMessageQueue.h"
#include "DVDOverlayContainer.h"
#include "DVDStreamInfo.h"
#include "DVDSubtitles/DVDFactorySubtitle.h"
#include "IVideoPlayer.h"

class CDVDInputStream;
class CDVDSubtitleStream;
class CDVDSubtitleParser;
class CDVDInputStreamNavigator;
class CDVDOverlayCodec;

class CVideoPlayerSubtitle : public IDVDStreamPlayer
{
public:
  CVideoPlayerSubtitle(CDVDOverlayContainer* pOverlayContainer, CProcessInfo &processInfo);
  ~CVideoPlayerSubtitle() override;

  void Process(double pts, double offset);
  void Flush();
  void FindSubtitles(const char* strFilename);
  int GetSubtitleCount();

  void UpdateOverlayInfo(const std::shared_ptr<CDVDInputStreamNavigator>& pStream, int iAction)
  {
    m_pOverlayContainer->UpdateOverlayInfo(pStream, &m_dvdspus, iAction);
  }

  bool AcceptsData() const override;
  void SendMessage(CDVDMsg* pMsg, int priority = 0) override;
  void FlushMessages() override {}
  bool OpenStream(CDVDStreamInfo hints) override { return OpenStream(hints, hints.filename); }
  bool OpenStream(CDVDStreamInfo &hints, std::string& filename);
  void CloseStream(bool bWaitForBuffers) override;

  bool IsInited() const override { return true; }
  bool IsStalled() const override { return m_pOverlayContainer->GetSize() == 0; }
private:
  CDVDOverlayContainer* m_pOverlayContainer;

  CDVDSubtitleStream* m_pSubtitleStream;
  CDVDSubtitleParser* m_pSubtitleFileParser;
  CDVDOverlayCodec*   m_pOverlayCodec;
  CDVDDemuxSPU        m_dvdspus;

  CDVDStreamInfo      m_streaminfo;
  double              m_lastPts;


  CCriticalSection    m_section;
};


//typedef struct SubtitleInfo
//{

//
//} SubtitleInfo;

