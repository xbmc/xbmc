#pragma once

/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "client.h"
#include "HTSPSession.h"

class cHTSPDemux
{
public:
  cHTSPDemux();
  ~cHTSPDemux();

  bool Open(const PVR_CHANNEL &channelinfo);
  void Close();
  bool GetStreamProperties(PVR_STREAM_PROPERTIES* props);
  void Abort();
  DemuxPacket* Read();
  bool SwitchChannel(const PVR_CHANNEL &channelinfo);
  int CurrentChannel() { return m_channel; }
  bool GetSignalStatus(PVR_SIGNAL_STATUS &qualityinfo);

protected:
  void SubscriptionStart (htsmsg_t *m);
  void SubscriptionStop  (htsmsg_t *m);
  void SubscriptionStatus(htsmsg_t *m);
  DemuxPacket *ParseMuxPacket(htsmsg_t *m);

private:
  void SetLanguageInfo(const char *strLanguage, char *strDestination);

  unsigned              m_subs;
  cHTSPSession          m_session;
  int                   m_channel;
  int                   m_tag;
  int                   m_StatusCount;
  std::string           m_Status;
  PVR_STREAM_PROPERTIES m_Streams;
  SChannels             m_channels;
  SQueueStatus          m_QueueStatus;
  SQuality              m_Quality;
  SSourceInfo           m_SourceInfo;
};
