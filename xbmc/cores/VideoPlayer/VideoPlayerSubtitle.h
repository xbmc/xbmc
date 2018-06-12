/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "DVDOverlayContainer.h"
#include "DVDSubtitles/DVDFactorySubtitle.h"
#include "DVDStreamInfo.h"
#include "DVDMessageQueue.h"
#include "DVDDemuxSPU.h"
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

  void UpdateOverlayInfo(std::shared_ptr<CDVDInputStreamNavigator> pStream, int iAction) { m_pOverlayContainer->UpdateOverlayInfo(pStream, &m_dvdspus, iAction); }

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

