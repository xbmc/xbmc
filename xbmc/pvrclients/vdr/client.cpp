/*
 *      Copyright (C) 2005-2009 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "client.h"
#include "../../addons/include/xbmc_pvr_dll.h"
#include "pvrclient-vdr.h"

using namespace std;

PVRClientVDR *g_client  = NULL;
bool m_bCreated         = false;
ADDON_STATUS curStatus  = STATUS_UNKNOWN;
int g_clientID          = -1;

/* User adjustable settings are saved here.
 * Default values are defined inside client.h
 * and exported to the other source files.
 */
std::string m_sHostname = DEFAULT_HOST;
int m_iPort             = DEFAULT_PORT;
bool m_bOnlyFTA         = DEFAULT_FTA_ONLY;
bool m_bRadioEnabled    = DEFAULT_RADIO;
bool m_bCharsetConv     = DEFAULT_CHARCONV;
int m_iConnectTimeout   = DEFAULT_TIMEOUT;
bool m_bNoBadChannels   = DEFAULT_BADCHANNELS;


//-- Create -------------------------------------------------------------------
// Called after loading of the dll, all steps to become Client functional
// must be performed here.
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS Create(ADDON_HANDLE hdl, int ClientID)
{
  XBMC_register_me(hdl);
  PVR_register_me(hdl);

  //XBMC_log(LOG_DEBUG, "Creating VDR PVR-Client");

  curStatus     = STATUS_UNKNOWN;
  g_client      = new PVRClientVDR();
  g_clientID    = ClientID;

  /* Read setting "host" from settings.xml */
  char * buffer;
  buffer = (char*) malloc (1024);
  buffer[0] = 0; /* Set the end of string */

  if (XBMC_get_setting("host", buffer))
    m_sHostname = buffer;
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC_log(LOG_ERROR, "Couldn't get 'host' setting, falling back to '127.0.0.1' as default");
    m_sHostname = DEFAULT_HOST;
  }
  free (buffer);

  /* Read setting "port" from settings.xml */
  if (!XBMC_get_setting("port", &m_iPort))
  {
    /* If setting is unknown fallback to defaults */
    XBMC_log(LOG_ERROR, "Couldn't get 'port' setting, falling back to '2004' as default");
    m_iPort = DEFAULT_PORT;
  }

  /* Read setting "ftaonly" from settings.xml */
  if (!XBMC_get_setting("ftaonly", &m_bOnlyFTA))
  {
    /* If setting is unknown fallback to defaults */
    XBMC_log(LOG_ERROR, "Couldn't get 'ftaonly' setting, falling back to 'false' as default");
    m_bOnlyFTA = DEFAULT_FTA_ONLY;
  }

  /* Read setting "useradio" from settings.xml */
  if (!XBMC_get_setting("useradio", &m_bRadioEnabled))
  {
    /* If setting is unknown fallback to defaults */
    XBMC_log(LOG_ERROR, "Couldn't get 'useradio' setting, falling back to 'true' as default");
    m_bRadioEnabled = DEFAULT_RADIO;
  }

  /* Read setting "convertchar" from settings.xml */
  if (!XBMC_get_setting("convertchar", &m_bCharsetConv))
  {
    /* If setting is unknown fallback to defaults */
    XBMC_log(LOG_ERROR, "Couldn't get 'convertchar' setting, falling back to 'false' as default");
    m_bCharsetConv = DEFAULT_CHARCONV;
  }

  /* Read setting "timeout" from settings.xml */
  if (!XBMC_get_setting("timeout", &m_iConnectTimeout))
  {
    /* If setting is unknown fallback to defaults */
    XBMC_log(LOG_ERROR, "Couldn't get 'timeout' setting, falling back to %i seconds as default", DEFAULT_TIMEOUT);
    m_iConnectTimeout = DEFAULT_TIMEOUT;
  }

  /* Read setting "ignorechannels" from settings.xml */
  if (!XBMC_get_setting("ignorechannels", &m_bNoBadChannels))
  {
    /* If setting is unknown fallback to defaults */
    XBMC_log(LOG_ERROR, "Couldn't get 'ignorechannels' setting, falling back to 'true' as default");
    m_bNoBadChannels = DEFAULT_BADCHANNELS;
  }

  /* Create connection to streamdev-server */
  if (!g_client->Connect())
    curStatus = STATUS_LOST_CONNECTION;
  else
    curStatus = STATUS_OK;

  m_bCreated = true;
  return curStatus;
}

//-- Destroy ------------------------------------------------------------------
// Used during destruction of the client, all steps to do clean and safe Create
// again must be done.
//-----------------------------------------------------------------------------
extern "C" void Destroy()
{
  if (m_bCreated)
  {
    g_client->Disconnect();

    delete g_client;
    g_client = NULL;

    m_bCreated = false;
  }
  curStatus = STATUS_UNKNOWN;
}

//-- GetStatus ----------------------------------------------------------------
// Report the current Add-On Status to XBMC
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS GetStatus()
{
  return curStatus;
}

//-- HasSettings --------------------------------------------------------------
// Report "true", yes this AddOn have settings
//-----------------------------------------------------------------------------
extern "C" bool HasSettings()
{
  return true;
}

//-- SetSetting ---------------------------------------------------------------
// Called everytime a setting is changed by the user and to inform AddOn about
// new setting and to do required stuff to apply it.
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS SetSetting(const char *settingName, const void *settingValue)
{
  string str = settingName;
  if (str == "host")
  {
    string tmp_sHostname;
    XBMC_log(LOG_INFO, "Changed Setting 'host' from %s to %s", m_sHostname.c_str(), (const char*) settingValue);
    tmp_sHostname = m_sHostname;
    m_sHostname = (const char*) settingValue;
    if (tmp_sHostname != m_sHostname)
      return STATUS_NEED_RESTART;
  }
  else if (str == "port")
  {
    XBMC_log(LOG_INFO, "Changed Setting 'port' from %u to %u", m_iPort, *(int*) settingValue);
    if (m_iPort != *(int*) settingValue)
    {
      m_iPort = *(int*) settingValue;
      return STATUS_NEED_RESTART;
    }
  }
  else if (str == "ftaonly")
  {
    XBMC_log(LOG_INFO, "Changed Setting 'ftaonly' from %u to %u", m_bOnlyFTA, *(bool*) settingValue);
    m_bOnlyFTA = *(bool*) settingValue;
  }
  else if (str == "useradio")
  {
    XBMC_log(LOG_INFO, "Changed Setting 'useradio' from %u to %u", m_bRadioEnabled, *(bool*) settingValue);
    m_bRadioEnabled = *(bool*) settingValue;
  }
  else if (str == "convertchar")
  {
    XBMC_log(LOG_INFO, "Changed Setting 'convertchar' from %u to %u", m_bCharsetConv, *(bool*) settingValue);
    m_bCharsetConv = *(bool*) settingValue;
  }
  else if (str == "timeout")
  {
    XBMC_log(LOG_INFO, "Changed Setting 'timeout' from %u to %u", m_iConnectTimeout, *(int*) settingValue);
    m_iConnectTimeout = *(int*) settingValue;
  }
  else if (str == "ignorechannels")
  {
    XBMC_log(LOG_INFO, "Changed Setting 'ignorechannels' from %u to %u", m_bNoBadChannels, *(bool*) settingValue);
    m_bNoBadChannels = *(bool*) settingValue;
  }

  return STATUS_OK;
}

//-- GetProperties ------------------------------------------------------------
// Tell XBMC our requirements
//-----------------------------------------------------------------------------
extern "C" PVR_ERROR GetProperties(PVR_SERVERPROPS* props)
{
  return g_client->GetProperties(props);
}

//-- GetBackendName -----------------------------------------------------------
// Return the Name of the Backend
//-----------------------------------------------------------------------------
extern "C" const char * GetBackendName()
{
  return g_client->GetBackendName();
}

//-- GetBackendVersion --------------------------------------------------------
// Return the Version of the Backend as String
//-----------------------------------------------------------------------------
extern "C" const char * GetBackendVersion()
{
  return g_client->GetBackendVersion();
}

//-- GetConnectionString ------------------------------------------------------
// Return a String with connection info, if available
//-----------------------------------------------------------------------------
extern "C" const char * GetConnectionString()
{
  return g_client->GetConnectionString();
}

//-- GetDriveSpace ------------------------------------------------------------
// Return the Total and Free Drive space on the PVR Backend
//-----------------------------------------------------------------------------
extern "C" PVR_ERROR GetDriveSpace(long long *total, long long *used)
{
  return g_client->GetDriveSpace(total, used);
}

extern "C" int GetNumBouquets()
{
  return 0;
}

extern "C" PVR_ERROR RequestEPGForChannel(PVRHANDLE handle, unsigned int number, time_t start, time_t end)
{
  return g_client->RequestEPGForChannel(number, handle, start, end);
}

extern "C" int GetNumChannels()
{
  return g_client->GetNumChannels();
}

extern "C" PVR_ERROR RequestChannelList(PVRHANDLE handle, int radio)
{
  return g_client->RequestChannelList(handle, radio);
}
/*
extern "C" PVR_ERROR GetChannelSettings(cPVRChannelInfoTag *result)
{
  return g_client->GetChannelSettings(result);
}

extern "C" PVR_ERROR UpdateChannelSettings(const cPVRChannelInfoTag &chaninfo)
{
  return g_client->UpdateChannelSettings(chaninfo);
}

extern "C" PVR_ERROR AddChannel(const cPVRChannelInfoTag &info)
{
  return g_client->AddChannel(info);
}

extern "C" PVR_ERROR DeleteChannel(unsigned int number)
{
  return g_client->DeleteChannel(number);
}

extern "C" PVR_ERROR RenameChannel(unsigned int number, CStdString &newname)
{
  return g_client->RenameChannel(number, newname);
}

extern "C" PVR_ERROR MoveChannel(unsigned int number, unsigned int newnumber)
{
  return g_client->MoveChannel(number, newnumber);
}
*/
extern "C" int GetNumRecordings(void)
{
  return g_client->GetNumRecordings();
}

extern "C" PVR_ERROR RequestRecordingsList(PVRHANDLE handle)
{
  return g_client->RequestRecordingsList(handle);
}

extern "C" PVR_ERROR DeleteRecording(const PVR_RECORDINGINFO &recinfo)
{
  return g_client->DeleteRecording(recinfo);
}

extern "C" PVR_ERROR RenameRecording(const PVR_RECORDINGINFO &recinfo, CStdString &newname)
{
  return g_client->RenameRecording(recinfo, newname);
}

extern "C" int GetNumTimers(void)
{
  return g_client->GetNumTimers();
}

extern "C" PVR_ERROR RequestTimerList(PVRHANDLE handle)
{
  return g_client->RequestTimerList(handle);
}

extern "C" PVR_ERROR AddTimer(const PVR_TIMERINFO &timerinfo)
{
  return g_client->AddTimer(timerinfo);
}

extern "C" PVR_ERROR DeleteTimer(const PVR_TIMERINFO &timerinfo, bool force)
{
  return g_client->DeleteTimer(timerinfo, force);
}

extern "C" PVR_ERROR RenameTimer(const PVR_TIMERINFO &timerinfo, const char *newname)
{
  return g_client->RenameTimer(timerinfo, newname);
}

extern "C" PVR_ERROR UpdateTimer(const PVR_TIMERINFO &timerinfo)
{
  return g_client->UpdateTimer(timerinfo);
}

extern "C" bool OpenLiveStream(unsigned int channel)
{
  return g_client->OpenLiveStream(channel);
}

extern "C" void CloseLiveStream()
{
  return g_client->CloseLiveStream();
}

extern "C" int ReadLiveStream(BYTE* buf, int buf_size)
{
  return g_client->ReadLiveStream(buf, buf_size);
}

extern "C" int GetCurrentClientChannel()
{
  return g_client->GetCurrentClientChannel();
}

extern "C" bool SwitchChannel(unsigned int channel)
{
  return g_client->SwitchChannel(channel);
}

extern "C" bool OpenRecordedStream(const PVR_RECORDINGINFO &recinfo)
{
  return g_client->OpenRecordedStream(recinfo);
}

extern "C" void CloseRecordedStream(void)
{
  return g_client->CloseRecordedStream();
}

extern "C" int ReadRecordedStream(BYTE* buf, int buf_size)
{
  return g_client->ReadRecordedStream(buf, buf_size);
}

extern "C" __int64 SeekRecordedStream(__int64 pos, int whence)
{
  return g_client->SeekRecordedStream(pos, whence);
}

extern "C" __int64 LengthRecordedStream(void)
{
  return g_client->LengthRecordedStream();
}

extern "C" bool TeletextPagePresent(unsigned int channel, unsigned int Page, unsigned int subPage)
{
  return g_client->TeletextPagePresent(channel, Page, subPage);
}

extern "C" bool ReadTeletextPage(BYTE* buf, unsigned int channel, unsigned int Page, unsigned int subPage)
{
  return g_client->ReadTeletextPage(buf, channel, Page, subPage);
}
