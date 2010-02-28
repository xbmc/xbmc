/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#define DVD_NOPTS_VALUE    (-1LL<<52) // should be possible to represent in both double and __int64
#define DVD_TIME_BASE 1000000

#include "pvrclient-tvheadend.h"

const size_t CIRCULAR_BUFFER_SIZE = 1024 * 1024;

/************************************************************/
/** Class interface */

cPVRClientTvheadend::cPVRClientTvheadend()
{
  m_pSession = NULL;
}

cPVRClientTvheadend::~cPVRClientTvheadend() {
  Disconnect();
}

/* connect functions */
bool cPVRClientTvheadend::Connect(std::string sHostname, int iPort)
{
  m_url.SetHostName(sHostname);
  m_url.SetPort(iPort);

  if (m_pSession)
    Disconnect();

  /* connect session */
  if (!(m_pSession = CHTSPDirectorySession::Acquire(m_url)))  //if (!m_cSession.Connect(m_sHostname, m_iPort))
    return false;

  return true;
}

void cPVRClientTvheadend::Disconnect()
{
  CHTSPDirectorySession::Release(m_pSession);
  m_pSession = NULL;
}

PVR_ERROR cPVRClientTvheadend::GetBackendTime(time_t *localTime, int *gmtOffset)
{
  if (!IsConnected() || !m_pSession->GetTime(localTime, gmtOffset))
    return PVR_ERROR_SERVER_ERROR;
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientTvheadend::RequestChannelList(PVRHANDLE handle, bool radio)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  if (radio)
    return PVR_ERROR_NO_ERROR;

  SChannels channels = m_pSession->GetChannels();
  STags     tags     = m_pSession->GetTags();

  for(STags::iterator tit = tags.begin(); tit != tags.end(); ++tit)
  {
    STag& t = tit->second;
    PVR_BOUQUET bou;
    bou.Name = (char*)t.name.c_str();
    bou.Category = "";
    bou.Number = t.id;

    for(SChannels::iterator it = channels.begin(); it != channels.end(); ++it) {
      SChannel& channel = it->second;

      if (channel.MemberOf(t.id))
      {
        PVR_CHANNEL tag;
        memset(&tag, 0 , sizeof(tag));
        tag.uid         = channel.id;
        tag.number      = channel.id;//num;
        tag.name        = channel.name.c_str();
        tag.callsign    = channel.name.c_str();
        tag.input_format = "";

        char url[128];
        sprintf(url, "htsp://%s:%d/tags/0/%d.ts",
            m_url.GetHostName().c_str(), m_url.GetPort(), channel.id);
        tag.stream_url  = url;
        tag.bouquet     = t.id;

        XBMC_log(LOG_DEBUG, "%s - %s",
            __PRETTY_FUNCTION__, channel.name.c_str());

        PVR_transfer_channel_entry(handle, &tag);
      }
    }
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientTvheadend::RequestEPGForChannel(PVRHANDLE handle, const PVR_CHANNEL &channel, time_t start, time_t end)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  SChannels channels = m_pSession->GetChannels();

  if (channels.find(channel.uid) != channels.end())
  {
    time_t stop;

    SEvent event;
    event.id = channels[channel.uid].event;

    printf("channel.uid %d, channel.id %d, event.id %d\n", channel.uid, channels[channel.uid].id, event.id);

    if (event.id == 0)
      return PVR_ERROR_NO_ERROR;

    do {

      bool success = m_pSession->GetEvent(event, event.id);

      if (success){
//        XBMC_log(LOG_DEBUG, "%s - uid %d, title %s, desc %s, start %d, stop %d",
//            __PRETTY_FUNCTION__, event.id, event.title.c_str(), event.descs.c_str(), event.start, event.stop);
//        printf("%s - uid %d, title %s, desc %s, start %d, stop %d\n",
//            __PRETTY_FUNCTION__, event.id, event.title.c_str(), event.descs.c_str(), event.start, event.stop);

        PVR_PROGINFO broadcast;
        memset(&broadcast, 0, sizeof(PVR_PROGINFO));

        broadcast.channum         = event.chan_id >= 0 ? event.chan_id : channel.number;
        broadcast.uid             = event.id;
        broadcast.title           = event.title.c_str();
        broadcast.subtitle        = event.title.c_str();
        broadcast.description     = event.descs.c_str();
        broadcast.starttime       = event.start;
        broadcast.endtime         = event.stop;
        broadcast.genre_type      = event.content;
        broadcast.genre_sub_type  = 0;
        PVR_transfer_epg_entry(handle, &broadcast);

        event.id = event.next;
        stop = event.stop;
      } else
        break;

    } while(end > stop);

    return PVR_ERROR_NO_ERROR;
  }

  return PVR_ERROR_NO_ERROR;
}

/************************************************************/
/** Server handling */

PVR_ERROR cPVRClientTvheadend::GetProperties(PVR_SERVERPROPS *props) {
  props->SupportChannelLogo        = true;
  props->SupportTimeShift          = false;
  props->SupportEPG                = true;
  props->SupportRecordings         = true;
  props->SupportTimers             = true;
  props->SupportTV                 = true;
  props->SupportRadio              = true;
  props->SupportChannelSettings    = true;
  props->SupportDirector           = false;
  props->SupportBouquets           = true;
  props->HandleInputStream         = true;
  props->HandleDemuxing            = true;
  props->SupportChannelScan        = false;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientTvheadend::GetStreamProperties(PVR_STREAMPROPS *props) {
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;
#if 0
  props->nstreams = sl.size();

  std::list<htsp_stream_t>::iterator it;
  int i;

  for (i = 0, it = sl.begin(); it != sl.end(); ++i, ++it)
  {
    props->stream[i].id         = it->id;
    props->stream[i].physid     = it->physid;
    props->stream[i].codec_type = it->codec_type;
    props->stream[i].codec_id   = it->codec_id;
  }
#endif
  return PVR_ERROR_NO_ERROR;
}

bool cPVRClientTvheadend::SwitchChannel(const PVR_CHANNEL &channelinfo)
{
  return OpenLiveStream(channelinfo);
}

/************************************************************/
/** Live stream handling */
bool cPVRClientTvheadend::OpenLiveStream(const PVR_CHANNEL &channelinfo)
{
  return true;
}

void cPVRClientTvheadend::CloseLiveStream()
{
}

int cPVRClientTvheadend::ReadLiveStream(unsigned char* buf, int buf_size)
{
  return -1;
}

