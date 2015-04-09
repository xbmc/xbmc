#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "DVDOverlayContainer.h"
#include "DVDSubtitles/DVDFactorySubtitle.h"
#include "DVDStreamInfo.h"
#include "DVDMessageQueue.h"
#include "DVDDemuxSPU.h"
#include "IDVDPlayer.h"

class CDVDInputStream;
class CDVDSubtitleStream;
class CDVDSubtitleParser;
class CDVDInputStreamNavigator;
class CDVDOverlayCodec;

class CDVDPlayerSubtitle : public IDVDStreamPlayer
{
public:
  CDVDPlayerSubtitle(CDVDOverlayContainer* pOverlayContainer);
  ~CDVDPlayerSubtitle();

  void Process(double pts, double offset);
  void Flush();
  void FindSubtitles(const char* strFilename);
  int GetSubtitleCount();

  void UpdateOverlayInfo(CDVDInputStreamNavigator* pStream, int iAction) { m_pOverlayContainer->UpdateOverlayInfo(pStream, &m_dvdspus, iAction); }

  bool AcceptsData() const;
  void SendMessage(CDVDMsg* pMsg, int priority = 0);
  void FlushMessages() {}
  bool OpenStream(CDVDStreamInfo &hints) { return OpenStream(hints, hints.filename); }
  bool OpenStream(CDVDStreamInfo &hints, std::string& filename);
  void CloseStream(bool bWaitForBuffers);

  bool IsInited() const { return true; }
  bool IsStalled() const { return m_pOverlayContainer->GetSize() == 0; }
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

