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

#include "client.h"
#include "timers.h"
#include "channels.h"
#include "recordings.h"
#include "epg.h"
#include "pvrclient-vdr.h"
#include "pvrclient-vdr_os.h"

#ifdef _LINUX
#define SD_BOTH SHUT_RDWR
#endif

#define SEEK_POSSIBLE 0x10 // flag used to check if protocol allows seeks

using namespace std;

pthread_mutex_t m_critSection;

bool PVRClientVDR::m_bStop						= true;
SOCKET PVRClientVDR::m_socket_data				= INVALID_SOCKET;
SOCKET PVRClientVDR::m_socket_video				= INVALID_SOCKET;
CVTPTransceiver *PVRClientVDR::m_transceiver    = NULL;
bool PVRClientVDR::m_bConnected					= false;

/************************************************************/
/** Class interface */

PVRClientVDR::PVRClientVDR()
{
  m_iCurrentChannel   = 1;
  m_transceiver       = new CVTPTransceiver();
  m_bConnected        = false;
  m_socket_video      = INVALID_SOCKET;
  m_socket_data       = INVALID_SOCKET;
  m_bStop             = true;

  pthread_mutex_init(&m_critSection, NULL);
}

PVRClientVDR::~PVRClientVDR()
{
  Disconnect();
}


/************************************************************/
/** Server handling */

PVR_ERROR PVRClientVDR::GetProperties(PVR_SERVERPROPS *props)
{
  props->SupportChannelLogo        = false;
  props->SupportTimeShift          = false;
  props->SupportEPG                = true;
  props->SupportRecordings         = true;
  props->SupportTimers             = true;
  props->SupportRadio              = true;
  props->SupportChannelSettings    = true;
  props->SupportTeletext           = false;
  props->SupportDirector           = false;
  props->SupportBouquets           = false;

  return PVR_ERROR_NO_ERROR;
}

bool PVRClientVDR::Connect()
{
  /* Open Streamdev-Server VTP-Connection to VDR Backend Server */
  if (!m_transceiver->Open(m_sHostname, m_iPort))
    return false;

  /* Check VDR streamdev is patched by calling a newly added command */
  if (GetNumChannels() == -1)
  {
    XBMC_log(LOG_ERROR, "PCRClient-vdr: Detected unsupported Streamdev-Version");
    return false;
  }

  /* Get Data socket from VDR Backend */
  m_socket_data = m_transceiver->GetStreamData();

  /* If received socket is invalid, return */
  if (m_socket_data == INVALID_SOCKET)
  {
    XBMC_log(LOG_ERROR, "PCRClient-vdr: Couldn't get socket for data response");
    return false;
  }

  /* Start VTP Listening Thread */
//  m_bStop = false;
//  if (pthread_create(&m_thread, NULL, &Process, (void *)"PVRClientVDR VTP-Listener") != 0) {
//    return false;
//  }

  m_bConnected = true;
  return true;
}

void PVRClientVDR::Disconnect()
{
  m_bStop = true;
//  pthread_join(m_thread, NULL);

  m_bConnected = false;

  /* Check if  stream sockets are open, if yes close them */
  if (m_socket_data != INVALID_SOCKET)
  {
    m_transceiver->AbortStreamData();
    closesocket(m_socket_data);
    m_socket_data = INVALID_SOCKET;
  }

  if (m_socket_video != INVALID_SOCKET)
  {
    m_transceiver->AbortStreamLive();
    closesocket(m_socket_video);
    m_socket_video = INVALID_SOCKET;
  }

  /* Close Streamdev-Server VTP Backend Connection */
  m_transceiver->Close();
}

bool PVRClientVDR::IsUp()
{
  if (m_bConnected || m_transceiver->IsOpen())
  {
    return true;
  }
  return false;
}

void* PVRClientVDR::Process(void*)
{
  char   		 data[1024];
  fd_set         set_r, set_e;
  struct timeval tv;
  int            res;

  while (!m_bStop)
  {
	if ((!m_transceiver->IsOpen()) || (m_socket_data == INVALID_SOCKET))
	{
	  XBMC_log(LOG_ERROR, "PVRClientVDR::Process - Loosed connectio to VDR");
	  m_bConnected = false;
	  return NULL;
	}

	tv.tv_sec = 5;
	tv.tv_usec = 0;

	FD_ZERO(&set_r);
	FD_ZERO(&set_e);
	FD_SET(m_socket_data, &set_r);
	FD_SET(m_socket_data, &set_e);
	res = select(FD_SETSIZE, &set_r, NULL, &set_e, &tv);
	if (res < 0)
	{
	  XBMC_log(LOG_ERROR, "PVRClientVDR::Process - select failed");
	  continue;
	}

    if (res == 0)
      continue;

	res = recv(m_socket_data, (char*)data, sizeof(data), 0);
	if (res < 0)
	{
	  XBMC_log(LOG_ERROR, "PVRClientVDR::Process - failed");
	  continue;
	}

	if (res == 0)
	   continue;

	CStdString respStr = data;
	if (respStr.find("MODT", 0) == 0)
	{
	  PVR_event_callback(PVR_EVENT_TIMERS_CHANGE, "");
	}
	else if (respStr.find("DELT", 0) == 0)
	{
	  PVR_event_callback(PVR_EVENT_TIMERS_CHANGE, "");
	}
	else if (respStr.find("ADDT", 0) == 0)
	{
	  PVR_event_callback(PVR_EVENT_TIMERS_CHANGE, "");
	}
	else if (respStr.find("MODC", 0) == 0)
	{
	  PVR_event_callback(PVR_EVENT_CHANNELS_CHANGE, "");
	}
	else if (respStr.find("DELC", 0) == 0)
	{
	  PVR_event_callback(PVR_EVENT_CHANNELS_CHANGE, "");
	}
	else if (respStr.find("ADDC", 0) == 0)
	{
	  PVR_event_callback(PVR_EVENT_CHANNELS_CHANGE, "");
	}
	else
	{
	  XBMC_log(LOG_ERROR, "PVRClientVDR::Process - Unkown respond command %s", respStr.c_str());
	}
  }
  return NULL;
}


/************************************************************/
/** General handling */

const char* PVRClientVDR::GetBackendName()
{
  if (!m_transceiver->IsOpen())
    return "";

  pthread_mutex_lock(&m_critSection);

  vector<string>  lines;
  int             code;

  if (!m_transceiver->SendCommand("STAT name", code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return "";
  }

  vector<string>::iterator it = lines.begin();
  string& data(*it);

  pthread_mutex_unlock(&m_critSection);
  return data.c_str();
}

const char* PVRClientVDR::GetBackendVersion()
{
  if (!m_transceiver->IsOpen())
    return "";

  pthread_mutex_lock(&m_critSection);

  vector<string>  lines;
  int             code;

  if (!m_transceiver->SendCommand("STAT version", code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return "";
  }

  vector<string>::iterator it = lines.begin();

  string& data(*it);

  pthread_mutex_unlock(&m_critSection);
  return data.c_str();
}

const char* PVRClientVDR::GetConnectionString()
{
  char buffer[1024];
  sprintf(buffer, "%s:%i", m_sHostname.c_str(), m_iPort);
  string s_tmp = buffer;
  return s_tmp.c_str();
}

PVR_ERROR PVRClientVDR::GetDriveSpace(long long *total, long long *used)
{
  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  vector<string>  lines;
  int             code;

  if (!m_transceiver->SendCommand("STAT disk", code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  vector<string>::iterator it = lines.begin();
  string& data(*it);
  size_t found = data.find("MB");

  if (found != CStdString::npos)
  {
    *total = atol(data.c_str()) * 1024;
    data.erase(0, found + 3);
    *used = atol(data.c_str()) * 1024;
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** EPG handling */

PVR_ERROR PVRClientVDR::RequestEPGForChannel(unsigned int number, PVRHANDLE handle, time_t start, time_t end)
{

  vector<string> lines;
  int            code;
  char           buffer[1024];
  int            found;
  cEpg           epg;
  PVR_PROGINFO   broadcast;

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  if (start != 0)
    sprintf(buffer, "LSTE %d from %lu to %lu", number, (long)start, (long)end);
  else
    sprintf(buffer, "LSTE %d", number);
  while (!m_transceiver->SendCommand(buffer, code, lines))
  {
    if (code != 451)
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_SERVER_ERROR;
    }
    Sleep(750);
  }

  for (vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
  {
    string& data(*it);
    CStdString str_result = data;

    if (m_bCharsetConv)
      XBMC_unknown_to_utf8(str_result);
    
    bool isEnd = epg.ParseLine(str_result.c_str());
    if (isEnd && epg.StartTime() != 0)
    {
      broadcast.channum         = number;
      broadcast.uid             = epg.UniqueId();
      broadcast.title           = epg.Title();
      broadcast.subtitle        = epg.ShortText();
      broadcast.description     = epg.Description();
      broadcast.starttime       = epg.StartTime();
      broadcast.endtime         = epg.EndTime();
      broadcast.genre           = epg.Genre();
      broadcast.genre_type      = epg.GenreType();
      broadcast.genre_sub_type  = epg.GenreSubType();
      PVR_transfer_epg_entry(handle, &broadcast);
      epg.Reset();
    }
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** Channel handling */

int PVRClientVDR::GetNumChannels()
{
  vector<string>  lines;
  int             code;

  if (!m_transceiver->IsOpen())
    return -1;

  pthread_mutex_lock(&m_critSection);

  if (!m_transceiver->SendCommand("STAT channels", code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return -1;
  }

  vector<string>::iterator it = lines.begin();

  string& data(*it);
  pthread_mutex_unlock(&m_critSection);
  return atol(data.c_str());
}

PVR_ERROR PVRClientVDR::RequestChannelList(PVRHANDLE handle, bool radio)
{
  vector<string> lines;
  int            code;

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  while (!m_transceiver->SendCommand("LSTC", code, lines))
  {
    if (code != 451)
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_SERVER_ERROR;
    }
    Sleep(750);
  }

  for (vector<string>::iterator it = lines.begin(); it < lines.end(); it++)
  {
    string& data(*it);
    CStdString str_result = data;

    if (m_bCharsetConv)
      XBMC_unknown_to_utf8(str_result);

    cChannel channel;
    channel.Parse(str_result.c_str());

    /* Ignore channels without streams */
    if (m_bNoBadChannels && channel.Vpid() == 0 && channel.Apid(0) == 0 && channel.Dpid(0) == 0)
      continue;

    PVR_CHANNEL tag;
    tag.uid = channel.Sid();
    tag.number = channel.Number();
    tag.name = channel.Name();
    tag.callsign = channel.Name();
    tag.iconpath = "";
    tag.encrypted = channel.Ca() ? true : false;
    tag.radio = (channel.Vpid() == 0) && (channel.Apid(0) != 0) ? true : false;
    tag.hide = false;
    tag.recording = false;
    tag.teletext = channel.Tpid() ? true : false;
    tag.bouquet = 0;
    tag.multifeed = false;
    tag.stream_url = "";

    if (radio == tag.radio)
      PVR_transfer_channel_entry(handle, &tag);
  }

  pthread_mutex_unlock(&m_critSection);
  return PVR_ERROR_NO_ERROR;
}
/*
PVR_ERROR PVRClientVDR::GetChannelSettings(cPVRChannelInfoTag *result)
{

  vector<string> lines;
  int            code;
  char           buffer[1024];

  if (result->m_iClientNum < 1)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  sprintf(buffer, "LSTC %d", result->m_iClientNum);
  while (!m_transceiver->SendCommand(buffer, code, lines))
  {
    if (code != 451)
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_SERVER_ERROR;
    }
    Sleep(750);
  }

  for (vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
  {
    string& data(*it);
    CStdString str_result = data;
    int found;
    int i_tmp;

    result->m_Settings.m_VPID = 0;
    result->m_Settings.m_APID1 = 0;
    result->m_Settings.m_APID2 = 0;
    result->m_Settings.m_DPID1 = 0;
    result->m_Settings.m_DPID2 = 0;
    result->m_Settings.m_CAID = 0;
    CStdString name;
    int id;

    if (m_bCharsetConv)
      XBMC_unknown_to_utf8(str_result);

    // Skip Channel number
    str_result.erase(0, str_result.find(" ", 0) + 1);

    // Channel and provider name
    found = str_result.find(":", 0);
    name.assign(str_result, found);
    str_result.erase(0, found + 1);
    found = name.find(";", 0);

    if (found != -1)
    {
      name.erase(0, found + 1);
      result->m_Settings.m_strProvider = name;
    }

    // Channel frequency
    result->m_Settings.m_Freq = atol(str_result.c_str());
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);
    found = str_result.find(":", 0);
    result->m_Settings.m_parameter.assign(str_result, found);
    str_result.erase(0, found);

    // Source Type
    if (str_result.compare(0, 2, ":C") == 0)
    {
      result->m_Settings.m_SourceType = src_DVBC;
      result->m_Settings.m_satellite = "Cable";
      str_result.erase(0, 3);
    }
    else if (str_result.compare(0, 2, ":T") == 0)
    {
      result->m_Settings.m_SourceType = src_DVBT;
      result->m_Settings.m_satellite = "Terrestrial";
      str_result.erase(0, 3);
    }
    else if (str_result.compare(0, 2, ":S") == 0)
    {
      result->m_Settings.m_SourceType = src_DVBS;
      str_result.erase(0, 2);
      found = str_result.find(":", 0);
      result->m_Settings.m_satellite.assign(str_result, found);
      str_result.erase(0, found + 1);
    }
    else if (str_result.compare(0, 2, ":P") == 0)
    {
      result->m_Settings.m_SourceType = srcAnalog;
      result->m_Settings.m_satellite = "Analog";
      str_result.erase(0, 3);
    }

    // Channel symbolrate
    result->m_Settings.m_Symbolrate = atol(str_result.c_str());
    str_result.erase(0, str_result.find(":", 0) + 1);

    // Channel program id
    result->m_Settings.m_VPID = atol(str_result.c_str());
    str_result.erase(0, str_result.find(":", 0) + 1);

    // Channel audio id's
    found = str_result.find(":", 0);
    name.assign(str_result, found);
    str_result.erase(0, found + 1);
    found = name.find(";", 0);

    if (found == -1)
    {
      id = atol(name.c_str());

      if (id == 0)
      {
        result->m_Settings.m_APID1 = 0;
        result->m_Settings.m_APID2 = 0;
        result->m_Settings.m_DPID1 = 0;
        result->m_Settings.m_DPID2 = 0;
      }
      else
      {
        result->m_Settings.m_APID1 = id;
        found = name.find(",", 0);

        if (found == -1)
        {
          result->m_Settings.m_APID2 = 0;
        }
        else
        {
          name.erase(0, found + 1);
          result->m_Settings.m_APID2 = atol(name.c_str());
        }
        result->m_Settings.m_DPID1 = 0;
        result->m_Settings.m_DPID2 = 0;
      }
    }
    else
    {
      int id;

      id = atol(name.c_str());

      if (id == 0)
      {
        result->m_Settings.m_APID1 = 0;
        result->m_Settings.m_APID2 = 0;
      }
      else
      {
        result->m_Settings.m_APID1 = id;
        found = name.find(",", 0);

        if (found == -1)
        {
          result->m_Settings.m_APID2 = 0;
        }
        else
        {
          name.erase(0, found + 1);
          result->m_Settings.m_APID2 = atol(name.c_str());
        }
      }

      found = name.find(";", 0);

      name.erase(0, found + 1);
      id = atol(name.c_str());

      if (id == 0)
      {
        result->m_Settings.m_DPID1 = 0;
        result->m_Settings.m_DPID2 = 0;
      }
      else
      {
        result->m_Settings.m_DPID1 = id;
        found = name.find(",", 0);

        if (found == -1)
        {
          result->m_Settings.m_DPID2 = 0;
        }
        else
        {
          name.erase(0, found + 1);
          result->m_Settings.m_DPID2 = atol(name.c_str());
        }
      }
    }

    // Teletext id
    result->m_Settings.m_TPID = atol(str_result.c_str());
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);

    // CAID id
    result->m_Settings.m_CAID = atol(str_result.c_str());
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);

    // Service id
    result->m_Settings.m_SID = atol(str_result.c_str());
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);

    // Network id
    result->m_Settings.m_NID = atol(str_result.c_str());
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);

    // Transport id
    result->m_Settings.m_TID = atol(str_result.c_str());
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);

    // Radio id
    result->m_Settings.m_RID = atol(str_result.c_str());
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);

    // DVB-S2 ?
    if (result->m_Settings.m_SourceType == src_DVBS)
    {
      str_result = result->m_Settings.m_parameter;
      found = str_result.find("S", 0);

      if (found != -1)
      {
        str_result.erase(0, found + 1);
        i_tmp = atol(str_result.c_str());

        if (i_tmp == 1)
        {
          result->m_Settings.m_SourceType = src_DVBS2;
        }
      }
    }

    // Inversion
    str_result = result->m_Settings.m_parameter;
    found = str_result.find("I", 0);
    if (found != -1)
    {
      str_result.erase(0, found + 1);
      i_tmp = atol(str_result.c_str());

      if (i_tmp == 0)
      {
        result->m_Settings.m_Inversion = InvOff;
      }
      else if (i_tmp == 1)
      {
        result->m_Settings.m_Inversion = InvOn;
      }
      else if (i_tmp == 999)
      {
        result->m_Settings.m_Inversion = InvAuto;
      }
    }
    else
    {
      result->m_Settings.m_Inversion = InvAuto;
    }

    // CoderateL
    if (result->m_Settings.m_SourceType == src_DVBT)
    {
      str_result = result->m_Settings.m_parameter;
      found = str_result.find("D", 0);

      if (found != -1)
      {
        str_result.erase(0, found + 1);
        i_tmp = atol(str_result.c_str());

        if (i_tmp == 0)
        {
          result->m_Settings.m_CoderateL = Coderate_None;
        }
        else if (i_tmp == 12)
        {
          result->m_Settings.m_CoderateL = Coderate_1_2;
        }
        else if (i_tmp == 23)
        {
          result->m_Settings.m_CoderateL = Coderate_2_3;
        }
        else if (i_tmp == 34)
        {
          result->m_Settings.m_CoderateL = Coderate_3_4;
        }
        else if (i_tmp == 45)
        {
          result->m_Settings.m_CoderateL = Coderate_4_5;
        }
        else if (i_tmp == 56)
        {
          result->m_Settings.m_CoderateL = Coderate_5_6;
        }
        else if (i_tmp == 67)
        {
          result->m_Settings.m_CoderateL = Coderate_6_7;
        }
        else if (i_tmp == 78)
        {
          result->m_Settings.m_CoderateL = Coderate_7_8;
        }
        else if (i_tmp == 89)
        {
          result->m_Settings.m_CoderateL = Coderate_8_9;
        }
        else if (i_tmp == 910)
        {
          result->m_Settings.m_CoderateL = Coderate_9_10;
        }
        else if (i_tmp == 999 || i_tmp == 910)
        {
          result->m_Settings.m_CoderateL = Coderate_Auto;
        }
      }
      else
      {
        result->m_Settings.m_CoderateL = Coderate_None;
      }
    }
    else
    {
      result->m_Settings.m_CoderateL = Coderate_None;
    }

    // CoderateH
    str_result = result->m_Settings.m_parameter;

    found = str_result.find("C", 0);

    if (found != -1)
    {
      str_result.erase(0, found + 1);
      i_tmp = atol(str_result.c_str());

      if (i_tmp == 0)
      {
        result->m_Settings.m_CoderateH = Coderate_None;
      }
      else if (i_tmp == 12)
      {
        result->m_Settings.m_CoderateH = Coderate_1_2;
      }
      else if (i_tmp == 23)
      {
        result->m_Settings.m_CoderateH = Coderate_2_3;
      }
      else if (i_tmp == 34)
      {
        result->m_Settings.m_CoderateH = Coderate_3_4;
      }
      else if (i_tmp == 45)
      {
        result->m_Settings.m_CoderateH = Coderate_4_5;
      }
      else if (i_tmp == 56)
      {
        result->m_Settings.m_CoderateH = Coderate_5_6;
      }
      else if (i_tmp == 67)
      {
        result->m_Settings.m_CoderateH = Coderate_6_7;
      }
      else if (i_tmp == 78)
      {
        result->m_Settings.m_CoderateH = Coderate_7_8;
      }
      else if (i_tmp == 89)
      {
        result->m_Settings.m_CoderateH = Coderate_8_9;
      }
      else if (i_tmp == 910)
      {
        result->m_Settings.m_CoderateL = Coderate_9_10;
      }
      else if (i_tmp == 999 || i_tmp == 910)
      {
        result->m_Settings.m_CoderateH = Coderate_Auto;
      }
    }
    else
    {
      result->m_Settings.m_CoderateH = Coderate_None;
    }

    // Modulation
    str_result = result->m_Settings.m_parameter;

    found = str_result.find("M", 0);

    if (found != -1)
    {
      str_result.erase(0, found + 1);
      i_tmp = atol(str_result.c_str());

      if (i_tmp == 0)
      {
        result->m_Settings.m_Modulation = modNone;
      }
      else if (i_tmp == 4)
      {
        result->m_Settings.m_Modulation = modQAM4;
      }
      else if (i_tmp == 16)
      {
        result->m_Settings.m_Modulation = modQAM16;
      }
      else if (i_tmp == 32)
      {
        result->m_Settings.m_Modulation = modQAM32;
      }
      else if (i_tmp == 64)
      {
        result->m_Settings.m_Modulation = modQAM64;
      }
      else if (i_tmp == 128)
      {
        result->m_Settings.m_Modulation = modQAM128;
      }
      else if (i_tmp == 256)
      {
        result->m_Settings.m_Modulation = modQAM256;
      }
      else if (i_tmp == 512)
      {
        result->m_Settings.m_Modulation = modQAM512;
      }
      else if (i_tmp == 1024)
      {
        result->m_Settings.m_Modulation = modQAM1024;
      }
      else if (i_tmp == 998)
      {
        result->m_Settings.m_Modulation = modQAMAuto;
      }
      else if (i_tmp == 1)
      {
        result->m_Settings.m_Modulation = modBPSK;
      }
      else if (i_tmp == 2)
      {
        result->m_Settings.m_Modulation = modQPSK;
      }
      else if (i_tmp == 3)
      {
        result->m_Settings.m_Modulation = modOQPSK;
      }
      else if (i_tmp == 5)
      {
        result->m_Settings.m_Modulation = mod8PSK;
      }
      else if (i_tmp == 6)
      {
        result->m_Settings.m_Modulation = mod16APSK;
      }
      else if (i_tmp == 7)
      {
        result->m_Settings.m_Modulation = mod32APSK;
      }
      else if (i_tmp == 8)
      {
        result->m_Settings.m_Modulation = modOFDM;
      }
      else if (i_tmp == 9)
      {
        result->m_Settings.m_Modulation = modCOFDM;
      }
      else if (i_tmp == 10)
      {
        result->m_Settings.m_Modulation = modVSB8;
      }
      else if (i_tmp == 11)
      {
        result->m_Settings.m_Modulation = modVSB16;
      }
    }
    else
    {
      result->m_Settings.m_Modulation = modNone;
    }

    // Bandwith
    str_result = result->m_Settings.m_parameter;

    found = str_result.find("B", 0);

    if (found != -1)
    {
      str_result.erase(0, found + 1);
      i_tmp = atol(str_result.c_str());

      if (i_tmp == 5)
      {
        result->m_Settings.m_Bandwidth = bw_5MHz;
      }
      else if (i_tmp == 6)
      {
        result->m_Settings.m_Bandwidth = bw_6MHz;
      }
      else if (i_tmp == 7)
      {
        result->m_Settings.m_Bandwidth = bw_7MHz;
      }
      else if (i_tmp == 8)
      {
        result->m_Settings.m_Bandwidth = bw_8MHz;
      }
      else if (i_tmp == 999)
      {
        result->m_Settings.m_Bandwidth = bw_Auto;
      }
    }
    else
    {
      result->m_Settings.m_Bandwidth = bw_Auto;
    }

    // Hierarchie
    str_result = result->m_Settings.m_parameter;

    found = str_result.find("Y", 0);

    if (found != -1)
    {
      str_result.erase(0, found + 1);
      i_tmp = atol(str_result.c_str());

      if (i_tmp == 0)
      {
        result->m_Settings.m_Hierarchie = false;
      }
      else if (i_tmp == 1)
      {
        result->m_Settings.m_Hierarchie = true;
      }
    }
    else
    {
      result->m_Settings.m_Hierarchie = false;
    }

    // Alpha
    str_result = result->m_Settings.m_parameter;

    found = str_result.find("A", 0);

    if (found != -1)
    {
      str_result.erase(0, found + 1);
      i_tmp = atol(str_result.c_str());

      if (i_tmp == 0)
      {
        result->m_Settings.m_Alpha = alpha_0;
      }
      else if (i_tmp == 1)
      {
        result->m_Settings.m_Alpha = alpha_1;
      }
      else if (i_tmp == 2)
      {
        result->m_Settings.m_Alpha = alpha_2;
      }
      else if (i_tmp == 4)
      {
        result->m_Settings.m_Alpha = alpha_4;
      }
    }
    else
    {
      result->m_Settings.m_Alpha = alpha_0;
    }

    // Guard
    str_result = result->m_Settings.m_parameter;

    found = str_result.find("G", 0);

    if (found != -1)
    {
      str_result.erase(0, found + 1);
      i_tmp = atol(str_result.c_str());

      if (i_tmp == 4)
      {
        result->m_Settings.m_Guard = guard_1_4;
      }
      else if (i_tmp == 8)
      {
        result->m_Settings.m_Guard = guard_1_8;
      }
      else if (i_tmp == 16)
      {
        result->m_Settings.m_Guard = guard_1_16;
      }
      else if (i_tmp == 32)
      {
        result->m_Settings.m_Guard = guard_1_32;
      }
      else if (i_tmp == 999)
      {
        result->m_Settings.m_Guard = guard_Auto;
      }
    }
    else
    {
      result->m_Settings.m_Guard = guard_Auto;
    }

    // Transmission
    str_result = result->m_Settings.m_parameter;

    found = str_result.find("T", 0);

    if (found != -1)
    {
      str_result.erase(0, found + 1);
      i_tmp = atol(str_result.c_str());

      if (i_tmp == 2)
      {
        result->m_Settings.m_Transmission = transmission_2K;
      }
      else if (i_tmp == 4)
      {
        result->m_Settings.m_Transmission = transmission_4K;
      }
      else if (i_tmp == 8)
      {
        result->m_Settings.m_Transmission = transmission_8K;
      }
      else if (i_tmp == 999)
      {
        result->m_Settings.m_Transmission = transmission_Auto;
      }
    }
    else
    {
      result->m_Settings.m_Transmission = transmission_Auto;
    }

    // Priority
    str_result = result->m_Settings.m_parameter;

    found = str_result.find("P", 0);

    if (found != -1)
    {
      str_result.erase(0, found + 1);
      i_tmp = atol(str_result.c_str());

      if (i_tmp == 0)
      {
        result->m_Settings.m_Priority = false;
      }
      else if (i_tmp == 1)
      {
        result->m_Settings.m_Priority = true;
      }
    }
    else
    {
      result->m_Settings.m_Priority = false;
    }

    // Rolloff
    str_result = result->m_Settings.m_parameter;

    found = str_result.find("O", 0);

    if (found != -1)
    {
      str_result.erase(0, found + 1);
      i_tmp = atol(str_result.c_str());

      if (i_tmp == 0)
      {
        result->m_Settings.m_Rolloff = rolloff_Unknown;
      }
      else if (i_tmp == 20)
      {
        result->m_Settings.m_Rolloff = rolloff_20;
      }
      else if (i_tmp == 25)
      {
        result->m_Settings.m_Rolloff = rolloff_25;
      }
      else if (i_tmp == 35)
      {
        result->m_Settings.m_Rolloff = rolloff_35;
      }
    }
    else
    {
      result->m_Settings.m_Rolloff = rolloff_Unknown;
    }

    // Polarization
    str_result = result->m_Settings.m_parameter;

    if ((int) str_result.find("H", 0) != -1)
      result->m_Settings.m_Polarization = pol_H;

    if ((int) str_result.find("h", 0) != -1)
      result->m_Settings.m_Polarization = pol_H;

    if ((int) str_result.find("V", 0) != -1)
      result->m_Settings.m_Polarization = pol_V;

    if ((int) str_result.find("v", 0) != -1)
      result->m_Settings.m_Polarization = pol_V;

    if ((int) str_result.find("L", 0) != -1)
      result->m_Settings.m_Polarization = pol_L;

    if ((int) str_result.find("l", 0) != -1)
      result->m_Settings.m_Polarization = pol_L;

    if ((int) str_result.find("R", 0) != -1)
      result->m_Settings.m_Polarization = pol_R;

    if ((int) str_result.find("r", 0) != -1)
      result->m_Settings.m_Polarization = pol_R;
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientVDR::UpdateChannelSettings(const cPVRChannelInfoTag &chaninfo)
{
  CStdString     m_Summary;
  CStdString     m_Summary_2;
  vector<string> lines;
  int            code;
  char           buffer[1024];

  pthread_mutex_lock(&m_critSection);

  if (chaninfo.m_iClientNum == -1)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SAVED;
  }

  m_Summary.Format("%d %s;%s:%i:" , chaninfo.m_iClientNum

                   , chaninfo.m_strChannel.c_str()
                   , chaninfo.m_Settings.m_strProvider.c_str()
                   , chaninfo.m_Settings.m_Freq);

  if ((chaninfo.m_Settings.m_SourceType == src_DVBS) ||
      (chaninfo.m_Settings.m_SourceType == src_DVBS2))
  {
    if      (chaninfo.m_Settings.m_Polarization == pol_H)
      m_Summary += "h";
    else if (chaninfo.m_Settings.m_Polarization == pol_V)
      m_Summary += "v";
    else if (chaninfo.m_Settings.m_Polarization == pol_L)
      m_Summary += "l";
    else if (chaninfo.m_Settings.m_Polarization == pol_R)
      m_Summary += "r";
  }

  if (chaninfo.m_Settings.m_SourceType == src_DVBT)
  {
    if      (chaninfo.m_Settings.m_Inversion == InvOff)
      m_Summary += "I0";
    else if (chaninfo.m_Settings.m_Inversion == InvOn)
      m_Summary += "I1";
    else if (chaninfo.m_Settings.m_Inversion == InvAuto)
      m_Summary += "I999";
  }

  if (chaninfo.m_Settings.m_SourceType == src_DVBT)
  {
    if      (chaninfo.m_Settings.m_Bandwidth == bw_5MHz)
      m_Summary += "B5";
    else if (chaninfo.m_Settings.m_Bandwidth == bw_6MHz)
      m_Summary += "B6";
    else if (chaninfo.m_Settings.m_Bandwidth == bw_7MHz)
      m_Summary += "B7";
    else if (chaninfo.m_Settings.m_Bandwidth == bw_8MHz)
      m_Summary += "B8";
    else if (chaninfo.m_Settings.m_Bandwidth == bw_Auto)
      m_Summary += "B999";
  }

  if      (chaninfo.m_Settings.m_CoderateH == Coderate_None)
    m_Summary += "C0";
  else if (chaninfo.m_Settings.m_CoderateH == Coderate_1_2)
    m_Summary += "C12";
  else if (chaninfo.m_Settings.m_CoderateH == Coderate_2_3)
    m_Summary += "C23";
  else if (chaninfo.m_Settings.m_CoderateH == Coderate_3_4)
    m_Summary += "C34";
  else if (chaninfo.m_Settings.m_CoderateH == Coderate_4_5)
    m_Summary += "C45";
  else if (chaninfo.m_Settings.m_CoderateH == Coderate_5_6)
    m_Summary += "C56";
  else if (chaninfo.m_Settings.m_CoderateH == Coderate_6_7)
    m_Summary += "C67";
  else if (chaninfo.m_Settings.m_CoderateH == Coderate_7_8)
    m_Summary += "C78";
  else if (chaninfo.m_Settings.m_CoderateH == Coderate_8_9)
    m_Summary += "C89";
  else if (chaninfo.m_Settings.m_CoderateH == Coderate_9_10)
    m_Summary += "C910";
  else if (chaninfo.m_Settings.m_CoderateH == Coderate_Auto)
    m_Summary += "C999";

  if (chaninfo.m_Settings.m_SourceType == src_DVBT)
  {
    if      (chaninfo.m_Settings.m_CoderateL == Coderate_None)
      m_Summary += "D0";
    else if (chaninfo.m_Settings.m_CoderateL == Coderate_1_2)
      m_Summary += "D12";
    else if (chaninfo.m_Settings.m_CoderateL == Coderate_2_3)
      m_Summary += "D23";
    else if (chaninfo.m_Settings.m_CoderateL == Coderate_3_4)
      m_Summary += "D34";
    else if (chaninfo.m_Settings.m_CoderateL == Coderate_4_5)
      m_Summary += "D45";
    else if (chaninfo.m_Settings.m_CoderateL == Coderate_5_6)
      m_Summary += "D56";
    else if (chaninfo.m_Settings.m_CoderateL == Coderate_6_7)
      m_Summary += "D67";
    else if (chaninfo.m_Settings.m_CoderateL == Coderate_7_8)
      m_Summary += "D78";
    else if (chaninfo.m_Settings.m_CoderateL == Coderate_8_9)
      m_Summary += "D89";
    else if (chaninfo.m_Settings.m_CoderateL == Coderate_9_10)
      m_Summary += "D910";
    else if (chaninfo.m_Settings.m_CoderateL == Coderate_Auto)
      m_Summary += "D999";
  }

  if      (chaninfo.m_Settings.m_Modulation == modNone)
    m_Summary += "M0";
  else if (chaninfo.m_Settings.m_Modulation == modQAM4)
    m_Summary += "M4";
  else if (chaninfo.m_Settings.m_Modulation == modQAM16)
    m_Summary += "M16";
  else if (chaninfo.m_Settings.m_Modulation == modQAM32)
    m_Summary += "M32";
  else if (chaninfo.m_Settings.m_Modulation == modQAM64)
    m_Summary += "M64";
  else if (chaninfo.m_Settings.m_Modulation == modQAM128)
    m_Summary += "M128";
  else if (chaninfo.m_Settings.m_Modulation == modQAM256)
    m_Summary += "M256";
  else if (chaninfo.m_Settings.m_Modulation == modQAM512)
    m_Summary += "M512";
  else if (chaninfo.m_Settings.m_Modulation == modQAM1024)
    m_Summary += "M1024";
  else if (chaninfo.m_Settings.m_Modulation == modQAMAuto)
    m_Summary += "M998";
  else if (chaninfo.m_Settings.m_Modulation == modBPSK)
    m_Summary += "M1";
  else if (chaninfo.m_Settings.m_Modulation == modQPSK)
    m_Summary += "M2";
  else if (chaninfo.m_Settings.m_Modulation == modOQPSK)
    m_Summary += "M3";
  else if (chaninfo.m_Settings.m_Modulation == mod8PSK)
    m_Summary += "M5";
  else if (chaninfo.m_Settings.m_Modulation == mod16APSK)
    m_Summary += "M6";
  else if (chaninfo.m_Settings.m_Modulation == mod32APSK)
    m_Summary += "M7";
  else if (chaninfo.m_Settings.m_Modulation == modOFDM)
    m_Summary += "M8";
  else if (chaninfo.m_Settings.m_Modulation == modCOFDM)
    m_Summary += "M9";
  else if (chaninfo.m_Settings.m_Modulation == modVSB8)
    m_Summary += "M10";
  else if (chaninfo.m_Settings.m_Modulation == modVSB16)
    m_Summary += "M11";

  if (chaninfo.m_Settings.m_SourceType == src_DVBT)
  {
    if      (chaninfo.m_Settings.m_Transmission == transmission_2K)
      m_Summary += "T2";
    else if (chaninfo.m_Settings.m_Transmission == transmission_4K)
      m_Summary += "T4";
    else if (chaninfo.m_Settings.m_Transmission == transmission_8K)
      m_Summary += "T8";
    else if (chaninfo.m_Settings.m_Transmission == transmission_Auto)
      m_Summary += "T999";

    if      (chaninfo.m_Settings.m_Guard == guard_1_4)
      m_Summary += "G4";
    else if (chaninfo.m_Settings.m_Guard == guard_1_8)
      m_Summary += "G8";
    else if (chaninfo.m_Settings.m_Guard == guard_1_16)
      m_Summary += "G16";
    else if (chaninfo.m_Settings.m_Guard == guard_1_32)
      m_Summary += "G32";
    else if (chaninfo.m_Settings.m_Guard == guard_Auto)
      m_Summary += "G999";

    if      (chaninfo.m_Settings.m_Hierarchie)
      m_Summary += "Y1";
    else
      m_Summary += "Y0";

    if      (chaninfo.m_Settings.m_Alpha == alpha_0)
      m_Summary += "A0";
    else if (chaninfo.m_Settings.m_Alpha == alpha_1)
      m_Summary += "A1";
    else if (chaninfo.m_Settings.m_Alpha == alpha_2)
      m_Summary += "A2";
    else if (chaninfo.m_Settings.m_Alpha == alpha_4)
      m_Summary += "A4";

    if      (chaninfo.m_Settings.m_Priority)
      m_Summary += "P1";
    else
      m_Summary += "P0";
  }

  if (chaninfo.m_Settings.m_SourceType == src_DVBS2)
  {
    if      (chaninfo.m_Settings.m_Rolloff == rolloff_Unknown)
      m_Summary += "O0";
    else if (chaninfo.m_Settings.m_Rolloff == rolloff_20)
      m_Summary += "O20";
    else if (chaninfo.m_Settings.m_Rolloff == rolloff_25)
      m_Summary += "O25";
    else if (chaninfo.m_Settings.m_Rolloff == rolloff_25)
      m_Summary += "O35";
  }

  if      (chaninfo.m_Settings.m_SourceType == src_DVBS)
    m_Summary += "O35S0:S";
  else if (chaninfo.m_Settings.m_SourceType == src_DVBS2)
    m_Summary += "S1:S";
  else if (chaninfo.m_Settings.m_SourceType == src_DVBC)
    m_Summary += ":C";
  else if (chaninfo.m_Settings.m_SourceType == src_DVBT)
    m_Summary += ":T";

  m_Summary_2.Format(":%i:%i:%i,%i;%i,%i:%i:%i:%i:%i:%i:%i" , chaninfo.m_Settings.m_Symbolrate
                     , chaninfo.m_Settings.m_VPID
                     , chaninfo.m_Settings.m_APID1
                     , chaninfo.m_Settings.m_APID2
                     , chaninfo.m_Settings.m_DPID1
                     , chaninfo.m_Settings.m_DPID2
                     , chaninfo.m_Settings.m_TPID
                     , chaninfo.m_Settings.m_CAID
                     , chaninfo.m_Settings.m_SID
                     , chaninfo.m_Settings.m_NID
                     , chaninfo.m_Settings.m_TID
                     , chaninfo.m_Settings.m_RID);

  m_Summary += m_Summary_2;

  sprintf(buffer, "LSTC %d", chaninfo.m_iClientNum);

  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SYNC;
  }

  sprintf(buffer, "MODC %s", m_Summary.c_str());

  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SAVED;
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientVDR::AddChannel(const cPVRChannelInfoTag &info)
{

  CStdString m_Summary;
  CStdString m_Summary_2;
  bool update_channel;
  int iChannelNum;

  if (!m_transceiver->IsOpen())
  {
    return PVR_ERROR_SERVER_ERROR;
  }

  if (info.m_iClientNum == -1)
  {
    int new_number = GetNumChannels();

    if (new_number == -1)
    {
      new_number = 1;
    }

    iChannelNum = new_number + 1;

    update_channel = false;
  }
  else
  {
    iChannelNum = info.m_iClientNum;
    update_channel = true;
  }

  m_Summary.Format("%d %s;%s:%i:" , iChannelNum

                   , info.m_strChannel.c_str()
                   , info.m_Settings.m_strProvider.c_str()
                   , info.m_Settings.m_Freq);

  if ((info.m_Settings.m_SourceType == src_DVBS) ||
      (info.m_Settings.m_SourceType == src_DVBS2))
  {
    if      (info.m_Settings.m_Polarization == pol_H)
      m_Summary += "h";
    else if (info.m_Settings.m_Polarization == pol_V)
      m_Summary += "v";
    else if (info.m_Settings.m_Polarization == pol_L)
      m_Summary += "l";
    else if (info.m_Settings.m_Polarization == pol_R)
      m_Summary += "r";
  }

  if (info.m_Settings.m_SourceType == src_DVBT)
  {
    if      (info.m_Settings.m_Inversion == InvOff)
      m_Summary += "I0";
    else if (info.m_Settings.m_Inversion == InvOn)
      m_Summary += "I1";
    else if (info.m_Settings.m_Inversion == InvAuto)
      m_Summary += "I999";
  }

  if (info.m_Settings.m_SourceType == src_DVBT)
  {
    if      (info.m_Settings.m_Bandwidth == bw_5MHz)
      m_Summary += "B5";
    else if (info.m_Settings.m_Bandwidth == bw_6MHz)
      m_Summary += "B6";
    else if (info.m_Settings.m_Bandwidth == bw_7MHz)
      m_Summary += "B7";
    else if (info.m_Settings.m_Bandwidth == bw_8MHz)
      m_Summary += "B8";
    else if (info.m_Settings.m_Bandwidth == bw_Auto)
      m_Summary += "B999";
  }

  if      (info.m_Settings.m_CoderateH == Coderate_None)
    m_Summary += "C0";
  else if (info.m_Settings.m_CoderateH == Coderate_1_2)
    m_Summary += "C12";
  else if (info.m_Settings.m_CoderateH == Coderate_2_3)
    m_Summary += "C23";
  else if (info.m_Settings.m_CoderateH == Coderate_3_4)
    m_Summary += "C34";
  else if (info.m_Settings.m_CoderateH == Coderate_4_5)
    m_Summary += "C45";
  else if (info.m_Settings.m_CoderateH == Coderate_5_6)
    m_Summary += "C56";
  else if (info.m_Settings.m_CoderateH == Coderate_6_7)
    m_Summary += "C67";
  else if (info.m_Settings.m_CoderateH == Coderate_7_8)
    m_Summary += "C78";
  else if (info.m_Settings.m_CoderateH == Coderate_8_9)
    m_Summary += "C89";
  else if (info.m_Settings.m_CoderateH == Coderate_9_10)
    m_Summary += "C910";
  else if (info.m_Settings.m_CoderateH == Coderate_Auto)
    m_Summary += "C999";

  if (info.m_Settings.m_SourceType == src_DVBT)
  {
    if      (info.m_Settings.m_CoderateL == Coderate_None)
      m_Summary += "D0";
    else if (info.m_Settings.m_CoderateL == Coderate_1_2)
      m_Summary += "D12";
    else if (info.m_Settings.m_CoderateL == Coderate_2_3)
      m_Summary += "D23";
    else if (info.m_Settings.m_CoderateL == Coderate_3_4)
      m_Summary += "D34";
    else if (info.m_Settings.m_CoderateL == Coderate_4_5)
      m_Summary += "D45";
    else if (info.m_Settings.m_CoderateL == Coderate_5_6)
      m_Summary += "D56";
    else if (info.m_Settings.m_CoderateL == Coderate_6_7)
      m_Summary += "D67";
    else if (info.m_Settings.m_CoderateL == Coderate_7_8)
      m_Summary += "D78";
    else if (info.m_Settings.m_CoderateL == Coderate_8_9)
      m_Summary += "D89";
    else if (info.m_Settings.m_CoderateL == Coderate_9_10)
      m_Summary += "D910";
    else if (info.m_Settings.m_CoderateL == Coderate_Auto)
      m_Summary += "D999";
  }

  if      (info.m_Settings.m_Modulation == modNone)
    m_Summary += "M0";
  else if (info.m_Settings.m_Modulation == modQAM4)
    m_Summary += "M4";
  else if (info.m_Settings.m_Modulation == modQAM16)
    m_Summary += "M16";
  else if (info.m_Settings.m_Modulation == modQAM32)
    m_Summary += "M32";
  else if (info.m_Settings.m_Modulation == modQAM64)
    m_Summary += "M64";
  else if (info.m_Settings.m_Modulation == modQAM128)
    m_Summary += "M128";
  else if (info.m_Settings.m_Modulation == modQAM256)
    m_Summary += "M256";
  else if (info.m_Settings.m_Modulation == modQAM512)
    m_Summary += "M512";
  else if (info.m_Settings.m_Modulation == modQAM1024)
    m_Summary += "M1024";
  else if (info.m_Settings.m_Modulation == modQAMAuto)
    m_Summary += "M998";
  else if (info.m_Settings.m_Modulation == modBPSK)
    m_Summary += "M1";
  else if (info.m_Settings.m_Modulation == modQPSK)
    m_Summary += "M2";
  else if (info.m_Settings.m_Modulation == modOQPSK)
    m_Summary += "M3";
  else if (info.m_Settings.m_Modulation == mod8PSK)
    m_Summary += "M5";
  else if (info.m_Settings.m_Modulation == mod16APSK)
    m_Summary += "M6";
  else if (info.m_Settings.m_Modulation == mod32APSK)
    m_Summary += "M7";
  else if (info.m_Settings.m_Modulation == modOFDM)
    m_Summary += "M8";
  else if (info.m_Settings.m_Modulation == modCOFDM)
    m_Summary += "M9";
  else if (info.m_Settings.m_Modulation == modVSB8)
    m_Summary += "M10";
  else if (info.m_Settings.m_Modulation == modVSB16)
    m_Summary += "M11";

  if (info.m_Settings.m_SourceType == src_DVBT)
  {
    if      (info.m_Settings.m_Transmission == transmission_2K)
      m_Summary += "T2";
    else if (info.m_Settings.m_Transmission == transmission_4K)
      m_Summary += "T4";
    else if (info.m_Settings.m_Transmission == transmission_8K)
      m_Summary += "T8";
    else if (info.m_Settings.m_Transmission == transmission_Auto)
      m_Summary += "T999";

    if      (info.m_Settings.m_Guard == guard_1_4)
      m_Summary += "G4";
    else if (info.m_Settings.m_Guard == guard_1_8)
      m_Summary += "G8";
    else if (info.m_Settings.m_Guard == guard_1_16)
      m_Summary += "G16";
    else if (info.m_Settings.m_Guard == guard_1_32)
      m_Summary += "G32";
    else if (info.m_Settings.m_Guard == guard_Auto)
      m_Summary += "G999";

    if (info.m_Settings.m_Hierarchie)
      m_Summary += "Y1";
    else
      m_Summary += "Y0";

    if      (info.m_Settings.m_Alpha == alpha_0)
      m_Summary += "A0";
    else if (info.m_Settings.m_Alpha == alpha_1)
      m_Summary += "A1";
    else if (info.m_Settings.m_Alpha == alpha_2)
      m_Summary += "A2";
    else if (info.m_Settings.m_Alpha == alpha_4)
      m_Summary += "A4";

    if (info.m_Settings.m_Priority)
      m_Summary += "P1";
    else
      m_Summary += "P0";
  }

  if (info.m_Settings.m_SourceType == src_DVBS2)
  {
    if      (info.m_Settings.m_Rolloff == rolloff_Unknown)
      m_Summary += "O0";
    else if (info.m_Settings.m_Rolloff == rolloff_20)
      m_Summary += "O20";
    else if (info.m_Settings.m_Rolloff == rolloff_25)
      m_Summary += "O25";
    else if (info.m_Settings.m_Rolloff == rolloff_25)
      m_Summary += "O35";
  }

  if      (info.m_Settings.m_SourceType == src_DVBS)
    m_Summary += "O35S0:S";
  else if (info.m_Settings.m_SourceType == src_DVBS2)
    m_Summary += "S1:S";
  else if (info.m_Settings.m_SourceType == src_DVBC)
    m_Summary += ":C";
  else if (info.m_Settings.m_SourceType == src_DVBT)
    m_Summary += ":T";

  m_Summary_2.Format(":%i:%i:%i,%i;%i,%i:%i:%i:%i:%i:%i:%i" , info.m_Settings.m_Symbolrate
                     , info.m_Settings.m_VPID
                     , info.m_Settings.m_APID1
                     , info.m_Settings.m_APID2
                     , info.m_Settings.m_DPID1
                     , info.m_Settings.m_DPID2
                     , info.m_Settings.m_TPID
                     , info.m_Settings.m_CAID
                     , info.m_Settings.m_SID
                     , info.m_Settings.m_NID
                     , info.m_Settings.m_TID
                     , info.m_Settings.m_RID);

  m_Summary += m_Summary_2;

  vector<string> lines;

  int            code;

  char           buffer[1024];

  pthread_mutex_lock(&m_critSection);

  if (!update_channel)
  {
    sprintf(buffer, "NEWC %s", m_Summary.c_str());

    if (!m_transceiver->SendCommand(buffer, code, lines))
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_SERVER_ERROR;
    }

    if (code != 250)
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_NOT_SAVED;
    }
  }
  else
  {
    // Modified channel
    sprintf(buffer, "LSTC %d", iChannelNum);

    if (!m_transceiver->SendCommand(buffer, code, lines))
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_SERVER_ERROR;
    }

    if (code != 250)
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_NOT_SYNC;
    }

    sprintf(buffer, "MODC %s", m_Summary.c_str());

    if (!m_transceiver->SendCommand(buffer, code, lines))
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_SERVER_ERROR;
    }

    if (code != 250)
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_NOT_SAVED;
    }
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientVDR::DeleteChannel(unsigned int number)
{

  vector<string> lines;
  int            code;
  char           buffer[1024];

  if (!m_transceiver->IsOpen())
  {
    return PVR_ERROR_SERVER_ERROR;
  }

  pthread_mutex_lock(&m_critSection);

  sprintf(buffer, "LSTC %d", number);

  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    return PVR_ERROR_NOT_SYNC;
  }

  sprintf(buffer, "DELC %d", number);

  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    return PVR_ERROR_NOT_DELETED;
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientVDR::RenameChannel(unsigned int number, CStdString &newname)
{

  CStdString     str_part1;
  CStdString     str_part2;
  vector<string> lines;
  int            code;
  char           buffer[1024];
  int            found;

  if (!m_transceiver->IsOpen())
  {
    return PVR_ERROR_SERVER_ERROR;
  }

  pthread_mutex_lock(&m_critSection);

  sprintf(buffer, "LSTC %d", number);

  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SYNC;
  }

  vector<string>::iterator it = lines.begin();

  string& data(*it);
  CStdString str_result = data;

  found = str_result.find(" ", 0);
  str_part1.assign(str_result, found + 1);
  str_result.erase(0, found + 1);

  /// Channel and provider name
  found = str_result.find(":", 0);
  str_part2.assign(str_result, found);
  str_result.erase(0, found);
  found = str_part2.find(";", 0);

  if (found == -1)
  {
    str_part2 = newname;
  }
  else
  {
    str_part2.erase(0, found);
    str_part2.insert(0, newname);
  }

  sprintf(buffer, "MODC %s %s %s", str_part1.c_str(), str_part2.c_str(), str_result.c_str());

  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SAVED;
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientVDR::MoveChannel(unsigned int number, unsigned int newnumber)
{
  vector<string> lines;
  int            code;
  char           buffer[1024];

  if (!m_transceiver->IsOpen())
      return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  sprintf(buffer, "LSTC %d", number);

  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SYNC;
  }

  sprintf(buffer, "MOVC %d %d", number, newnumber);

  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SAVED;
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

bool PVRClientVDR::GetChannel(unsigned int number, PVR_CHANNEL &channeldata)
{
  vector<string> lines;
  int            code;
  char           buffer[1024];

  if (!m_transceiver->IsOpen())
    return false;

  sprintf(buffer, "LSTC %d", number);
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    if (code != 451)
    {
      return false;
    }
    Sleep(750);
  }

  vector<string>::iterator it = lines.begin();
  string& data(*it);
  CStdString str_result = data;

  cChannel channel;
  channel.Parse(str_result.c_str());

  PVR_CHANNEL tag;
  tag.uid = channel.Sid();
  tag.number = channel.Number();
  tag.name = channel.Name();
  tag.callsign = "";
  tag.iconpath = "";
  tag.encrypted = channel.Ca() ? true : false;
  tag.radio = (channel.Vpid() == 0) && (channel.Apid(0) != 0) ? true : false;
  tag.hide = false;
  tag.recording = false;
  tag.teletext = channel.Tpid() ? true : false;
  tag.bouquet = 0;
  tag.multifeed = false;
  tag.stream_url = "";

  return true;
}
*/

/************************************************************/
/** Record handling **/

int PVRClientVDR::GetNumRecordings(void)
{
  vector<string>  lines;
  int             code;

  if (!m_transceiver->IsOpen())
    return -1;

  pthread_mutex_lock(&m_critSection);

  if (!m_transceiver->SendCommand("STAT records", code, lines))
  {
    XBMC_log(LOG_ERROR, "PCRClient-vdr: Couldn't get recordings count");
    pthread_mutex_unlock(&m_critSection);
    return -1;
  }

  vector<string>::iterator it = lines.begin();

  string& data(*it);
  pthread_mutex_unlock(&m_critSection);
  return atol(data.c_str());
}

PVR_ERROR PVRClientVDR::RequestRecordingsList(PVRHANDLE handle)
{
  vector<string> linesShort;
  int            code;

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  if (!m_transceiver->SendCommand("LSTR", code, linesShort))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  for (vector<string>::iterator it = linesShort.begin(); it != linesShort.end(); it++)
  {
    string& data(*it);
    CStdString str_result = data;

    /* Convert to UTF8 string format */
    if (m_bCharsetConv)
      XBMC_unknown_to_utf8(str_result);

    cRecording recording;
    if (recording.ParseEntryLine(str_result.c_str()))
    {
      char           buffer[1024];
      vector<string> linesDetails;

      sprintf(buffer, "LSTR %d", recording.Index());
      if (!m_transceiver->SendCommand(buffer, code, linesDetails))
        continue;

      for (vector<string>::iterator it2 = linesDetails.begin(); it2 != linesDetails.end(); it2++)
      {
        string& data2(*it2);
        CStdString str_details = data2;

        /* Convert to UTF8 string format */
        if (m_bCharsetConv)
          XBMC_unknown_to_utf8(str_details);

        recording.ParseLine(str_details.c_str());
      }

      PVR_RECORDINGINFO tag;
      tag.index           = recording.Index();
      tag.channelName     = recording.ChannelName();
      tag.framesPerSecond = recording.FramesPerSecond();
      tag.lifetime        = recording.Lifetime();
      tag.priority        = recording.Priority();
      tag.starttime       = recording.StartTime();
      tag.duration        = recording.Duration();
      tag.title           = recording.FileName();
      tag.subtitle        = recording.ShortText();
      tag.description     = recording.Description();

      PVR_transfer_recording_entry(handle, &tag);
    }
  }
  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientVDR::DeleteRecording(const PVR_RECORDINGINFO &recinfo)
{
  vector<string> lines;
  int            code;
  char           buffer[1024];

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  sprintf(buffer, "LSTR %d", recinfo.index);
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 215)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SYNC;
  }

  sprintf(buffer, "DELR %d", recinfo.index);
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_DELETED;
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientVDR::RenameRecording(const PVR_RECORDINGINFO &recinfo, const char *newname)
{
  vector<string> lines;
  int            code;
  char           buffer[1024];

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  sprintf(buffer, "LSTR %d", recinfo.index);
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 215)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SYNC;
  }

  sprintf(buffer, "RENR %d %s", recinfo.index, newname);

  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_DELETED;
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** Timer handling */

int PVRClientVDR::GetNumTimers(void)
{
  vector<string>  lines;
  int             code;

  if (!m_transceiver->IsOpen())
    return -1;

  pthread_mutex_lock(&m_critSection);

  if (!m_transceiver->SendCommand("STAT timers", code, lines))
  {
    XBMC_log(LOG_ERROR, "PCRClient-vdr: Couldn't get timers count");
    pthread_mutex_unlock(&m_critSection);
    return -1;
  }

  vector<string>::iterator it = lines.begin();

  string& data(*it);
  pthread_mutex_unlock(&m_critSection);
  return atol(data.c_str());
}

PVR_ERROR PVRClientVDR::RequestTimerList(PVRHANDLE handle)
{
  vector<string> lines;
  int            code;

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  if (!m_transceiver->SendCommand("LSTT", code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  for (vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
  {
    string& data(*it);
    CStdString str_result = data;

    /**
     * VDR Format given by LSTT:
     * 250-1 1:6:2008-10-27:0013:0055:50:99:Zeiglers wunderbare Welt des Fußballs:
     * 250 2 0:15:2008-10-26:2000:2138:50:99:ZDFtheaterkanal:
     * 250 3 1:6:MTWTFS-:2000:2129:50:99:WDR Köln:
     */

    if (m_bCharsetConv)
      XBMC_unknown_to_utf8(str_result);

    cTimer timer;
    timer.Parse(str_result.c_str());

    PVR_TIMERINFO tag;
    tag.index = timer.Index();
    tag.active = timer.HasFlags(tfActive);
    tag.channelNum = timer.Channel();
    tag.firstday = timer.FirstDay();
    tag.starttime = timer.StartTime();
    tag.endtime = timer.StopTime();
    tag.recording = timer.HasFlags(tfRecording) || timer.HasFlags(tfInstant);
    tag.title = timer.File();
    tag.priority = timer.Priority();
    tag.lifetime = timer.Lifetime();
    tag.repeat = timer.WeekDays() == 0 ? false : true;
    tag.repeatflags = timer.WeekDays();

    PVR_transfer_timer_entry(handle, &tag);
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientVDR::GetTimerInfo(unsigned int timernumber, PVR_TIMERINFO &tag)
{
  vector<string>  lines;
  int             code;
  char            buffer[1024];

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  sprintf(buffer, "LSTT %d", timernumber);
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  vector<string>::iterator it = lines.begin();
  string& data(*it);
  CStdString str_result = data;

  if (m_bCharsetConv)
    XBMC_unknown_to_utf8(str_result);

  cTimer timer;
  timer.Parse(str_result.c_str());

  tag.index = timer.Index();
  tag.active = timer.HasFlags(tfActive);
  tag.channelNum = timer.Channel();
  tag.firstday = timer.FirstDay();
  tag.starttime = timer.StartTime();
  tag.endtime = timer.StopTime();
  tag.recording = timer.HasFlags(tfRecording) || timer.HasFlags(tfInstant);
  tag.title = timer.File();
  tag.priority = timer.Priority();
  tag.lifetime = timer.Lifetime();
  tag.repeat = timer.WeekDays() == 0 ? false : true;
  tag.repeatflags = timer.WeekDays();

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientVDR::AddTimer(const PVR_TIMERINFO &timerinfo)
{
  vector<string> lines;
  int            code;
  char           buffer[1024];

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  cTimer timer(&timerinfo);

  pthread_mutex_lock(&m_critSection);

  if (timerinfo.index == -1)
  {
    sprintf(buffer, "NEWT %s", timer.ToText().c_str());

    if (!m_transceiver->SendCommand(buffer, code, lines))
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_NOT_SAVED;
    }

    if (code != 250)
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_NOT_SYNC;
    }
  }
  else
  {
    // Modified timer
    sprintf(buffer, "LSTT %d", timerinfo.index);
    if (!m_transceiver->SendCommand(buffer, code, lines))
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_SERVER_ERROR;
    }

    if (code != 250)
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_NOT_SYNC;
    }

    sprintf(buffer, "MODT %d %s", timerinfo.index, timer.ToText().c_str());
    if (!m_transceiver->SendCommand(buffer, code, lines))
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_NOT_SAVED;
    }

    if (code != 250)
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_NOT_SYNC;
    }
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientVDR::DeleteTimer(const PVR_TIMERINFO &timerinfo, bool force)
{
  vector<string> lines;
  int            code;
  char           buffer[1024];

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  sprintf(buffer, "LSTT %d", timerinfo.index);
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SYNC;
  }

  lines.erase(lines.begin(), lines.end());

  if (force)
  {
    sprintf(buffer, "DELT %d FORCE", timerinfo.index);
  }
  else
  {
    sprintf(buffer, "DELT %d", timerinfo.index);
  }

  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    vector<string>::iterator it = lines.begin();
    string& data(*it);
    CStdString str_result = data;
    pthread_mutex_unlock(&m_critSection);

    int found = str_result.find("is recording", 0);

    if (found != -1)
    {
      return PVR_ERROR_RECORDING_RUNNING;
    }
    else
    {
      return PVR_ERROR_NOT_DELETED;
    }
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SYNC;
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientVDR::RenameTimer(const PVR_TIMERINFO &timerinfo, const char *newname)
{
  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  PVR_TIMERINFO timerinfo1;
  PVR_ERROR ret = GetTimerInfo(timerinfo.index, timerinfo1);
  if (ret != PVR_ERROR_NO_ERROR)
    return ret;

  timerinfo1.title = newname;
  return UpdateTimer(timerinfo1);
}

PVR_ERROR PVRClientVDR::UpdateTimer(const PVR_TIMERINFO &timerinfo)
{
  vector<string> lines;
  int            code;
  char           buffer[1024];

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  if (timerinfo.index == -1)
    return PVR_ERROR_NOT_SAVED;

  cTimer timer(&timerinfo);

  pthread_mutex_lock(&m_critSection);

  sprintf(buffer, "LSTT %d", timerinfo.index);
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SYNC;
  }

  sprintf(buffer, "MODT %d %s", timerinfo.index, timer.ToText().c_str());
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SAVED;
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SYNC;
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** Live stream handling */

bool PVRClientVDR::OpenLiveStream(unsigned int channel)
{
  if (!m_transceiver->IsOpen())
    return false;

  pthread_mutex_lock(&m_critSection);

  if (!m_transceiver->CanStreamLive(channel))
  {
    pthread_mutex_unlock(&m_critSection);
    return false;
  }

  /* Check if a another stream is opened, if yes first close them */
  if (m_socket_video != INVALID_SOCKET)
  {
    shutdown(m_socket_video, SD_BOTH);

    if (m_iCurrentChannel < 0)
      m_transceiver->AbortStreamRecording();
    else
      m_transceiver->AbortStreamLive();

    closesocket(m_socket_video);
  }

  /* Get Stream socked from VDR Backend */
  m_socket_video = m_transceiver->GetStreamLive(channel);

  /* If received socket is invalid, return */
  if (m_socket_video == INVALID_SOCKET)
  {
    XBMC_log(LOG_ERROR, "PCRClient-vdr: Couldn't get socket for live tv");
    pthread_mutex_unlock(&m_critSection);
    return false;
  }

  m_iCurrentChannel = channel;

  pthread_mutex_unlock(&m_critSection);
  return true;
}

void PVRClientVDR::CloseLiveStream()
{
  if (!m_transceiver->IsOpen())
    return;

  pthread_mutex_lock(&m_critSection);

  if (m_socket_video != INVALID_SOCKET)
  {
    m_transceiver->AbortStreamLive();
    closesocket(m_socket_video);
  }

  pthread_mutex_unlock(&m_critSection);

  return;
}

int PVRClientVDR::ReadLiveStream(BYTE* buf, int buf_size)
{
  if (!m_transceiver->IsOpen())
    return 0;

  if (m_socket_video == INVALID_SOCKET)
    return 0;

  fd_set         set_r, set_e;

  struct timeval tv;

  int            res;

  tv.tv_sec = 10;
  tv.tv_usec = 0;
  FD_ZERO(&set_r);
  FD_ZERO(&set_e);
  FD_SET(m_socket_video, &set_r);
  FD_SET(m_socket_video, &set_e);
  res = select(FD_SETSIZE, &set_r, NULL, &set_e, &tv);

  if (res < 0)
  {
    XBMC_log(LOG_ERROR, "PVRClientVDR::Read - select failed");
    return 0;
  }

  if (res == 0)
  {
    XBMC_log(LOG_ERROR, "PVRClientVDR::Read - timeout waiting for data");
    return 0;
  }

  res = recv(m_socket_video, (char*)buf, (size_t)buf_size, 0);

  if (res < 0)
  {
    XBMC_log(LOG_ERROR, "PVRClientVDR::Read - failed");
    return 0;
  }

  if (res == 0)
  {
    XBMC_log(LOG_ERROR, "PVRClientVDR::Read - eof");
    return 0;
  }

  return res;
}

int PVRClientVDR::GetCurrentClientChannel()
{
  return m_iCurrentChannel;
}

bool PVRClientVDR::SwitchChannel(unsigned int channel)
{
  if (!m_transceiver->IsOpen())
    return false;

  pthread_mutex_lock(&m_critSection);

  if (!m_transceiver->CanStreamLive(channel))
  {
    pthread_mutex_unlock(&m_critSection);
    return false;
  }

  if (m_socket_video != INVALID_SOCKET)
  {
    shutdown(m_socket_video, SD_BOTH);
    m_transceiver->AbortStreamLive();
    closesocket(m_socket_video);
  }

  m_socket_video = m_transceiver->GetStreamLive(channel);

  if (m_socket_video == INVALID_SOCKET)
  {
    XBMC_log(LOG_ERROR, "PCRClient-vdr: Couldn't get socket for live tv");
    pthread_mutex_unlock(&m_critSection);
    return false;
  }

  m_iCurrentChannel = channel;

  pthread_mutex_unlock(&m_critSection);
  return true;
}


/************************************************************/
/** Record stream handling */

bool PVRClientVDR::OpenRecordedStream(const PVR_RECORDINGINFO &recinfo)
{
  if (!m_transceiver->IsOpen())
  {
    return false;
  }

  pthread_mutex_lock(&m_critSection);

  /* Check if a another stream is opened, if yes first close them */

  if (m_socket_video != INVALID_SOCKET)
  {
    shutdown(m_socket_video, SD_BOTH);

    if (m_iCurrentChannel < 0)
      m_transceiver->AbortStreamRecording();
    else
      m_transceiver->AbortStreamLive();

    closesocket(m_socket_video);
  }

  /* Get Stream socked from VDR Backend */
  m_socket_video = m_transceiver->GetStreamRecording(recinfo.index, &currentPlayingRecordBytes, &currentPlayingRecordFrames);

  pthread_mutex_unlock(&m_critSection);

  if (!currentPlayingRecordBytes)
  {
    return false;
  }

  /* If received socket is invalid, return */
  if (m_socket_video == INVALID_SOCKET)
  {
    XBMC_log(LOG_ERROR, "PCRClient-vdr: Couldn't get socket for recording");
    return false;
  }

  m_iCurrentChannel               = -1;
  currentPlayingRecordPosition    = 0;
  return true;
}

void PVRClientVDR::CloseRecordedStream(void)
{

  if (!m_transceiver->IsOpen())
  {
    return;
  }

  pthread_mutex_lock(&m_critSection);

  if (m_socket_video != INVALID_SOCKET)
  {
    m_transceiver->AbortStreamRecording();
    closesocket(m_socket_video);
  }

  pthread_mutex_unlock(&m_critSection);

  return;
}

int PVRClientVDR::ReadRecordedStream(BYTE* buf, int buf_size)
{

  vector<string> lines;
  int            code;
  char           buffer[1024];
  unsigned long  amountReceived;

  if (!m_transceiver->IsOpen() || m_socket_video == INVALID_SOCKET)
    return 0;

  if (currentPlayingRecordPosition + buf_size > currentPlayingRecordBytes)
    return 0;

  sprintf(buffer, "READ %llu %u", currentPlayingRecordPosition, buf_size);
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    return 0;
  }

  vector<string>::iterator it = lines.begin();
  string& data(*it);

  amountReceived = atol(data.c_str());

  fd_set         set_r, set_e;

  struct timeval tv;
  int            res;

  tv.tv_sec = 3;
  tv.tv_usec = 0;

  FD_ZERO(&set_r);
  FD_ZERO(&set_e);
  FD_SET(m_socket_video, &set_r);
  FD_SET(m_socket_video, &set_e);
  res = select(FD_SETSIZE, &set_r, NULL, &set_e, &tv);

  if (res < 0)
  {
    XBMC_log(LOG_ERROR, "PVRClientVDR::ReadRecordedStream - select failed");
    return 0;
  }

  if (res == 0)
  {
    XBMC_log(LOG_ERROR, "PVRClientVDR::ReadRecordedStream - timeout waiting for data");
    return 0;
  }

  res = recv(m_socket_video, (char*)buf, (size_t)buf_size, 0);

  if (res < 0)
  {
    XBMC_log(LOG_ERROR, "PVRClientVDR::ReadRecordedStream - failed");
    return 0;
  }

  if (res == 0)
  {
    XBMC_log(LOG_ERROR, "PVRClientVDR::ReadRecordedStream - eof");
    return 0;
  }

  currentPlayingRecordPosition += res;

  return res;
}

__int64 PVRClientVDR::SeekRecordedStream(__int64 pos, int whence)
{

  if (!m_transceiver->IsOpen())
  {
    return 0;
  }

  __int64 nextPos = currentPlayingRecordPosition;

  switch (whence)
  {

    case SEEK_SET:
      nextPos = pos;
      break;

    case SEEK_CUR:
      nextPos += pos;
      break;

    case SEEK_END:

      if (currentPlayingRecordBytes)
        nextPos = currentPlayingRecordBytes - pos;
      else
        return -1;

      break;

    case SEEK_POSSIBLE:
      return 1;

    default:
      return -1;
  }

  if (nextPos > currentPlayingRecordBytes)
  {
    return 0;
  }

  currentPlayingRecordPosition = nextPos;

  return currentPlayingRecordPosition;
}

__int64 PVRClientVDR::LengthRecordedStream(void)
{
  return currentPlayingRecordBytes;
}

bool PVRClientVDR::TeletextPagePresent(unsigned int channel, unsigned int Page, unsigned int subPage)
{
  vector<string> lines;
  int            code;
  char           buffer[1024];
  unsigned long  amountReceived;

  if (!m_transceiver->IsOpen() || m_socket_video == INVALID_SOCKET)
    return 0;

  sprintf(buffer, "LTXT PRESENT %u %u %u", channel, Page, subPage);
  if (m_transceiver->SendCommand(buffer, code, lines))
  {
    vector<string>::iterator it = lines.begin();
    string& data(*it);
    if (code == 250 && data == "PAGEPRESENT")
      return true;
  }

  return false;
}

bool PVRClientVDR::ReadTeletextPage(BYTE *buf, unsigned int channel, unsigned int Page, unsigned int subPage)
{
  vector<string> lines;
  int            code;
  char           buffer[1024];
  unsigned long  amountReceived;

  if (!m_transceiver->IsOpen() || m_socket_video == INVALID_SOCKET)
    return 0;

  sprintf(buffer, "LTXT GET %u %u %u", channel, Page, subPage);
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    return false;
  }

  vector<string>::iterator it = lines.begin();
  string& data(*it);

  for (int i = 0; i < 40*24+12; i++)
  {
    buf[i] = atol(data.c_str());
    data.erase(0, data.find(" ")+1);
    if (data == "ENDDATA")
      return false;
  }
  return true;
}
