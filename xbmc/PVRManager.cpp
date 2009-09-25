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

#include "Application.h"
#include "GUISettings.h"
#include "Util.h"
#include "GUIWindowTV.h"
#include "GUIWindowManager.h"
#include "utils/GUIInfoManager.h"
#include "settings/AddonSettings.h"
#include "PVRManager.h"
#include "pvrclients/PVRClientFactory.h"
#include "MusicInfoTag.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "FileSystem/File.h"
#include "StringUtils.h"

/* GUI Messages includes */
#include "GUIDialogOK.h"

#define CHANNELCHECKDELTA     600 // seconds before checking for changes inside channels list
#define TIMERCHECKDELTA       300 // seconds before checking for changes inside timers list
#define RECORDINGCHECKDELTA   450 // seconds before checking for changes inside recordings list

using namespace std;
using namespace MUSIC_INFO;
using namespace XFILE;
using namespace ADDON;

/**********************************************************************
/* BEGIN OF CLASS **___ CPVRTimeshiftRcvr ____________________________*
/**                                                                  **/

/********************************************************************
/* CPVRTimeshiftRcvr constructor
/*
/* It creates a thread based PVR stream receiver which fill a buffer
/* file from where we can read the stream later.
/********************************************************************/
CPVRTimeshiftRcvr::CPVRTimeshiftRcvr()
{
  /* Set initial data */
  m_pFile         = NULL;
  m_MaxSizeStatic = g_guiSettings.GetInt("pvrplayback.timeshiftcache") * 1024 * 1024;

  /* Create the file class for writing */
  m_pFile = new CFile();
  if (!m_pFile)
  {
    return;
  }

  /* Open the buffer file for writing with the overwrite flag is set. */
  CStdString filename = g_guiSettings.GetString("pvrplayback.timeshiftpath")+"/.timeshift_cache.ts";
  if (!m_pFile->OpenForWrite(filename, true))
  {
    CLog::Log(LOGERROR,"PVR: Can't open timeshift receiver file %s for writing", filename.c_str());
    delete m_pFile;
    m_pFile = NULL;
    return;
  }

  InitializeCriticalSection(&m_critSection);
  CLog::Log(LOGDEBUG,"PVR: created timeshift receiver");
}

/********************************************************************
/* CPVRTimeshiftRcvr destructor
/*
/* It stopping the thread, delete the buffer file and destroy this.
/* class
/********************************************************************/
CPVRTimeshiftRcvr::~CPVRTimeshiftRcvr()
{
  /* Stop the receiving thread */
  StopThread();

  /* Close and delete the buffer file */
  if (m_pFile)
  {
    m_pFile->Close();
    m_pFile->Delete(g_guiSettings.GetString("pvrplayback.timeshiftpath")+"/.timeshift_cache.ts");
    CLog::Log(LOGDEBUG,"PVR: deleted Timeshift receiver buffer file");
    delete m_pFile;
    m_pFile = NULL;
  }

  DeleteCriticalSection(&m_critSection);
  CLog::Log(LOGDEBUG,"PVR: destroyed Timeshift receiver");
}

/********************************************************************
/* CPVRTimeshiftRcvr StartReceiver
/*
/* It start the the receiving thread and return true if everything is
/* ok. IPVRClient of the current PVRClient must be passed to this
/* function.
/********************************************************************/
bool CPVRTimeshiftRcvr::StartReceiver(IPVRClient *client)
{
  if (!m_pFile)
  {
    CLog::Log(LOGERROR,"PVR: Can't start timeshift receiver without file");
    return false;
  }

  /* Reset all counters and flags */
  m_client    = client;
  m_MaxSize   = m_MaxSizeStatic;
  m_written   = 0;
  m_position  = 0;
  m_Started   = timeGetTime();
  m_pFile->Seek(0);

  /* Clear the timestamp table */
  m_Timestamps.clear();

  /* Create the reading thread */
  Create();
  SetName("PVR Timeshift receiver");
  SetPriority(5);

  CLog::Log(LOGDEBUG,"PVR: Timeshift receiver started");
  return true;
}

/********************************************************************
/* CPVRTimeshiftRcvr StopReceiver
/*
/* It stop the the receiving thread
/********************************************************************/
void CPVRTimeshiftRcvr::StopReceiver()
{
  StopThread();

  CLog::Log(LOGDEBUG,"PVR: Timeshift receiver stopped");
}

/********************************************************************
/* CPVRTimeshiftRcvr GetMaxSize
/*
/* Return the maximum size for the buffer file, is required for wrap
/* around during buffer read.
/********************************************************************/
__int64 CPVRTimeshiftRcvr::GetMaxSize()
{
  return m_MaxSize;
}

/********************************************************************
/* CPVRTimeshiftRcvr GetWritten
/*
/* Return how many bytes are totally readed from the client and writed
/* to the buffer file.
/********************************************************************/
__int64 CPVRTimeshiftRcvr::GetWritten()
{
  return m_written;
}

/********************************************************************
/* CPVRTimeshiftRcvr GetDuration
/*
/* Return how many time in milliseconds (ms) is stored in the buffer
/* file, required for seeking and progress bar.
/********************************************************************/
DWORD CPVRTimeshiftRcvr::GetDuration()
{
  if (m_Timestamps.size() > 1)
    return m_Timestamps[m_Timestamps.size()-1].time - m_Timestamps[0].time;
  else
    return 0;
}

/********************************************************************
/* CPVRTimeshiftRcvr GetDurationString
/*
/* Same as GetDuration but return a string which is used by the
/* GUIInfoManager label "pvr.timeshiftduration".
/********************************************************************/
const char* CPVRTimeshiftRcvr::GetDurationString()
{
  StringUtils::SecondsToTimeString(GetDuration()/1000, m_DurationStr, TIME_FORMAT_GUESS);
  return m_DurationStr.c_str();
}

/********************************************************************
/* CPVRTimeshiftRcvr GetTimeTotal
/*
/* Return how long in milliseconds (ms) the thread is running.
/* Also required for seeking.
/********************************************************************/
DWORD CPVRTimeshiftRcvr::GetTimeTotal()
{
  if (m_Timestamps.size() > 0)
    return m_Timestamps[m_Timestamps.size()-1].time;
  else
    return 0;
}

/********************************************************************
/* CPVRTimeshiftRcvr TimeToPos
/*
/* Convert a timestamp in milliseconds (ms) to a byte position.
/* It is based upon a std::deque list which stores reference data
/* to a time for each performed read.
/* It also write the new time to "timeRet" which is passed as pointer
/* and set the wrapback label which indicates that the data is in the
/* upper part behind the current write position.
/********************************************************************/
__int64 CPVRTimeshiftRcvr::TimeToPos(DWORD time, DWORD *timeRet, bool *wrapback)
{
  __int64 pos = 0;

  EnterCriticalSection(&m_critSection);

  if (m_Timestamps.size() > 0)
  {
    /* Ignore the last 1,5 seconds */
    if (time < m_Timestamps[m_Timestamps.size()-1].time - 1500)
    {
      std::deque<STimestamp>::iterator Timestamp = m_Timestamps.begin();
      for(;Timestamp != m_Timestamps.end();Timestamp++)
      {
        if (Timestamp->time > time)
        {
          pos       = Timestamp->pos;
          *timeRet  = Timestamp->time;
          if (pos > m_position)
            *wrapback = true;
          break;
        }
      }
    }
  }

  LeaveCriticalSection(&m_critSection);
  return pos;
}

/********************************************************************
/* CPVRTimeshiftRcvr Process
/*
/* The Main thread. It read the data from the client and write it to
/* the buffer file. If the Maximum size of the file is arrived, it
/* swaps around and write for beginning of the file.
/* For this reason, we can only seek back to the maximum size of the
/* file.
/********************************************************************/
void CPVRTimeshiftRcvr::Process()
{
  CLog::Log(LOGDEBUG,"PVR: Timeshift receiver thread started");

  bool wraparound = false;

  while (!m_bStop)
  {
    /* Create the timestamp for this read */
    STimestamp timestamp;
    timestamp.pos   = m_position;
    timestamp.time  = timeGetTime() - m_Started;

    /* Read the data from the stream */
    int ret = m_client->ReadLiveStream(buf, sizeof(buf));
    if (ret > 0)
    {
      EnterCriticalSection(&m_critSection);

      /* If read was ok, store the timestamp inside the list */
      m_Timestamps.push_back(timestamp);
      /* If we have reached the first wrap around, we delete the oldest
        timestamp in the list. The data is no more present in the buffer */
      if (wraparound)
        m_Timestamps.pop_front();

      /* Save current position and increase totally written bytes */
      m_position += m_pFile->Write(buf, ret);
      m_written  += ret;

      LeaveCriticalSection(&m_critSection);
    }

    /* Check if we reached the maximum size of the buffer file */
    if (m_position >= m_MaxSizeStatic)
    {
      /* If yes we wrap around and the file position is set to null */
      wraparound = true;
      m_MaxSize  = m_position;
      m_position = 0;
      m_pFile->Seek(0);
    }

    Sleep(10);
  }

  CLog::Log(LOGDEBUG,"PVR: Timeshift receiver thread ended");
}



/**********************************************************************
/* BEGIN OF CLASS **___ CPVRManager __________________________________*
/**                                                                  **/

/********************************************************************
/* CPVRManager constructor
/*
/* It creates the PVRManager, which mostly handle all PVR related
/* operations for XBMC
/********************************************************************/
CPVRManager::CPVRManager()
{
  m_hasRecordings = false;
  m_isRecording   = false;
  m_hasTimers     = false;

  InitializeCriticalSection(&m_critSection);
  CLog::Log(LOGDEBUG,"PVR: created");
}

/********************************************************************
/* CPVRManager destructor
/*
/* Destroy this class
/********************************************************************/
CPVRManager::~CPVRManager()
{
  DeleteCriticalSection(&m_critSection);
  CLog::Log(LOGDEBUG,"PVR: destroyed");
}














void CPVRManager::Start()
{
  /* First stop and remove any clients */
  if (!m_clients.empty())
    Stop();

  /* Check if TV is enabled under Settings->Video->TV->Enable */
  if (!g_guiSettings.GetBool("pvrmanager.enabled"))
    return;

  CLog::Log(LOGNOTICE, "PVR: PVRManager starting");

  /* Reset Member variables and System Info swap counters */
  m_CurrentGroupID          = -1;
  m_currentPlayingChannel   = NULL;
  m_currentPlayingRecording = NULL;
  m_PreviousChannel[0]      = 1;
  m_PreviousChannel[1]      = 1;
  m_PreviousChannelIndex    = 0;
  m_infoToggleStart         = NULL;
  m_infoToggleCurrent       = 0;
  m_TimeshiftReceiver       = NULL;
  m_pTimeshiftFile          = NULL;
  m_timeshiftInt            = false;

  /* Discover, load and create chosen Client add-on's. */
  g_addonmanager.RegisterAddonCallback(ADDON_PVRDLL, this);
  if (!LoadClients())
  {
    CLog::Log(LOGERROR, "PVR: couldn't load clients");
    return;
  }

  /* Get TV Channels from Backends */
  PVRChannelsTV.Load(false);

  /* Get Radio Channels from Backends */
  PVRChannelsRadio.Load(true);

  /* Get Timers from Backends */
  PVRTimers.Load();

  /* Get Recordings from Backend */
  PVRRecordings.Load();

  /* Get Epg's from Backend */
  cPVREpgs::Load();

  /* Create the supervisor thread to do all background activities */
  Create();
  SetName("XBMC PVR Supervisor");
  SetPriority(-15);

  CLog::Log(LOGNOTICE, "PVR: PVRManager started. Clients loaded = %u", m_clients.size());
  return;
}

void CPVRManager::Stop()
{
  CLog::Log(LOGNOTICE, "PVR: PVRManager stoping");
  StopThread();

  for (unsigned int i = 0; i < m_clients.size(); i++)
  {
    delete m_clients[i];
  }
  m_clients.clear();
  m_clientsProps.clear();

  if (m_pTimeshiftFile)
  {
    delete m_pTimeshiftFile;
    m_pTimeshiftFile = NULL;
  }

  return;
}

bool CPVRManager::LoadClients()
{
  /* Get all PVR Add on's */
  VECADDONS *addons = g_addonmanager.GetAddonsFromType(ADDON_PVRDLL);

  /* Make sure addon's are loaded */
  if (addons == NULL || addons->empty())
    return false;

  m_database.Open();

  /* load the clients */
  CPVRClientFactory factory;
  for (unsigned i=0; i < addons->size(); i++)
  {
    const CAddon& clientAddon = addons->at(i);

    if (clientAddon.m_disabled) // ignore disabled addons
      continue;

    /* Add client to TV-Database to identify different backend types,
     * if client is already added his id is given.
     */
    long clientID = m_database.AddClient(clientAddon.m_strName, clientAddon.m_guid);
    if (clientID == -1)
    {
      CLog::Log(LOGERROR, "PVR: Can't Add/Get PVR Client '%s' to to TV Database", clientAddon.m_strName.c_str());
      continue;
    }

    /* Load the Client library's and inside them into Client list if
     * success. Client initialization is also performed during loading.
     */
    IPVRClient *client = factory.LoadPVRClient(clientAddon, clientID, this);
    if (client)
    {
      m_clients.insert(std::make_pair(client->GetID(), client));
    }
  }

  m_database.Close();

  // Request each client's basic properties
  GetClientProperties();

  return !m_clients.empty();
}

unsigned long CPVRManager::GetFirstClientID()
{
  CLIENTMAPITR itr = m_clients.begin();
  return m_clients[(*itr).first]->GetID();
}

void CPVRManager::GetClientProperties()
{
  m_clientsProps.clear();
  CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    GetClientProperties((*itr).first);
    itr++;
  }
}

void CPVRManager::GetClientProperties(long clientID)
{
  PVR_SERVERPROPS props;
  if (m_clients[clientID]->GetProperties(&props) == PVR_ERROR_NO_ERROR)
  {
    // store the client's properties
    m_clientsProps.insert(std::make_pair(clientID, props));
  }
}

void CPVRManager::OnClientMessage(const long clientID, const PVR_EVENT clientEvent, const char* msg)
{
  /* here the manager reacts to messages sent from any of the clients via the IPVRClientCallback */
  switch (clientEvent) {
    case PVR_EVENT_UNKNOWN:
      CLog::Log(LOGDEBUG, "%s - PVR: client_%ld unknown event : %s", __FUNCTION__, clientID, msg);
      break;

    case PVR_EVENT_TIMERS_CHANGE:
	  {
        CLog::Log(LOGDEBUG, "%s - PVR: client_%ld timers changed", __FUNCTION__, clientID);
        SyncInfo();

        CGUIWindowTV *pTVWin = (CGUIWindowTV *)m_gWindowManager.GetWindow(WINDOW_TV);
	    if (pTVWin)
    	  pTVWin->UpdateData(TV_WINDOW_TIMERS);
	  }
      break;

    case PVR_EVENT_RECORDINGS_CHANGE:
	  {
        CLog::Log(LOGDEBUG, "%s - PVR: client_%ld recording list changed", __FUNCTION__, clientID);
        SyncInfo();

        CGUIWindowTV *pTVWin = (CGUIWindowTV *)m_gWindowManager.GetWindow(WINDOW_TV);
	    if (pTVWin)
    	  pTVWin->UpdateData(TV_WINDOW_RECORDINGS);
	  }
      break;

    case PVR_EVENT_CHANNELS_CHANGE:
	  {
        CLog::Log(LOGDEBUG, "%s - PVR: client_%ld channel list changed", __FUNCTION__, clientID);
        SyncInfo();

        CGUIWindowTV *pTVWin = (CGUIWindowTV *)m_gWindowManager.GetWindow(WINDOW_TV);
	    if (pTVWin)
		{
    	  pTVWin->UpdateData(TV_WINDOW_CHANNELS_TV);
		  pTVWin->UpdateData(TV_WINDOW_CHANNELS_RADIO);
		}
	  }
      break;
    default:
      break;
  }
}

ADDON_STATUS CPVRManager::SetSetting(const CAddon* addon, const char *settingName, const void *settingValue)
{
  if (!addon)
    return STATUS_UNKNOWN;

  CLog::Log(LOGINFO, "PVR: set setting of clientName: %s, settingName: %s", addon->m_strName.c_str(), settingName);
  CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    if (m_clients[(*itr).first]->m_guid == addon->m_guid)
    {
      if (m_clients[(*itr).first]->m_strName == addon->m_strName)
      {
        return m_clients[(*itr).first]->SetSetting(settingName, settingValue);
      }
    }
    itr++;
  }
  return STATUS_UNKNOWN;
}

bool CPVRManager::RequestRestart(const CAddon* addon, bool datachanged)
{
  if (!addon)
    return false;

  CLog::Log(LOGINFO, "PVR: requested restart of clientName:%s, clientGUID:%s", addon->m_strName.c_str(), addon->m_guid.c_str());
  CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    if (m_clients[(*itr).first]->m_guid == addon->m_guid)
    {
      if (m_clients[(*itr).first]->m_strName == addon->m_strName)
      {
        CLog::Log(LOGINFO, "PVR: restarting clientName:%s, clientGUID:%s", addon->m_strName.c_str(), addon->m_guid.c_str());
        StopThread();
        if (m_clients[(*itr).first]->ReInit())
        {
          /* Get TV Channels from Backends */
          PVRChannelsTV.Update();

          /* Get Radio Channels from Backends */
          PVRChannelsRadio.Update();

          /* Get Timers from Backends */
          PVRTimers.Update();

          /* Get Recordings from Backend */
          PVRRecordings.Update();
        }
        Create();
      }
    }
    itr++;
  }
  return true;
}

bool CPVRManager::RequestRemoval(const CAddon* addon)
{
  if (!addon)
    return false;

  CLog::Log(LOGINFO, "PVR: requested removal of clientName:%s, clientGUID:%s", addon->m_strName.c_str(), addon->m_guid.c_str());
  CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    if (m_clients[(*itr).first]->m_guid == addon->m_guid)
    {
      if (m_clients[(*itr).first]->m_strName == addon->m_strName)
      {
        CLog::Log(LOGINFO, "PVR: removing clientName:%s, clientGUID:%s", addon->m_strName.c_str(), addon->m_guid.c_str());
        m_clients[(*itr).first]->Remove();
        m_clients.erase((*itr).first);
        return true;
      }
    }
    itr++;
  }

  return false;
}

int CPVRManager::GetGroupList(CFileItemList* results)
{
  for (unsigned int i = 0; i < m_channel_group.size(); i++)
  {
    CFileItemPtr group(new CFileItem(m_channel_group[i].m_Title));
    group->m_strTitle = m_channel_group[i].m_Title;
    group->m_strPath.Format("%i", m_channel_group[i].m_ID);
    results->Add(group);
  }
  return m_channel_group.size();
}

void CPVRManager::AddGroup(const CStdString &newname)
{
  EnterCriticalSection(&m_critSection);
  m_database.Open();

  m_database.AddGroup(GetFirstClientID(), newname);
  m_database.GetGroupList(GetFirstClientID(), &m_channel_group);

  m_database.Close();
  LeaveCriticalSection(&m_critSection);
}

bool CPVRManager::RenameGroup(unsigned int GroupId, const CStdString &newname)
{
  EnterCriticalSection(&m_critSection);
  m_database.Open();

  m_database.RenameGroup(GetFirstClientID(), GroupId, newname);
  m_database.GetGroupList(GetFirstClientID(), &m_channel_group);

  m_database.Close();
  LeaveCriticalSection(&m_critSection);
  return true;
}

bool CPVRManager::DeleteGroup(unsigned int GroupId)
{
  EnterCriticalSection(&m_critSection);
//  m_database.Open();
//
//  m_database.DeleteGroup(GetFirstClientID(), GroupId);
//
//  for (unsigned int i = 0; i < PVRChannelsTV.size(); i++)
//  {
//    if (PVRChannelsTV[i].GroupID() == GroupId)
//    {
//      PVRChannelsTV[i].m_iGroupID = 0;
//      m_database.UpdateChannel(GetFirstClientID(), PVRChannelsTV[i]);
//    }
//  }
//  for (unsigned int i = 0; i < PVRChannelsRadio.size(); i++)
//  {
//    if (PVRChannelsRadio[i].GroupID() == GroupId)
//    {
//      PVRChannelsRadio[i].m_iGroupID = 0;
//      m_database.UpdateChannel(GetFirstClientID(), PVRChannelsRadio[i]);
//    }
//  }
//  m_database.GetGroupList(GetFirstClientID(), &m_channel_group);
//  m_database.Close();
  LeaveCriticalSection(&m_critSection);
  return true;
}

bool CPVRManager::ChannelToGroup(unsigned int number, unsigned int GroupId, bool radio)
{
//  if (!radio)
//  {
//    if (((int) number <= PVRChannelsTV.size()+1) && (number != 0))
//    {
//      EnterCriticalSection(&m_critSection);
//      m_database.Open();
//      PVRChannelsTV[number-1].m_iGroupID = GroupId;
//      m_database.UpdateChannel(GetFirstClientID(), PVRChannelsTV[number-1]);
//      m_database.Close();
//      LeaveCriticalSection(&m_critSection);
//      return true;
//    }
//  }
//  else
//  {
//    if (((int) number <= PVRChannelsRadio.size()+1) && (number != 0))
//    {
//      EnterCriticalSection(&m_critSection);
//      m_database.Open();
//      PVRChannelsRadio[number-1].m_iGroupID = GroupId;
//      m_database.UpdateChannel(GetFirstClientID(), PVRChannelsRadio[number-1]);
//      m_database.Close();
//      LeaveCriticalSection(&m_critSection);
//      return true;
//    }
//  }
  return false;
}

int CPVRManager::GetPrevGroupID(int current_group_id)
{
  if (m_channel_group.size() == 0)
    return -1;

  if ((current_group_id == -1) || (current_group_id == 0))
    return m_channel_group[m_channel_group.size()-1].m_ID;

  for (unsigned int i = 0; i < m_channel_group.size(); i++)
  {
    if (current_group_id == m_channel_group[i].m_ID)
    {
      if (i != 0)
        return m_channel_group[i-1].m_ID;
      else
        return -1;
    }
  }
  return -1;
}

int CPVRManager::GetNextGroupID(int current_group_id)
{
  unsigned int i = 0;

  if (m_channel_group.size() == 0)
    return -1;

  if ((current_group_id == 0) || (current_group_id == -1))
    return m_channel_group[0].m_ID;

  if (m_channel_group.size() == 0)
    return -1;

  for (; i < m_channel_group.size(); i++)
  {
    if (current_group_id == m_channel_group[i].m_ID)
      break;
  }

  if (i >= m_channel_group.size()-1)
    return -1;
  else
    return m_channel_group[i+1].m_ID;
}

CStdString CPVRManager::GetGroupName(int GroupId)
{
  if (GroupId == -1)
    return g_localizeStrings.Get(593);

  for (unsigned int i = 0; i < m_channel_group.size(); i++)
  {
    if (GroupId == m_channel_group[i].m_ID)
      return m_channel_group[i].m_Title;
  }

  return g_localizeStrings.Get(593);
}

int CPVRManager::GetFirstChannelForGroupID(int GroupId, bool radio)
{
  if (GroupId == -1)
    return 1;

  if (!radio)
  {
    for (unsigned int i = 0; i < PVRChannelsTV.size(); i++)
    {
      if (PVRChannelsTV[i].GroupID() == GroupId)
        return i+1;
    }
  }
  else
  {
    for (unsigned int i = 0; i < PVRChannelsRadio.size(); i++)
    {
      if (PVRChannelsRadio[i].GroupID() == GroupId)
        return i+1;
    }
  }
  return 1;
}

void CPVRManager::SetPlayingGroup(int GroupId)
{
  m_CurrentGroupID = GroupId;
}

int CPVRManager::GetPlayingGroup()
{
  return m_CurrentGroupID;
}

void CPVRManager::SyncInfo()
{
  PVRRecordings.GetNumRecordings() > 0 ? m_hasRecordings = true : m_hasRecordings = false;
  PVRTimers.GetNumTimers()     > 0 ? m_hasTimers     = true : m_hasTimers = false;
  m_isRecording = false;

  if (m_hasTimers)
  {
    cPVRTimerInfoTag *nextRec = PVRTimers.GetNextActiveTimer();

    m_nextRecordingTitle    = nextRec->Title();
    m_nextRecordingChannel  = PVRChannelsTV.GetNameForChannel(nextRec->Number());
    m_nextRecordingDateTime = nextRec->Start().GetAsLocalizedDateTime(false, false);

    if (nextRec->IsRecording() == true)
    {
      m_isRecording = true;
      CLog::Log(LOGDEBUG, "%s - PVR: next timer is currently recording", __FUNCTION__);
    }
    else
    {
      m_isRecording = false;
    }
  }

  if (m_isRecording)
  {
    m_nowRecordingTitle = m_nextRecordingTitle;
    m_nowRecordingDateTime = m_nextRecordingDateTime;
    m_nowRecordingChannel = m_nextRecordingChannel;
    CLog::Log(LOGDEBUG, "%s - PVR: data of active recording is used: '%s', '%s', '%s'", __FUNCTION__,
                        m_nowRecordingTitle.c_str(),
                        m_nextRecordingDateTime.c_str(),
                        m_nextRecordingChannel.c_str());
  }
  else
  {
    m_nowRecordingTitle.clear();
    m_nowRecordingDateTime.clear();
    m_nowRecordingChannel.clear();
  }
}










/*************************************************************/
/** INTERNAL FUNCTIONS                                      **/
/*************************************************************/

/********************************************************************
/* CPVRManager CreateInternalTimeshift
/*
/* Create the internal timeshift receiver and the file class for
/* reading.
/********************************************************************/
bool CPVRManager::CreateInternalTimeshift()
{
  /* Delete the timeshift receiving class if present */
  if (m_TimeshiftReceiver)
    delete m_TimeshiftReceiver;
  if (m_pTimeshiftFile)
    delete m_pTimeshiftFile;

  /* Create the receiving class to perform independent stream reading and.
     Open the stream buffer file*/
  unsigned int flags  = READ_NO_CACHE;
  m_TimeshiftReceiver = new CPVRTimeshiftRcvr();
  m_pTimeshiftFile    = new CFile();
  if (!m_pTimeshiftFile->Open(g_guiSettings.GetString("pvrplayback.timeshiftpath")+"/.timeshift_cache.ts", flags))
  {
    /* If open fails return with timeshift disabled */
    delete m_TimeshiftReceiver;
    delete m_pTimeshiftFile;
    m_TimeshiftReceiver = NULL;
    m_pTimeshiftFile    = NULL;
    return false;
  }
  return true;
}


/*************************************************************/
/** PVRManager Update and control thread                    **/
/*************************************************************/

void CPVRManager::Process()
{
  DWORD Now = timeGetTime();
  DWORD LastTVChannelCheck = Now;
  DWORD LastRadioChannelCheck = Now-CHANNELCHECKDELTA*1000/2;

  while (!m_bStop)
  {
    Now = timeGetTime();

    /* Check for new or updated TV Channels */
    if (Now - LastTVChannelCheck > CHANNELCHECKDELTA*1000) // don't do this too often
    {
      CLog::Log(LOGDEBUG,"PVR: Updating TV Channel list");
      PVRChannelsTV.Update();
      LastTVChannelCheck = Now;
    }
    /* Check for new or updated Radio Channels */
    if (Now - LastRadioChannelCheck > CHANNELCHECKDELTA*1000) // don't do this too often
    {
      CLog::Log(LOGDEBUG,"PVR: Updating Radio Channel list");
      PVRChannelsRadio.Update();
      LastRadioChannelCheck = Now;
    }
    /* Check if we are 10 seconds before the end of the timeshift buffer
       and using timeshift and playback is paused, if yes start playback
       again */
    EnterCriticalSection(&m_critSection);
    if (m_timeshiftInt && g_application.IsPaused())
    {
      int time = (__int64)(g_application.GetTime()*1000) -
                 m_TimeshiftReceiver->GetTimeTotal() +
                 m_TimeshiftReceiver->GetDuration();

      if (time < 10000)
      {
        CLog::Log(LOGINFO,"PVR: Playback is resumed after reaching end of timeshift buffer");
        CAction action;
        action.id = ACTION_PAUSE;
        g_application.OnAction(action);
      }
    }
    LeaveCriticalSection(&m_critSection);

    Sleep(1000);
  }
}


/*************************************************************/
/** GUIInfoManager FUNCTIONS                                **/
/*************************************************************/

/********************************************************************
/* CPVRManager TranslateCharInfo
/*
/* Returns a GUIInfoManager Character String
/********************************************************************/
#define INFO_TOGGLE_TIME    1500
const char* CPVRManager::TranslateCharInfo(DWORD dwInfo)
{
  if      (dwInfo == PVR_NOW_RECORDING_TITLE)     return m_nowRecordingTitle;
  else if (dwInfo == PVR_NOW_RECORDING_CHANNEL)   return m_nowRecordingChannel;
  else if (dwInfo == PVR_NOW_RECORDING_DATETIME)  return m_nowRecordingDateTime;
  else if (dwInfo == PVR_NEXT_RECORDING_TITLE)    return m_nextRecordingTitle;
  else if (dwInfo == PVR_NEXT_RECORDING_CHANNEL)  return m_nextRecordingChannel;
  else if (dwInfo == PVR_NEXT_RECORDING_DATETIME) return m_nextRecordingDateTime;
  else if (dwInfo == PVR_BACKEND_NAME)            return m_backendName;
  else if (dwInfo == PVR_BACKEND_VERSION)         return m_backendVersion;
  else if (dwInfo == PVR_BACKEND_HOST)            return m_backendHost;
  else if (dwInfo == PVR_BACKEND_DISKSPACE)       return m_backendDiskspace;
  else if (dwInfo == PVR_BACKEND_CHANNELS)        return m_backendChannels;
  else if (dwInfo == PVR_BACKEND_TIMERS)          return m_backendTimers;
  else if (dwInfo == PVR_BACKEND_RECORDINGS)      return m_backendRecordings;
  else if (dwInfo == PVR_BACKEND_NUMBER)
  {
    if (m_infoToggleStart == 0)
    {
      m_infoToggleStart = timeGetTime();
      m_infoToggleCurrent = 0;
    }
    else
    {
      if (timeGetTime() - m_infoToggleStart > INFO_TOGGLE_TIME)
      {
        if (m_clients.size() > 0)
        {
          m_infoToggleCurrent++;
          if (m_infoToggleCurrent > m_clients.size()-1)
            m_infoToggleCurrent = 0;

          CLIENTMAPITR itr = m_clients.begin();
          for (unsigned int i = 0; i < m_infoToggleCurrent; i++)
            itr++;

          long long kBTotal = 0;
          long long kBUsed  = 0;
          if (m_clients[(*itr).first]->GetDriveSpace(&kBTotal, &kBUsed) == PVR_ERROR_NO_ERROR)
          {
            kBTotal /= 1024; // Convert to MBytes
            kBUsed /= 1024;  // Convert to MBytes
            m_backendDiskspace.Format("%s %0.f GByte - %s: %0.f GByte", g_localizeStrings.Get(18055), (float) kBTotal / 1024, g_localizeStrings.Get(156), (float) kBUsed / 1024);
          }
          else
          {
            m_backendDiskspace = g_localizeStrings.Get(18074);
          }

          int NumChannels = m_clients[(*itr).first]->GetNumChannels();
          if (NumChannels >= 0)
            m_backendChannels.Format("%i", NumChannels);
          else
            m_backendChannels = g_localizeStrings.Get(161);

          int NumTimers = m_clients[(*itr).first]->GetNumTimers();
          if (NumTimers >= 0)
            m_backendTimers.Format("%i", NumTimers);
          else
            m_backendTimers = g_localizeStrings.Get(161);

          int NumRecordings = m_clients[(*itr).first]->GetNumRecordings();
          if (NumRecordings >= 0)
            m_backendRecordings.Format("%i", NumRecordings);
          else
            m_backendRecordings = g_localizeStrings.Get(161);

          m_backendName         = m_clients[(*itr).first]->GetBackendName();
          m_backendVersion      = m_clients[(*itr).first]->GetBackendVersion();
          m_backendHost         = m_clients[(*itr).first]->GetConnectionString();
        }
        else
        {
          m_backendName         = "";
          m_backendVersion      = "";
          m_backendHost         = "";
          m_backendDiskspace    = "";
          m_backendTimers       = "";
          m_backendRecordings   = "";
          m_backendChannels     = "";
        }
        m_infoToggleStart = timeGetTime();
      }
    }

    static CStdString backendClients;
    if (m_clients.size() > 0)
      backendClients.Format("%u %s %u",m_infoToggleCurrent+1 ,g_localizeStrings.Get(20163), m_clients.size());
    else
      backendClients = g_localizeStrings.Get(14023);

    return backendClients;
  }
  else if (dwInfo == PVR_TOTAL_DISKSPACE)
  {
    long long kBTotal = 0;
    long long kBUsed  = 0;
    CLIENTMAPITR itr = m_clients.begin();
    while (itr != m_clients.end())
    {
      long long clientKBTotal = 0;
      long long clientKBUsed  = 0;

      if (m_clients[(*itr).first]->GetDriveSpace(&clientKBTotal, &clientKBUsed) == PVR_ERROR_NO_ERROR)
      {
        kBTotal += clientKBTotal;
        kBUsed += clientKBUsed;
      }
      itr++;
    }
    kBTotal /= 1024; // Convert to MBytes
    kBUsed /= 1024;  // Convert to MBytes
    m_totalDiskspace.Format("%s %0.f GByte - %s: %0.f GByte", g_localizeStrings.Get(18055), (float) kBTotal / 1024, g_localizeStrings.Get(156), (float) kBUsed / 1024);
    return m_totalDiskspace;
  }
  else if (dwInfo == PVR_NEXT_TIMER)
  {
    cPVRTimerInfoTag *next = PVRTimers.GetNextActiveTimer();
    if (next != NULL)
    {
      m_nextTimer.Format("%s %s %s %s", g_localizeStrings.Get(18190)
                         , next->Start().GetAsLocalizedDate(true)
                         , g_localizeStrings.Get(18191)
                         , next->Start().GetAsLocalizedTime("HH:mm", false));
      return m_nextTimer;
    }
  }
  else if (dwInfo == PVR_TIMESHIFT_DURATION)
  {
    if (m_TimeshiftReceiver)
      return m_TimeshiftReceiver->GetDurationString();
  }
  else if (dwInfo == PVR_TIMESHIFT_TIME)
  {
    if (m_TimeshiftReceiver)
    {
      int time = (__int64)(g_application.GetTime()*1000) -
                 m_TimeshiftReceiver->GetTimeTotal() +
                 m_TimeshiftReceiver->GetDuration();

      StringUtils::SecondsToTimeString(time/1000, m_timeshiftTime, TIME_FORMAT_GUESS);
      return m_timeshiftTime.c_str();
    }
  }
  else if (dwInfo == PVR_PLAYING_DURATION)
  {
    StringUtils::SecondsToTimeString(GetTotalTime()/1000, m_playingDuration, TIME_FORMAT_GUESS);
    return m_playingDuration.c_str();
  }
  else if (dwInfo == PVR_PLAYING_TIME)
  {
    StringUtils::SecondsToTimeString(GetStartTime()/1000, m_playingTime, TIME_FORMAT_GUESS);
    return m_playingTime.c_str();
  }
  return "";
}

/********************************************************************
/* CPVRManager TranslateIntInfo
/*
/* Returns a GUIInfoManager integer value
/********************************************************************/
int CPVRManager::TranslateIntInfo(DWORD dwInfo)
{
  if      (dwInfo == PVR_TIMESHIFT_PROGRESS)
  {
    if (!m_currentPlayingChannel || !m_timeshiftInt)
      return 0.0f;

    int time = (__int64)(g_application.GetTime()*1000) -
               m_TimeshiftReceiver->GetTimeTotal() +
               m_TimeshiftReceiver->GetDuration();

    return (float)(((float)time / m_TimeshiftReceiver->GetDuration()) * 100);
  }
  else if (dwInfo == PVR_PLAYING_PROGRESS)
  {
    return (float)(((float)GetStartTime() / GetTotalTime()) * 100);
  }
  return 0;
}

/********************************************************************
/* CPVRManager TranslateBoolInfo
/*
/* Returns a GUIInfoManager boolean value
/********************************************************************/
bool CPVRManager::TranslateBoolInfo(DWORD dwInfo)
{
  if (dwInfo == PVR_IS_RECORDING)
    return m_isRecording;
  else if (dwInfo == PVR_HAS_TIMER)
    return m_hasTimers;
  else if (dwInfo == PVR_IS_PLAYING_TV)
    return IsPlayingTV();
  else if (dwInfo == PVR_IS_PLAYING_RADIO)
    return IsPlayingRadio();
  else if (dwInfo == PVR_IS_PLAYING_RECORDING)
    return IsPlayingRecording();
  else if (dwInfo == PVR_IS_TIMESHIFTING)
    return m_timeshiftInt;

  return false;
}


/*************************************************************/
/** GENERAL FUNCTIONS                                       **/
/*************************************************************/

/********************************************************************
/* CPVRManager IsPlayingTV
/*
/* Returns true if a TV channel is playing
/********************************************************************/
bool CPVRManager::IsPlayingTV()
{
  if (!m_currentPlayingChannel)
    return false;

  return !m_currentPlayingChannel->GetTVChannelInfoTag()->IsRadio();
}

/********************************************************************
/* CPVRManager IsPlayingRadio
/*
/* Returns true if a radio channel is playing
/********************************************************************/
bool CPVRManager::IsPlayingRadio()
{
  if (!m_currentPlayingChannel)
    return false;

  return m_currentPlayingChannel->GetTVChannelInfoTag()->IsRadio();
}

/********************************************************************
/* CPVRManager IsPlayingRecording
/*
/* Returns true if a recording is playing
/********************************************************************/
bool CPVRManager::IsPlayingRecording()
{
  if (m_currentPlayingRecording)
    return false;
  else
    return true;
}

/********************************************************************
/* CPVRManager IsTimeshifting
/*
/* Returns true if timeshift is active
/********************************************************************/
bool CPVRManager::IsTimeshifting()
{
  if (m_timeshiftInt || m_timeshiftExt)
    return true;
  else
    return false;
}

/********************************************************************
/* CPVRManager GetCurrentClientProps
/*
/* Returns the properties of the current playing client or NULL if
/* if no stream is playing
/********************************************************************/
PVR_SERVERPROPS *CPVRManager::GetCurrentClientProps()
{
  if (m_currentPlayingChannel)
    return &m_clientsProps[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()];
  else if (m_currentPlayingRecording)
    return &m_clientsProps[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()];
  else
    return NULL;
}

/********************************************************************
/* CPVRManager GetCurrentPlayingItem
/*
/* Returns the current playing file item
/********************************************************************/
CFileItem *CPVRManager::GetCurrentPlayingItem()
{
  if (m_currentPlayingChannel)
    return m_currentPlayingChannel;
  else if (m_currentPlayingRecording)
    return m_currentPlayingRecording;
  else
    return NULL;
}

/********************************************************************
/* CPVRManager GetCurrentChannel
/*
/* Returns the current playing channel number
/********************************************************************/
bool CPVRManager::GetCurrentChannel(int *number, bool *radio)
{
  if (m_currentPlayingChannel)
  {
    *number = m_currentPlayingChannel->GetTVChannelInfoTag()->Number();
    *radio  = m_currentPlayingChannel->GetTVChannelInfoTag()->IsRadio();
    return true;
  }
  else
  {
    *number = 1;
    *radio  = false;
    return false;
  }
}

/********************************************************************
/* CPVRManager HaveActiveClients
/*
/* Returns true if a minimum one client is active
/********************************************************************/
bool CPVRManager::HaveActiveClients()
{
  if (m_clients.empty())
    return false;

  int ready = 0;
  CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    if (m_clients[(*itr).first]->ReadyToUse())
      ready++;
    itr++;
  }
  return ready > 0 ? true : false;
}

/********************************************************************
/* CPVRManager GetPreviousChannel
/*
/* Returns the previous selected channel or -1
/********************************************************************/
int CPVRManager::GetPreviousChannel()
{
  if (m_currentPlayingChannel == NULL)
    return -1;

  int LastChannel = m_currentPlayingChannel->GetTVChannelInfoTag()->Number();

  if (m_PreviousChannel[m_PreviousChannelIndex ^ 1] == LastChannel || LastChannel != m_PreviousChannel[0] && LastChannel != m_PreviousChannel[1])
    m_PreviousChannelIndex ^= 1;

  return m_PreviousChannel[m_PreviousChannelIndex ^= 1];
}

/********************************************************************
/* CPVRManager CanInstantRecording
/*
/* Returns true if we can start a instant recording on playing channel
/********************************************************************/
bool CPVRManager::CanInstantRecording()
{
  if (!m_currentPlayingChannel)
    return false;

  const cPVRChannelInfoTag* tag = m_currentPlayingChannel->GetTVChannelInfoTag();
  if (m_clientsProps[tag->ClientID()].SupportTimers)
    return true;
  else
    return false;
}

/********************************************************************
/* CPVRManager IsRecordingOnPlayingChannel
/*
/* Returns true if a instant recording on playing channel is running
/********************************************************************/
bool CPVRManager::IsRecordingOnPlayingChannel()
{
  if (!m_currentPlayingChannel)
    return false;

  const cPVRChannelInfoTag* tag = m_currentPlayingChannel->GetTVChannelInfoTag();
  return tag->IsRecording();
}

/********************************************************************
/* CPVRManager StartRecordingOnPlayingChannel
/*
/* Start a instant recording on playing channel
/********************************************************************/
bool CPVRManager::StartRecordingOnPlayingChannel(bool bOnOff)
{
  if (!m_currentPlayingChannel)
    return false;

  cPVRChannelInfoTag* tag = m_currentPlayingChannel->GetTVChannelInfoTag();
  if (m_clientsProps[tag->ClientID()].SupportTimers)
  {
    cPVRChannels *channels;
    if (!m_currentPlayingChannel->GetTVChannelInfoTag()->IsRadio())
      channels = &PVRChannelsTV;
    else
      channels = &PVRChannelsRadio;

    if (bOnOff && tag->IsRecording() == false)
    {
      cPVRTimerInfoTag newtimer(true);
      newtimer.SetTitle(tag->Name());
      CFileItem *item = new CFileItem(newtimer);

      if (!cPVRTimers::AddTimer(*item))
      {
        CGUIDialogOK::ShowAndGetInput(18100,0,18053,0);
        return false;
      }

      channels->at(tag->Number()-1).SetRecording(true); /* Set in channel list */
      tag->SetRecording(true);                          /* and also in current playing item */
      return true;
    }
    else if (tag->IsRecording() == true)
    {
      for (unsigned int i = 0; i < PVRTimers.size(); ++i)
      {
        if (!PVRTimers[i].IsRepeating() && PVRTimers[i].Active() &&
            (PVRTimers[i].Number() == tag->Number()) &&
            (PVRTimers[i].Start() <= CDateTime::GetCurrentDateTime()) &&
            (PVRTimers[i].Stop() >= CDateTime::GetCurrentDateTime()))
        {
          if (cPVRTimers::DeleteTimer(PVRTimers[i], true))
          {
            channels->at(tag->Number()-1).SetRecording(false);  /* Set in channel list */
            tag->SetRecording(false);                           /* and also in current playing item */
            return true;
          }
        }
      }
    }
  }
  return false;
}

/********************************************************************
/* CPVRManager SetCurrentPlayingProgram
/*
/* Write information of current playing program to the given FileItem.
/********************************************************************/
void CPVRManager::SetCurrentPlayingProgram(CFileItem& item)
{
  /* Check if a cPVRChannelInfoTag is inside file item */
  if (!item.IsTVChannel())
  {
    CLog::Log(LOGERROR, "CPVRManager: SetCurrentPlayingProgram no TVChannelTag given!");
    return;
  }

  cPVRChannelInfoTag* tag = item.GetTVChannelInfoTag();
  if (tag != NULL)
  {
    if (tag->Number() != m_PreviousChannel[m_PreviousChannelIndex])
      m_PreviousChannel[m_PreviousChannelIndex ^= 1] = tag->Number();

    const CTVEPGInfoTag *epgnow = NULL;
    const CTVEPGInfoTag *epgnext = NULL;
    cPVREpgsLock EpgsLock;
    cPVREpgs *s = (cPVREpgs *)cPVREpgs::EPGs(EpgsLock);
    if (s)
    {
      epgnow = s->GetEPG(tag, true)->GetInfoTagNow();
      epgnext = s->GetEPG(tag, true)->GetInfoTagNext();
    }

    if (epgnow)
    {
      tag->m_strTitle          = epgnow->Title();
      tag->m_strOriginalTitle  = epgnow->Title();
      tag->m_strPlotOutline    = epgnow->PlotOutline();
      tag->m_strPlot           = epgnow->Plot();
      tag->m_strGenre          = epgnow->Genre();
      tag->SetStartTime(epgnow->Start());
      tag->SetEndTime(epgnow->End());
      tag->SetDuration(epgnow->Duration());
      if (epgnext)
        tag->SetNextTitle(epgnext->Title());
      else
        tag->SetNextTitle("");

      if (tag->m_strPlot.Left(tag->m_strPlotOutline.length()) != tag->m_strPlotOutline && !tag->m_strPlotOutline.IsEmpty())
        tag->m_strPlot = tag->m_strPlotOutline + '\n' + tag->m_strPlot;

      CDateTimeSpan span = tag->StartTime() - tag->EndTime();

      StringUtils::SecondsToTimeString(span.GetSeconds()
                                       + span.GetMinutes() * 60.
                                       + span.GetHours() * 3600, tag->m_strRuntime, TIME_FORMAT_GUESS);
    }
    else
    {
      tag->m_strTitle          = g_localizeStrings.Get(18074);
      tag->m_strOriginalTitle  = g_localizeStrings.Get(18074);
      tag->m_strPlotOutline    = "";
      tag->m_strPlot           = "";
      tag->m_strGenre          = "";
      tag->SetStartTime(CDateTime::GetCurrentDateTime()+CDateTimeSpan(0, 0, 0, 0)-CDateTimeSpan(0, 1, 0, 0));
      tag->SetEndTime(CDateTime::GetCurrentDateTime()+CDateTimeSpan(0, 23, 0, 0));
      tag->SetDuration(CDateTimeSpan(0, 1, 0, 0));
      tag->SetNextTitle("");
    }

    if (tag->IsRadio())
    {
      CMusicInfoTag* musictag = item.GetMusicInfoTag();

      musictag->SetURL(tag->Path());
      musictag->SetTitle(tag->m_strTitle);
      musictag->SetArtist(tag->Name());
    //    musictag->SetAlbum(tag->m_strBouquet);
      musictag->SetAlbumArtist(tag->Name());
      musictag->SetGenre(tag->m_strGenre);
      musictag->SetDuration(tag->GetDuration());
      musictag->SetLoaded(true);
      musictag->SetComment("");
      musictag->SetLyrics("");
    }

    tag->m_strAlbum = tag->Name();
    tag->m_iSeason  = 0; /* set this so xbmc knows it's a tv show */
    tag->m_iEpisode = 0;
    tag->m_strShowTitle.Format("%i", tag->Number());

    item.m_strTitle = tag->Name();
    item.m_dateTime = tag->StartTime();
    item.m_strPath  = tag->Path();
  }
}


/*************************************************************/
/** PVR CLIENT INPUT STREAM                                 **/
/**                                                         **/
/** PVR Client internal input stream access, is used if     **/
/** inside PVR_SERVERPROPS the HandleInputStream is true    **/
/*************************************************************/

/********************************************************************
/* CPVRManager OpenLiveStream
/*
/* Open a Channel by the channel number. Number can be a TV or Radio
/* channel and is defined by the "bool radio" flag.
/*
/* Returns true if opening was succesfull
/********************************************************************/
bool CPVRManager::OpenLiveStream(unsigned int channel, bool radio)
{
  EnterCriticalSection(&m_critSection);

  /* Check if a channel or recording is already opened and clear it if yes */
  if (m_currentPlayingChannel)
    delete m_currentPlayingChannel;
  if (m_currentPlayingRecording)
    delete m_currentPlayingRecording;

  /* Set the new channel information */
  m_currentPlayingChannel   = new CFileItem(radio ? PVRChannelsRadio[channel-1] : PVRChannelsTV[channel-1]);
  m_currentPlayingRecording = NULL;
  m_scanStart               = timeGetTime();  /* Reset the stream scan timer */

  /* Open the stream on the Client */
  const cPVRChannelInfoTag* tag = m_currentPlayingChannel->GetTVChannelInfoTag();
  if (!m_clientsProps[tag->ClientID()].HandleInputStream ||
      !m_clients[tag->ClientID()]->OpenLiveStream(*tag))
  {
    delete m_currentPlayingChannel;
    m_currentPlayingChannel = NULL;
    LeaveCriticalSection(&m_critSection);
    return false;
  }

  /* Set information like the EPG for this channel to have program information
     in Fullscreen info */
  SetCurrentPlayingProgram(*m_currentPlayingChannel);

  /* Clear the timeshift flags */
  m_timeshiftExt            = false;
  m_timeshiftInt            = false;
  m_timeshiftLastWrapAround = 0;
  m_timeshiftCurrWrapAround = 0;
  m_playbackStarted         = tag->GetTime()*1000;

  /* Timeshift related code */
  /* Check if Client handles timeshift itself */
  if (m_clientsProps[tag->ClientID()].SupportTimeShift)
  {
    m_timeshiftExt = true;
  }
  /* Check if XBMC is allowed to do the timeshift */
  else if (g_guiSettings.GetBool("pvrplayback.timeshift") && g_guiSettings.GetString("pvrplayback.timeshiftpath") != "")
  {
    /* Create the receiving class to perform independent stream reading and
       Open the stream buffer file*/
    if (CreateInternalTimeshift() && m_TimeshiftReceiver->StartReceiver(m_clients[tag->ClientID()]))
      m_timeshiftInt = true;
  }

  LeaveCriticalSection(&m_critSection);
  return true;
}

/********************************************************************
/* CPVRManager OpenRecordedStream
/*
/* Open a recording by a index number passed to this function.
/*
/* Returns true if opening was succesfull
/********************************************************************/
bool CPVRManager::OpenRecordedStream(unsigned int recording)
{
  EnterCriticalSection(&m_critSection);

  bool ret = false;

  /* Check if a channel or recording is already opened and clear it if yes */
  if (m_currentPlayingChannel)
    delete m_currentPlayingChannel;
  if (m_currentPlayingRecording)
    delete m_currentPlayingRecording;

  /* Set the new recording information */
  m_currentPlayingRecording = new CFileItem(PVRRecordings[recording-1]);
  m_currentPlayingChannel   = NULL;
  m_scanStart               = timeGetTime();  /* Reset the stream scan timer */

  /* Open the recording stream on the Client */
  const cPVRRecordingInfoTag* tag = m_currentPlayingRecording->GetTVRecordingInfoTag();
  if (m_clientsProps[tag->ClientID()].HandleInputStream)
    ret = m_clients[tag->ClientID()]->OpenRecordedStream(*tag);

  LeaveCriticalSection(&m_critSection);
  return ret;
}

/********************************************************************
/* CPVRManager CloseStream
/*
/* Close the stream on the PVR Client and if internal timeshift is
/* used delete the Timeshift receiver.
/********************************************************************/
void CPVRManager::CloseStream()
{
  EnterCriticalSection(&m_critSection);

  if (m_currentPlayingChannel)
  {
    /* Stop the Timeshift if internal receiving is active */
    if (m_timeshiftInt)
    {
      m_pTimeshiftFile->Close();
      delete m_pTimeshiftFile;
      delete m_TimeshiftReceiver;
      m_TimeshiftReceiver = NULL;
      m_pTimeshiftFile    = NULL;
    }
    m_timeshiftInt    = false;
    m_timeshiftExt    = false;
    m_playbackStarted = -1;

    /* Close the Client connection */
    m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()]->CloseLiveStream();
    delete m_currentPlayingChannel;
    m_currentPlayingChannel = NULL;
  }
  else if (m_currentPlayingRecording)
  {
    /* Close the Client connection */
    m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->CloseRecordedStream();
    delete m_currentPlayingRecording;
    m_currentPlayingRecording = NULL;
  }

  LeaveCriticalSection(&m_critSection);
  return;
}

/********************************************************************
/* CPVRManager ReadStream
/*
/* Read the stream to the buffer pointer passed defined by "buf" and
/* a maximum site passed with "buf_size".
/*
/* The amount of readed bytes is returned.
/********************************************************************/
int CPVRManager::ReadStream(BYTE* buf, int buf_size)
{
  EnterCriticalSection(&m_critSection);

  int bytesReaded = 0;

  /* Check stream for available video or audio data, if after the scantime no stream
     is present playback is canceled and returns to the window */
  if (m_scanStart)
  {
    if (timeGetTime() - m_scanStart > (unsigned int) g_guiSettings.GetInt("pvrplayback.scantime")*1000)
    {
      CLog::Log(LOGERROR,"PVR: No video or audio data available after %i seconds, playback stopped", g_guiSettings.GetInt("pvrplayback.scantime"));
      LeaveCriticalSection(&m_critSection);
      return 0;
    }
    else if (g_application.IsPlayingVideo() || g_application.IsPlayingAudio())
      m_scanStart = NULL;
  }

  /* Process LiveTV Reading */
  if (m_currentPlayingChannel)
  {
    /* If internal Timeshift is used read from the Buffer file */
    if (m_timeshiftInt)
    {
      if (!m_pTimeshiftFile)
      {
        CLog::Log(LOGERROR,"PVR: internal Timeshift stream reading called without timeshift file");
        LeaveCriticalSection(&m_critSection);
        return -1;
      }

      DWORD now = timeGetTime();

      /* Never read behind current write position inside cache */
      while (m_timeshiftCurrWrapAround+m_pTimeshiftFile->GetPosition()+buf_size+131072 > m_TimeshiftReceiver->GetWritten())
      {
        if (timeGetTime() - now > 5*1000)
        {
          CLog::Log(LOGERROR,"PVR: internal Timeshift Cache Position timeout");
          LeaveCriticalSection(&m_critSection);
          return 0;
        }
        Sleep(5);
      }

      /* Reduce buffer size to prevent partly read behind range */
      int tmp = m_TimeshiftReceiver->GetMaxSize() - m_pTimeshiftFile->GetPosition();
      if (tmp > 0 && tmp < buf_size)
        buf_size = tmp;

REPEAT_READ:
      bytesReaded = m_pTimeshiftFile->Read(buf, buf_size);
      if (bytesReaded <= 0)
      {
        if (timeGetTime() - now > 5*1000)
        {
          CLog::Log(LOGERROR,"PVR: internal Timeshift Cache Read timeout");
          LeaveCriticalSection(&m_critSection);
          return 0;
        }

        Sleep(5);
        goto REPEAT_READ;
      }

      /* Check if we are at end of buffer file, if yes wrap around and
         start at beginning */
      if (m_pTimeshiftFile->GetPosition() >= m_TimeshiftReceiver->GetMaxSize())
      {
        CLog::Log(LOGDEBUG,"PVR: internal Timeshift Cache wrap around");
        m_timeshiftLastWrapAround  = m_timeshiftCurrWrapAround;
        m_timeshiftCurrWrapAround += m_TimeshiftReceiver->GetMaxSize();
        m_pTimeshiftFile->Seek(0);
      }

    }
    /* Read stream directly if timeshift is handled by client or not supported */
    else
    {
      bytesReaded = m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()]->ReadLiveStream(buf, buf_size);
    }
  }
  /* Process Recording Reading */
  else if (m_currentPlayingRecording)
  {
    bytesReaded = m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->ReadRecordedStream(buf, buf_size);
  }

  LeaveCriticalSection(&m_critSection);
  return bytesReaded;
}

/********************************************************************
/* CPVRManager LengthStream
/*
/* Return the length in bytes of the current stream readed from
/* PVR Client or NULL if internal timeshift is used.
/********************************************************************/
__int64 CPVRManager::LengthStream(void)
{
  __int64 streamLength = 0;

  EnterCriticalSection(&m_critSection);

  if (m_currentPlayingChannel)
  {
    if (m_timeshiftInt)
    {
      // TODO: handle for internal timeshift
      streamLength = 0;
    }
    else if (m_timeshiftExt)
    {
      // TODO: check with external timeshift
      streamLength = m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()]->LengthLiveStream();
    }
  }
  else if (m_currentPlayingRecording)
  {
    streamLength = m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->LengthRecordedStream();
  }

  LeaveCriticalSection(&m_critSection);
  return streamLength;
}

/********************************************************************
/* CPVRManager SeekTimeRequired
/*
/* Return true if internal timeshift is used.
/********************************************************************/
bool CPVRManager::SeekTimeRequired()
{
  if (m_currentPlayingChannel && m_timeshiftInt)
    return true;

  return false;
}

/********************************************************************
/* CPVRManager SeekTimeStep
/*
/* Return the seek time step in milliseconds
/********************************************************************/
int CPVRManager::SeekTimeStep(bool bPlus, bool bLargeStep, __int64 curTime)
{
  if (!m_currentPlayingChannel || !m_timeshiftInt)
    return 0;

  int timeMax = m_TimeshiftReceiver->GetTimeTotal() - 3000;
  int timeMin = m_TimeshiftReceiver->GetTimeTotal() - m_TimeshiftReceiver->GetDuration() + 3000;
  int step = m_TimeshiftReceiver->GetDuration()/100 * (bLargeStep ? 10 : 1);

  if (bPlus)
  {
    if (curTime > timeMax)
      return 0;

    if (curTime + step > timeMax)
      step = curTime + step - timeMax;

    if (step < 0)
      step = 0;

    return step;
  }
  else
  {
    if (curTime <= timeMin)
      return 0;

    if (curTime - step < timeMin)
      step = curTime - timeMin;

    if (step < 0)
      step = 0;

    return step;
  }
}

/********************************************************************
/* CPVRManager SeekTime
/*
/* Perform a seek by time passed as time position from the begin of
/* the playback in milliseconds.
/* It is only used when internal timeshift is used.
/*
/* Returns true if switch was succesfull
/********************************************************************/
bool CPVRManager::SeekTime(int iTimeInMsec, int *iRetTimeInMsec)
{
  DWORD newTimePos;
  bool  wrapback  = false;
  bool  ret       = false;

  if (!m_currentPlayingChannel || !m_timeshiftInt)
    return false;

   EnterCriticalSection(&m_critSection);

  __int64 streamNewPos = m_TimeshiftReceiver->TimeToPos(iTimeInMsec, &newTimePos, &wrapback);
  if (streamNewPos > 0 && newTimePos > 0)
  {
    /* Check if the position is before the wrap around position,
       if yes subract the file length that was previuously added
       by ReadStream. */
    if (wrapback && m_timeshiftCurrWrapAround > 0)
      m_timeshiftCurrWrapAround = m_timeshiftLastWrapAround;

    /* Seek to new file position for the next read */
    if (m_pTimeshiftFile->Seek(streamNewPos) >= 0)
    {
      *iRetTimeInMsec = newTimePos;
      ret = true;
    }
  }

  LeaveCriticalSection(&m_critSection);
  return ret;
}

/********************************************************************
/* CPVRManager SeekStream
/*
/* Perform a file seek to the PVR Client if a recording is played or
/* if a channel is played and the client support timeshift.
/* Seeking is not performed for if own internal timeshift is used.
/*
/* Returns the new position after seek or < 0 if seek was failed
/********************************************************************/
__int64 CPVRManager::SeekStream(__int64 pos, int whence)
{
  __int64 streamNewPos = 0;

  EnterCriticalSection(&m_critSection);

  if (m_currentPlayingChannel)
  {
    if (m_timeshiftInt)
    {
      // TODO: handle for internal timeshift
      streamNewPos = 0;
    }
    else if (m_timeshiftExt)
    {
      // TODO: check with external timeshift
      streamNewPos = m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()]->SeekLiveStream(pos, whence);
    }
  }
  else if (m_currentPlayingRecording)
  {
    streamNewPos = m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->SeekRecordedStream(pos, whence);
  }

  LeaveCriticalSection(&m_critSection);
  return streamNewPos;
}

/********************************************************************
/* CPVRManager UpdateItem
/*
/* Update the current running file: It is called during a channel
/* change to refresh the global file item.
/********************************************************************/
bool CPVRManager::UpdateItem(CFileItem& item)
{
  /* Don't update if a recording is played */
  if (item.IsTVRecording())
    return true;

  if (!item.IsTVChannel())
  {
    CLog::Log(LOGERROR, "CPVRManager: UpdateItem no TVChannelTag given!");
    return false;
  }

  cPVRChannelInfoTag* tag = item.GetTVChannelInfoTag();
  cPVRChannelInfoTag* current = m_currentPlayingChannel->GetTVChannelInfoTag();

  tag->m_strAlbum         = current->Name();
  tag->m_strTitle         = current->m_strTitle;
  tag->m_strOriginalTitle = current->m_strTitle;
  tag->m_strPlotOutline   = current->m_strPlotOutline;
  tag->m_strPlot          = current->m_strPlot;
  tag->m_strGenre         = current->m_strGenre;
  tag->m_strPath          = current->Path();
  tag->m_strShowTitle.Format("%i", current->Number());
  tag->SetNextTitle(current->NextTitle());
  tag->SetPath(current->Path());

  item.m_strTitle = current->Name();
  item.m_dateTime = current->StartTime();
  item.m_strPath  = current->Path();

  CDateTimeSpan span = current->StartTime() - current->EndTime();
  StringUtils::SecondsToTimeString(span.GetSeconds() + span.GetMinutes() * 60 + span.GetHours() * 3600,
                                   tag->m_strRuntime,
                                   TIME_FORMAT_GUESS);

  if (current->Icon() != "")
  {
    item.SetThumbnailImage(current->Icon());
  }
  else
  {
    item.SetThumbnailImage("");
    item.FillInDefaultIcon();
  }

  g_infoManager.SetCurrentItem(item);
  g_application.CurrentFileItem().m_strPath = item.m_strPath;
  return true;
}

/********************************************************************
/* CPVRManager ChannelSwitch
/*
/* It switch to the channel passed to this function
/*
/* Returns true if switch was succesfull
/********************************************************************/
bool CPVRManager::ChannelSwitch(unsigned int iChannel)
{
  if (!m_currentPlayingChannel)
    return false;

  cPVRChannels *channels;
  if (!m_currentPlayingChannel->GetTVChannelInfoTag()->IsRadio())
    channels = &PVRChannelsTV;
  else
    channels = &PVRChannelsRadio;

  if (iChannel > channels->size()+1)
  {
    CGUIDialogOK::ShowAndGetInput(18100,18105,0,0);
    return false;
  }

  EnterCriticalSection(&m_critSection);

  /* If Timeshift is active reset all relevant data and stop the receiver */
  if (m_timeshiftInt)
  {
    m_TimeshiftReceiver->StopReceiver();
    m_pTimeshiftFile->Seek(0);
  }

  /* Clear the timeshift flags */
  m_timeshiftExt                = false;
  m_timeshiftInt                = false;
  m_timeshiftLastWrapAround     = 0;
  m_timeshiftCurrWrapAround     = 0;
  const cPVRChannelInfoTag* tag = &channels->at(iChannel-1);

  /* Perform Channelswitch */
  if (!m_clients[tag->ClientID()]->SwitchChannel(*tag))
  {
    CGUIDialogOK::ShowAndGetInput(18100,0,18134,0);
    LeaveCriticalSection(&m_critSection);
    return false;
  }

  /* Timeshift related code */
  /* Check if Client handles timeshift itself */
  if (m_clientsProps[tag->ClientID()].SupportTimeShift)
  {
    m_timeshiftExt = true;
  }
  /* Start the Timeshift receiver again if active */
  else if (g_guiSettings.GetBool("pvrplayback.timeshift") && g_guiSettings.GetString("pvrplayback.timeshiftpath") != "")
  {
    if (!m_TimeshiftReceiver)
      CreateInternalTimeshift();

    /* Start the Timeshift receiver */
    if (m_TimeshiftReceiver && m_TimeshiftReceiver->StartReceiver(m_clients[tag->ClientID()]))
      m_timeshiftInt = true;
  }

  /* Update the Playing channel data and the current epg data */
  delete m_currentPlayingChannel;
  m_currentPlayingChannel = new CFileItem(*tag);
  m_scanStart             = timeGetTime();
  SetCurrentPlayingProgram(*m_currentPlayingChannel);
  m_playbackStarted       = m_currentPlayingChannel->GetTVChannelInfoTag()->GetTime()*1000 - (__int64)(g_application.GetTime()*1000);

  LeaveCriticalSection(&m_critSection);
  return true;
}

/********************************************************************
/* CPVRManager ChannelUp
/*
/* It switch to the next channel and return the new channel to
/* the pointer passed by this function
/*
/* Returns true if switch was succesfull
/********************************************************************/
bool CPVRManager::ChannelUp(unsigned int *newchannel)
{
   if (m_currentPlayingChannel)
   {
    cPVRChannels *channels;
    if (!m_currentPlayingChannel->GetTVChannelInfoTag()->IsRadio())
      channels = &PVRChannelsTV;
    else
      channels = &PVRChannelsRadio;

    EnterCriticalSection(&m_critSection);

    /* If Timeshift is active reset all relevant data and stop the receiver */
    if (m_timeshiftInt)
    {
     m_TimeshiftReceiver->StopReceiver();
     m_pTimeshiftFile->Seek(0);
    }

    /* Clear the timeshift flags */
    m_timeshiftExt            = false;
    m_timeshiftInt            = false;
    m_timeshiftLastWrapAround = 0;
    m_timeshiftCurrWrapAround = 0;

    unsigned int currentTVChannel = m_currentPlayingChannel->GetTVChannelInfoTag()->Number();
    const cPVRChannelInfoTag* tag;
    for (unsigned int i = 1; i < channels->size(); i++)
    {
      currentTVChannel += 1;

      if (currentTVChannel > channels->size())
        currentTVChannel = 1;

      tag = &channels->at(currentTVChannel-1);

      if ((m_CurrentGroupID != -1) && (m_CurrentGroupID != tag->GroupID()))
        continue;

      /* Perform Channelswitch */
      if (m_clients[tag->ClientID()]->SwitchChannel(*tag))
      {
        /* Update the Playing channel data and the current epg data */
        delete m_currentPlayingChannel;
        m_currentPlayingChannel = new CFileItem(*tag);
        m_scanStart             = timeGetTime();
        SetCurrentPlayingProgram(*m_currentPlayingChannel);
        m_playbackStarted       = m_currentPlayingChannel->GetTVChannelInfoTag()->GetTime()*1000 - (__int64)(g_application.GetTime()*1000);

        /* Timeshift related code */
        /* Check if Client handles timeshift itself */
        if (m_clientsProps[tag->ClientID()].SupportTimeShift)
        {
          m_timeshiftExt = true;
        }
        /* Start the Timeshift receiver again if active */
        else if (g_guiSettings.GetBool("pvrplayback.timeshift") && g_guiSettings.GetString("pvrplayback.timeshiftpath") != "")
        {
          if (!m_TimeshiftReceiver)
            CreateInternalTimeshift();

          /* Start the Timeshift receiver */
          if (m_TimeshiftReceiver && m_TimeshiftReceiver->StartReceiver(m_clients[tag->ClientID()]))
            m_timeshiftInt = true;
        }

        *newchannel = currentTVChannel;
        LeaveCriticalSection(&m_critSection);
        return true;
      }
    }
    LeaveCriticalSection(&m_critSection);
  }

  return false;
}

/********************************************************************
/* CPVRManager ChannelDown
/*
/* It switch to the previous channel and return the new channel to
/* the pointer passed by this function
/*
/* Returns true if switch was succesfull
/********************************************************************/
bool CPVRManager::ChannelDown(unsigned int *newchannel)
{
  if (m_currentPlayingChannel)
  {
    cPVRChannels *channels;
    if (!m_currentPlayingChannel->GetTVChannelInfoTag()->IsRadio())
      channels = &PVRChannelsTV;
    else
      channels = &PVRChannelsRadio;

    EnterCriticalSection(&m_critSection);

    /* If Timeshift is active reset all relevant data and stop the receiver */
    if (m_timeshiftInt)
    {
      m_TimeshiftReceiver->StopReceiver();
      m_pTimeshiftFile->Seek(0);
    }

    /* Clear the timeshift flags */
    m_timeshiftExt            = false;
    m_timeshiftInt            = false;
    m_timeshiftLastWrapAround = 0;
    m_timeshiftCurrWrapAround = 0;

    int currentTVChannel = m_currentPlayingChannel->GetTVChannelInfoTag()->Number();
    const cPVRChannelInfoTag* tag;
    for (unsigned int i = 1; i < channels->size(); i++)
    {
      currentTVChannel -= 1;

      if (currentTVChannel <= 0)
        currentTVChannel = channels->size();

      tag = &channels->at(currentTVChannel-1);

      if ((m_CurrentGroupID != -1) && (m_CurrentGroupID != tag->GroupID()))
        continue;

      /* Perform Channelswitch */
      if (m_clients[tag->ClientID()]->SwitchChannel(*tag))
      {
        /* Update the Playing channel data and the current epg data */
        delete m_currentPlayingChannel;
        m_currentPlayingChannel = new CFileItem(*tag);
        m_scanStart             = timeGetTime();
        SetCurrentPlayingProgram(*m_currentPlayingChannel);
        m_playbackStarted       = m_currentPlayingChannel->GetTVChannelInfoTag()->GetTime()*1000 - (__int64)(g_application.GetTime()*1000);

        /* Timeshift related code */
        /* Check if Client handles timeshift itself */
        if (m_clientsProps[tag->ClientID()].SupportTimeShift)
        {
          m_timeshiftExt = true;
        }
        /* Start the Timeshift receiver again if active */
        else if (g_guiSettings.GetBool("pvrplayback.timeshift") && g_guiSettings.GetString("pvrplayback.timeshiftpath") != "")
        {
          if (!m_TimeshiftReceiver)
            CreateInternalTimeshift();

          /* Start the Timeshift receiver */
          if (m_TimeshiftReceiver && m_TimeshiftReceiver->StartReceiver(m_clients[tag->ClientID()]))
            m_timeshiftInt = true;
        }

        *newchannel = currentTVChannel;
        LeaveCriticalSection(&m_critSection);
        return true;
      }
    }
    LeaveCriticalSection(&m_critSection);
  }
  return false;
}

/********************************************************************
/* CPVRManager GetTotalTime
/*
/* Returns the duration of the current program.
/********************************************************************/
int CPVRManager::GetTotalTime()
{
  int duration = 1;

  if (m_currentPlayingChannel)
    duration = m_currentPlayingChannel->GetTVChannelInfoTag()->GetDuration() * 1000;

   /* Use 1 instead of 0 to prevent divide by NULL floating point exception */
  if (duration <= 0)
    duration = 1;

  return duration;
}

/********************************************************************
/* CPVRManager GetStartTime
/*
/* Returns the start time of the current playing program.
/********************************************************************/
int CPVRManager::GetStartTime()
{
  if (!m_currentPlayingChannel)
    return 0;

  cPVRChannelInfoTag* tag = m_currentPlayingChannel->GetTVChannelInfoTag();
  if (tag->EndTime() < CDateTime::GetCurrentDateTime())
  {
    EnterCriticalSection(&m_critSection);

    int oldchannel  = tag->Number();
    int oldduration = tag->GetDuration();

    /* Set the new program information */
    SetCurrentPlayingProgram(*m_currentPlayingChannel);

    /* Correct the playback started time */
    if (m_timeshiftInt && oldchannel == tag->Number())
      m_playbackStarted -= oldduration * 1000;


    if (UpdateItem(*m_currentPlayingChannel))
    {
      g_application.CurrentFileItem() = *m_currentPlayingChannel;
      g_infoManager.SetCurrentItem(*m_currentPlayingChannel);
    }

    LeaveCriticalSection(&m_critSection);
  }

  if (m_timeshiftInt)
  {
    return m_playbackStarted + (__int64)(g_application.GetTime()*1000);
  }
  else
  {
    CDateTimeSpan time = CDateTime::GetCurrentDateTime() - tag->StartTime();
    return time.GetDays()    * 1000 * 60 * 60 * 24
         + time.GetHours()   * 1000 * 60 * 60
         + time.GetMinutes() * 1000 * 60
         + time.GetSeconds() * 1000;
  }
}


/*************************************************************/
/** PVR CLIENT DEMUXER                                      **/
/**                                                         **/
/** PVR Client internal demuxer access, is used if inside   **/
/** PVR_SERVERPROPS the HandleDemuxing is true              **/
/*************************************************************/

bool CPVRManager::OpenDemux(PVRDEMUXHANDLE handle)
{
  EnterCriticalSection(&m_critSection);

  bool ret = false;

  if (m_currentPlayingChannel)
  {
    if (GetCurrentClientProps()->HandleDemuxing == true)
      ret = m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()]->OpenTVDemux(handle, *m_currentPlayingChannel->GetTVChannelInfoTag());
  }
  else if (m_currentPlayingRecording)
  {
    if (GetCurrentClientProps()->HandleDemuxing == true)
      ret = m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->OpenRecordingDemux(handle, *m_currentPlayingRecording->GetTVRecordingInfoTag());
  }

  LeaveCriticalSection(&m_critSection);
  return ret;
}

void CPVRManager::DisposeDemux()
{
  if (m_currentPlayingChannel)
    m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()]->DisposeDemux();
  else if (m_currentPlayingRecording)
    m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->DisposeDemux();
}

void CPVRManager::ResetDemux()
{
  if (m_currentPlayingChannel)
    m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()]->ResetDemux();
  else if (m_currentPlayingRecording)
    m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->ResetDemux();
}

void CPVRManager::FlushDemux()
{
  if (m_currentPlayingChannel)
    m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()]->FlushDemux();
  else if (m_currentPlayingRecording)
    m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->FlushDemux();
}

void CPVRManager::AbortDemux()
{
  if (m_currentPlayingChannel)
    m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()]->AbortDemux();
  else if (m_currentPlayingRecording)
    m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->AbortDemux();
}

void CPVRManager::SetDemuxSpeed(int iSpeed)
{
  if (m_currentPlayingChannel)
    m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()]->SetDemuxSpeed(iSpeed);
  else if (m_currentPlayingRecording)
    m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->SetDemuxSpeed(iSpeed);
}

demux_packet_t* CPVRManager::ReadDemux()
{
  demux_packet_t *ret = NULL;

  if (m_currentPlayingChannel)
    ret = m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()]->ReadDemux();
  else if (m_currentPlayingRecording)
    ret = m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->ReadDemux();

  return ret;
}

bool CPVRManager::SeekDemuxTime(int time, bool backwords, double* startpts)
{
  bool ret = false;

  if (m_currentPlayingChannel)
    ret = m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()]->SeekDemuxTime(time, backwords, startpts);
  else if (m_currentPlayingRecording)
    ret = m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->SeekDemuxTime(time, backwords, startpts);

  return ret;
}

int CPVRManager::GetDemuxStreamLength()
{
  int ret = 0;

  if (m_currentPlayingChannel)
    ret = m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()]->GetDemuxStreamLength();
  else if (m_currentPlayingRecording)
    ret = m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->GetDemuxStreamLength();

  return ret;
}

CPVRManager g_PVRManager;
