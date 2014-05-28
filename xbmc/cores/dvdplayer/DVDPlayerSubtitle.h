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

#ifndef DVDPLAYER_DVDOVERLAYCONTAINER_H_INCLUDED
#define DVDPLAYER_DVDOVERLAYCONTAINER_H_INCLUDED
#include "DVDOverlayContainer.h"
#endif

#ifndef DVDPLAYER_DVDSUBTITLES_DVDFACTORYSUBTITLE_H_INCLUDED
#define DVDPLAYER_DVDSUBTITLES_DVDFACTORYSUBTITLE_H_INCLUDED
#include "DVDSubtitles/DVDFactorySubtitle.h"
#endif

#ifndef DVDPLAYER_DVDSTREAMINFO_H_INCLUDED
#define DVDPLAYER_DVDSTREAMINFO_H_INCLUDED
#include "DVDStreamInfo.h"
#endif

#ifndef DVDPLAYER_DVDMESSAGEQUEUE_H_INCLUDED
#define DVDPLAYER_DVDMESSAGEQUEUE_H_INCLUDED
#include "DVDMessageQueue.h"
#endif

#ifndef DVDPLAYER_DVDDEMUXSPU_H_INCLUDED
#define DVDPLAYER_DVDDEMUXSPU_H_INCLUDED
#include "DVDDemuxSPU.h"
#endif


class CDVDInputStream;
class CDVDSubtitleStream;
class CDVDSubtitleParser;
class CDVDInputStreamNavigator;
class CDVDOverlayCodec;

class CDVDPlayerSubtitle
{
public:
  CDVDPlayerSubtitle(CDVDOverlayContainer* pOverlayContainer);
  ~CDVDPlayerSubtitle();

  void Process(double pts, double offset);
  void Flush();
  void FindSubtitles(const char* strFilename);
  int GetSubtitleCount();

  void UpdateOverlayInfo(CDVDInputStreamNavigator* pStream, int iAction) { m_pOverlayContainer->UpdateOverlayInfo(pStream, &m_dvdspus, iAction); }

  bool AcceptsData();
  void SendMessage(CDVDMsg* pMsg);
  bool OpenStream(CDVDStreamInfo &hints, std::string& filename);
  void CloseStream(bool flush);

  bool IsStalled() { return m_pOverlayContainer->GetSize() == 0; }
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

