/*
*      Copyright (C) 2005-2008 Team XBMC
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

#include "stdafx.h"
#include "PVRClient-mythtv.h"

XFILE::CCMythSession*   PVRClientMythTv::m_mythEventSession;
myth_event_queue        PVRClientMythTv::m_thingsToDo;
CCriticalSection        PVRClientMythTv::m_thingsToDoSection;

PVRClientMythTv::PVRClientMythTv(DWORD clientID, IPVRClientCallback *callback)
  : IPVRClient(clientID, callback)
{
  m_recorder = NULL;
  m_session = NULL;
  m_isRunning = false;
  m_clientID = clientID;
  m_manager = callback;
}

PVRClientMythTv::~PVRClientMythTv()
{
  CLog::Log(LOGERROR, "PVR: client_%u destroyed", m_clientID);
  /*Release();*/
  StopThread();
}

void PVRClientMythTv::Release()
{
  if(m_session)
  {
    XFILE::CCMythSession::ReleaseSession(m_session);
    m_session = NULL;
  }
  m_dll = NULL;
}

void PVRClientMythTv::OnEvent(int event, const std::string& data)
{
  CSingleLock lock(m_thingsToDoSection);

  m_thingsToDo.push(std::make_pair<int, std::string>((int)event, data));
  lock.Leave();

  // start the thread to handle the event(s)
  if (!m_isRunning)
  {
    StopThread();
    Create(false, THREAD_MINSTACKSIZE);
  }
}

void PVRClientMythTv::OnStartup()
{
  int breakpoint = 0;
}

void PVRClientMythTv::OnExit()
{
  int breakpoint = 0;
}

bool PVRClientMythTv::GetEPGDataEnd(CDateTime &end)
{
  if (!GetLibrary() || !GetControl() || !GetDB())
    return false;

  /// need to use database query to find end of epg data

  return true;
}

void PVRClientMythTv::Connect()
{
  m_mythEventSession = XFILE::CCMythSession::AquireSession(m_connString);
  m_mythEventSession->SetListener(this);
}

PVRCLIENT_PROPS PVRClientMythTv::GetProperties()
{
  PVRCLIENT_PROPS props;

  props.Name            = "MythTV";
  props.HasBouquets     = false;
  props.DefaultHostname = "myth";
  props.DefaultPort     = 6543;
  props.DefaultUser     = "mythtv";
  props.DefaultPassword = "mythtv";

  return props;
}

bool PVRClientMythTv::IsUp()
{
  if (!GetControl() || !GetDB() || !GetLibrary())
    return false;

  // need to check user credentials are valid
  cmyth_chanlist_t results = m_dll->mysql_get_chanlist(m_database);
  int num = m_dll->chanlist_get_count(results);
  return (num > 0);
}

bool PVRClientMythTv::GetDriveSpace(long long *total, long long *used)
{
  if (!GetLibrary() || !GetControl())
    return false;

  cmyth_conn_t m_control = m_session->GetControl();
  if(!m_control)
  {
    CLog::Log(LOGERROR, "%s - unable to GetControl", __FUNCTION__);
    return false;
  }

  if(m_dll->conn_get_freespace(m_control, total, used) != 0)
  {
    return false;
  }

  return true;
}

bool PVRClientMythTv::GetRecordingSchedules( CFileItemList* results )
{
  if (!GetLibrary() || !GetControl())
    return false;
  
  cmyth_conn_t m_control = m_session->GetControl();
  if(!m_control)
  {
    CLog::Log(LOGERROR, "%s - unable to GetControl", __FUNCTION__);
    return false;
  }

  cmyth_proglist_t scheduled = m_dll->proglist_get_all_scheduled(m_control);
  int count = m_dll->proglist_get_count(scheduled);
  for(int i=0; i<count; i++)
  {
    cmyth_proginfo_t programme = m_dll->proglist_get_item(scheduled, i);
    if(programme)
    {
      CEPGInfoTag tag = FillProgrammeTag(programme);
      CFileItemPtr item(new CFileItem(tag));    
      results->Add(item);
      m_dll->ref_release(programme);
    }
  }
  m_dll->ref_release(scheduled);
  //Release();
  return true;
}

bool PVRClientMythTv::GetUpcomingRecordings( CFileItemList* results )
{
  if (!GetLibrary() || !GetControl())
    return false;

  cmyth_conn_t m_control = m_session->GetControl();
  if(!m_control)
  {
    CLog::Log(LOGERROR, "%s - unable to GetControl", __FUNCTION__);
    return false;
  }

  cmyth_proglist_t pending = m_dll->proglist_get_all_pending(m_control);
  int count = m_dll->proglist_get_count(pending);
  for(int i=0; i<count; i++)
  {
    cmyth_proginfo_t programme = m_dll->proglist_get_item(pending, i);
    if(programme)
    {
      CEPGInfoTag tag = FillProgrammeTag(programme);
      CFileItemPtr item(new CFileItem(tag));
      results->Add(item);
      m_dll->ref_release(programme);
    }
  }

  m_dll->ref_release(pending);
  //Release();
  return true;
}

bool PVRClientMythTv::GetConflicting(CFileItemList* conflicts)
{
  if (!GetLibrary() || !GetControl())
    return false;

  cmyth_proglist_t conflicting = m_dll->proglist_get_conflicting(m_control);
  int count = m_dll->proglist_get_count(conflicting);
  for(int i=0; i<count; i++)
  {
    cmyth_proginfo_t programme = m_dll->proglist_get_item(conflicting, i);
    if(programme)
    {
      CEPGInfoTag tag = FillProgrammeTag(programme);
      CFileItemPtr item(new CFileItem(tag));
      conflicts->Add(item);
      m_dll->ref_release(programme);
    }
  }
  m_dll->ref_release(conflicting);

  return true;
}

bool PVRClientMythTv::GetAllRecordings(CFileItemList* results)
{
  if (!GetLibrary() || !GetControl())
    return false;

  cmyth_proglist_t list = m_dll->proglist_get_all_recorded(m_control);
  if(!list)
  {
    CLog::Log(LOGERROR, "%s - unable to get list of recordings", __FUNCTION__);
    return false;
  }
  int count = m_dll->proglist_get_count(list);

  for(int i=0; i<count; i++)
  {
    cmyth_proginfo_t program = m_dll->proglist_get_item(list, i);
    if(program)
    {
      if(GetValue(m_dll->proginfo_recgroup(program)).Equals("LiveTV"))
      {
        m_dll->ref_release(program);
        continue;
      }

      CStdString name, path;

      int recstatus;
      recstatus = GetRecordingStatus(program);
      path = GetValue(m_dll->proginfo_pathname(program));
      name = GetValue(m_dll->proginfo_title(program));

      CLog::Log(LOGDEBUG, "Recordings: Name: %s. RecStatus: %i", name.c_str(), recstatus);

    /*  CFileItemPtr item(new CFileItem("", false));
      UpdateRecording(*item, program);

      url.SetFileName("recordings/" + path);
      url.GetURL(item->m_strPath);

      url.SetFileName("files/" + path +  ".png");
      url.GetURL(path);
      item->SetThumbnailImage(path);

      if(m_dll->proginfo_rec_status(program) != RS_RECORDING)
        name += " (" + item->m_dateTime.GetAsLocalizedDateTime() + ")";
      else
      {
        name += " (Recording)";
        item->SetThumbnailImage("");
      }

      item->SetLabel(name);

      results->Add(item);
      m_dll->ref_release(program);*/
    }

    /*if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
      results->AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551, LABEL_MASKS("%Z (%J)", "%I", "%L", ""));
    else
      results->AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%Z (%J)", "%I", "%L", ""));
    results->AddSortMethod(SORT_METHOD_SIZE, 553, LABEL_MASKS("%Z (%J)", "%I", "%L", "%I"));
    results->AddSortMethod(SORT_METHOD_DATE, 552, LABEL_MASKS("%Z", "%J %Q", "%L", "%J"));*/

  }
  m_dll->ref_release(list);
  return true;
}
int PVRClientMythTv::GetChannelList(PVRCLIENT_CHANNEL* channel)
{
  if (!GetLibrary() || !GetControl() || !GetDB())
    return 0;

  PVR_CHANLIST xbmcChanList;


  cmyth_chanlist_t chanlist = m_dll->mysql_get_chanlist(m_database);
  int numChannels = m_dll->chanlist_get_count(chanlist);
  for (int i=0; i<numChannels; i++)
  {
    cmyth_channel_t channel = m_dll->chanlist_get_item(chanlist, i);
    if (m_dll->channel_visible(channel) == 0)
      continue; // this channel is hidden on backend, ignore it

    xbmcChanList.push_back(GetXBMCChannel(channel));
  }

  return numChannels;
}

int PVRClientMythTv::GetNumChannels()
{
  if (!GetLibrary() || !GetControl() || !GetDB())
    return 0;

  cmyth_chanlist_t channels = m_dll->mysql_get_chanlist(m_database);
  return m_dll->chanlist_get_count(channels);
}

void PVRClientMythTv::GetEPGForChannel(int bouquet, int channel)
{
  // store this task for the worker thread
  CStdString chan;
  chan.Format("%u", channel);
  AddTask(GET_EPG_FOR_CHANNEL, chan);
}

void PVRClientMythTv::GetEPGForChannelTask(CStdString chan)
{
  if (!GetLibrary() || !GetControl() || !GetDB())
    return;

  cmyth_program_t *prog = NULL;

  //int count = m_dll->mysql_get_guide(m_database, &prog, now, end);
  //if (count <= 0)
  //  return;

  //int chanNum = atoi(chan.c_str());

  //for (int i = 0; i < count; i++)
  //{
  //  if (prog[i].channum == chanNum)
  //  {
  //    CFileItemPtr item(new CFileItem(prog[i].title, false));
  //    item->SetLabel(prog[i].title);
  //    item->m_dateTime = prog[i].starttime;
  //    item->SetLabelPreformated(true);

  //    CEPGInfoTag* tag = item->GetEPGInfoTag();

  //    tag->m_strAlbum       = GetValue(prog[i].callsign);
  //    tag->m_strShowTitle   = GetValue(prog[i].title);
  //    tag->m_strPlotOutline = GetValue(prog[i].subtitle);
  //    tag->m_strPlot        = GetValue(prog[i].description);
  //    tag->m_strGenre       = GetValue(prog[i].category);

  //    if(tag->m_strPlot.Left(tag->m_strPlotOutline.length()) != tag->m_strPlotOutline && !tag->m_strPlotOutline.IsEmpty())
  //      tag->m_strPlot = tag->m_strPlotOutline + '\n' + tag->m_strPlot;
  //    tag->m_strOriginalTitle = tag->m_strShowTitle;

  //    tag->m_strTitle = tag->m_strAlbum;
  //    if(tag->m_strShowTitle.length() > 0)
  //      tag->m_strTitle += " : " + tag->m_strShowTitle;

  //    CDateTimeSpan span(endtime.GetDay() - starttime.GetDay(),
  //      endtime.GetHour() - starttime.GetHour(),
  //      endtime.GetMinute() - starttime.GetMinute(),
  //      endtime.GetSecond() - starttime.GetSecond());

  //    StringUtils::SecondsToTimeString( span.GetSeconds()
  //      + span.GetMinutes() * 60 
  //      + span.GetHours() * 3600, tag->m_strRuntime, TIME_FORMAT_GUESS);

  //    tag->m_iSeason  = 0; /* set this so xbmc knows it's a tv show */
  //    tag->m_iEpisode = 0;
  //    tag->m_strStatus = prog[i].rec_status;
  //    items.Add(item);
  //  }
  //}
  //m_dll->ref_release(prog);
  return;

}

/* Process loop for this thread **********************************************/
void PVRClientMythTv::Process()
{
  CSingleLock lock(m_thingsToDoSection);

  CLog::Log(LOGDEBUG, "PVRClient:%u Begin Processing %u Tasks", m_clientID, m_thingsToDo.size());
  m_isRunning = true;

  while (!m_bStop && m_thingsToDo.size())
  {
    int task         = m_thingsToDo.front().first;
    CStdString data  = m_thingsToDo.front().second;
    m_thingsToDo.pop();

    /* remove the lock once we know what task to perform */
    lock.Leave();

    switch (task) {
    case CMYTH_EVENT_SCHEDULE_CHANGE:
        m_manager->OnClientMessage(m_clientID, PVRCLIENT_EVENT_SCHEDULE_CHANGE, data.c_str());
        break;
    case GET_EPG_FOR_CHANNEL:
      GetEPGForChannelTask(data.c_str());
      break;
    }
    /* lock before we check for new tasks */
    lock.Enter();
  }

  m_isRunning = false;
  CLog::Log(LOGDEBUG, "PVRClient:%u Finished Processing Tasks", m_clientID);
}

void PVRClientMythTv::AddTask(myth_task_t task, std::string &data)
{
  /* lock the tasklist, other threads may be reading from it */
  CSingleLock lock(m_thingsToDoSection);

  /* add new task */
  m_thingsToDo.push(std::make_pair<int, std::string>(task, data));

  /* if not running start a new thread */
  if (!m_isRunning)
  {
    /* magic recipe to avoid exceptions */
    StopThread();
    Create();
    m_isRunning = true;
  }
  /* remove lock */
}

CEPGInfoTag PVRClientMythTv::FillProgrammeTag(cmyth_proginfo_t programme)
{
  CEPGInfoTag tag(m_clientID);
  tag.m_channelNum = (int) GetValue(m_dll->proginfo_chan_id(programme));
  tag.m_strTitle = GetValue(m_dll->proginfo_title(programme));
  tag.m_startTime = GetValue(m_dll->proginfo_start(programme));
  tag.m_endTime = GetValue(m_dll->proginfo_end(programme));
  
  tag.m_recStatus = (RecStatus) GetRecordingStatus(programme); ///
  return tag;
}

PVRCLIENT_CHANNEL PVRClientMythTv::GetXBMCChannel(cmyth_channel_t channel)
{
  PVRCLIENT_CHANNEL chan;
  chan.Name = m_dll->channel_name(channel);
  chan.Callsign = chan.Name;
  chan.IconPath = m_dll->channel_icon(channel);
  chan.Number = m_dll->channel_channum(channel);

  return chan;
}

bool PVRClientMythTv::UpdateRecording(CFileItem &item, cmyth_proginfo_t info)
{
  /*if(!info)
    return false;


  CEPGInfoTag* tag = item.GetEPGInfoTag();

  tag->m_strAlbum       = GetValue(m_dll->proginfo_chansign(info));
  tag->m_strTitle       = GetValue(m_dll->proginfo_title(info));
  tag->m_strPlotOutline = GetValue(m_dll->proginfo_subtitle(info));
  tag->m_strPlot        = GetValue(m_dll->proginfo_description(info));
  tag->m_strGenre       = GetValue(m_dll->proginfo_category(info));
  tag->m_startTime = GetValue(m_dll->proginfo_rec_start(info));
  tag->m_endTime   =  GetValue(m_dll->proginfo_rec_end(info));

  if(tag->m_strPlot.Left(tag->m_strPlotOutline.length()) != tag->m_strPlotOutline && !tag->m_strPlotOutline.IsEmpty())
    tag->m_strPlot = tag->m_strPlotOutline + '\n' + tag->m_strPlot;

  tag->m_strTitle = tag->m_strAlbum;
  if(tag->m_strShowTitle.length() > 0)
    tag->m_strTitle += " : " + tag->m_strShowTitle;



  item.m_dwSize   = m_dll->proginfo_length(info);

  StringUtils::SecondsToTimeString( span.GetSeconds()
    + span.GetMinutes() * 60 
    + span.GetHours() * 3600, tag->m_strRuntime, TIME_FORMAT_GUESS);

  item.m_strTitle = GetValue(m_dll->proginfo_chanstr(info));
  

  if(m_dll->proginfo_rec_status(info) == RS_RECORDING)
  {
    tag->m_strStatus = "livetv";

    CStdString temp;

    temp = GetValue(m_dll->proginfo_chanicon(info));
    if(temp.length() > 0)
    {
      CURL url(item.m_strPath);
      url.SetFileName("files/channels/" + temp);
      url.GetURL(temp);
      item.SetThumbnailImage(temp);
    }

    temp = GetValue(m_dll->proginfo_chanstr(info));
    if(temp.length() > 0)
    {
      CURL url(item.m_strPath);
      url.SetFileName("channels/" + temp + ".ts");
      url.GetURL(temp);
      if(item.m_strPath != temp)
        item.m_strPath = temp;
    }
    item.SetCachedVideoThumb();
  }*/


  return true;
}

int PVRClientMythTv::GetRecordingStatus(cmyth_proginfo_t prog)
{
  if (!m_dll)
    return rsUnknown;

  switch (m_dll->proginfo_rec_status(prog))
  {
  case RS_DELETED:
  case RS_STOPPED:
  case RS_RECORDED:
  case RS_RECORDING:
  case RS_WILL_RECORD:
  case RS_DONT_RECORD:
  case RS_PREVIOUS_RECORDING:
  case RS_CURRENT_RECORDING:
  case RS_EARLIER_RECORDING:
  case RS_TOO_MANY_RECORDINGS:
  case RS_CANCELLED:
  case RS_CONFLICT:
  case RS_LATER_SHOWING:
  case RS_REPEAT:
  case RS_LOW_DISKSPACE:
  case RS_TUNER_BUSY:
    return m_dll->proginfo_rec_status(prog);

  default:
    return rsUnknown;
  }
}
bool PVRClientMythTv::GetLibrary()
{
  m_session = XFILE::CCMythSession::AquireSession(m_connString);
  m_dll = m_session->GetLibrary();
  if(!m_dll)
  {
    CLog::Log(LOGERROR, "%s - unable to GetLibrary", __FUNCTION__); /// send logging thru pvrmanager
    return false;
  }

  return true;
}

bool PVRClientMythTv::GetControl()
{
  m_session = XFILE::CCMythSession::AquireSession(m_connString); 
  m_control = m_session->GetControl();
  if(!m_control)
  {
    CLog::Log(LOGERROR, "%s - unable to GetControl", __FUNCTION__);
    return false;
  }

  return true;
}

bool PVRClientMythTv::GetDB()
{
  m_session = XFILE::CCMythSession::AquireSession(m_connString); 
  m_database = m_session->GetDatabase();
  if(!m_database)
  {
    CLog::Log(LOGERROR, "%s - unable to GetDB", __FUNCTION__);
    return false;
  }

  return true;
}
