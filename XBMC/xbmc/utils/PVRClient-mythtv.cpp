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
#include "URL.h"

PVRClientMythTv::PVRClientMythTv(DWORD sourceID, IPVRClientCallback *callback)
  : IPVRClient(sourceID, callback)
{
  CLog::Log(LOGERROR, "%s - constructor", __FUNCTION__);
  m_sourceID = sourceID;
  m_manager = callback;
}

PVRClientMythTv::~PVRClientMythTv()
{
  CLog::Log(LOGERROR, "%s - deconstructor", __FUNCTION__);
  Release();
}

PVRClientMythTv::Release()
{
  if(m_recorder)
  {
    m_dll->ref_release(m_recorder);
    m_recorder = NULL;
  }
  if(m_session)
  {
    CCMythSession::ReleaseSession(m_session);
    m_session = NULL;
  }
  m_dll = NULL;
}

//bool PVRClientMythTv::GetEPGDataEnd(CDateTime &end)
//{
//  CStdString strpath = "myth://myth:myth@tv"; ///TODO store individual settings for each client
//  CURL url(strpath);
//
//  m_session = XFILE::CCMythSession::AquireSession(strpath);
//  m_dll = m_session->GetLibrary();
//  time_t = m_dll->
//
//}

bool PVRClientMythTv::GetDriveSpace(long long *total, long long *used)
{
  CStdString strpath = "myth://myth:myth@tv"; ///TODO store individual settings for each client
  CURL url(strpath);

  m_session = XFILE::CCMythSession::AquireSession(strpath);
  m_dll = m_session->GetLibrary();

  cmyth_conn_t control = m_session->GetControl();
  if(!control)
  {
    CLog::Log(LOGERROR, "%s - unable to GetControl", __FUNCTION__);
    return false;
  }

  if(m_dll->conn_get_freespace(control, total, used) != 0)
  {
    Release();
    return false;
  }

  Release();
  return true;
}

bool PVRClientMythTv::GetRecordingSchedules( CFileItemList &results )
{
  CStdString strpath = "myth://myth:myth@tv"; ///TODO store individual settings for each client
  CURL url(strpath);

  m_session = XFILE::CCMythSession::AquireSession(strpath);
  m_dll = m_session->GetLibrary();
  
  cmyth_conn_t control = m_session->GetControl();
  if(!control)
  {
    CLog::Log(LOGERROR, "%s - unable to GetControl", __FUNCTION__);
    return false;
  }

  cmyth_proglist_t scheduled = m_dll->proglist_get_all_scheduled(control);
  int count = m_dll->proglist_get_count(scheduled);
  for(int i=0; i<count; i++)
  {
    cmyth_proginfo_t programme = m_dll->proglist_get_item(scheduled, i);
    if(programme)
    {
      CStdString name, path;

      path = m_dll->proginfo_pathname(programme);
      name = m_dll->proginfo_title(programme);

      CLog::Log(LOGDEBUG, "TV: get all scheduled: %s", name.c_str());
      /*CFileItemPtr item(new CFileItem("", false));
      m_session->UpdateItem(*item, programme);

      url.SetFileName("recordings/" + path);
      url.GetURL(item->m_strPath);

      url.SetFileName("files/" + path +  ".png");
      url.GetURL(path);
      item->SetThumbnailImage(path);

      if(m_dll->proginfo_rec_status(programme) != RS_RECORDING)
        name += " (" + item->m_dateTime.GetAsLocalizedDateTime() + ")";
      else
      {
        name += " (Recording)";
        item->SetThumbnailImage("");
      }

      item->SetLabel(name);

      results.Add(item);*/
      m_dll->ref_release(programme);
    }
  }
  m_dll->ref_release(scheduled);
  Release();
  return true;
}

bool PVRClientMythTv::GetUpcomingRecordings( CFileItemList &results )
{
  CStdString strpath = "myth://myth:myth@tv"; ///TODO store individual settings for each client
  CURL url(strpath);

  m_session = XFILE::CCMythSession::AquireSession(strpath);
  m_dll = m_session->GetLibrary();

  cmyth_conn_t control = m_session->GetControl();
  if(!control)
  {
    CLog::Log(LOGERROR, "%s - unable to GetControl", __FUNCTION__);
    return false;
  }

  cmyth_proglist_t pending = m_dll->proglist_get_all_pending(control);
  int count = m_dll->proglist_get_count(pending);
  for(int i=0; i<count; i++)
  {
    cmyth_proginfo_t programme = m_dll->proglist_get_item(pending, i);
    if(programme)
    {
      CFileItemPtr item(FillProgramme(programme));
      results.Add(item);
      m_dll->ref_release(programme);
    }
  }

  m_dll->ref_release(pending);
  Release();
  return true;
}

bool PVRClientMythTv::GetConflicting(CFileItemList &conflicts )
{
  CStdString strpath = "myth://myth:myth@tv"; ///TODO store individual settings for each client
  CURL url(strpath);

  m_session = XFILE::CCMythSession::AquireSession(strpath);
  m_dll = m_session->GetLibrary();

  cmyth_conn_t control = m_session->GetControl();
  if(!control)
  {
    CLog::Log(LOGERROR, "%s - unable to GetControl", __FUNCTION__);
    return false;
  }

  cmyth_proglist_t conflicting = m_dll->proglist_get_conflicting(control);
  int count = m_dll->proglist_get_count(conflicting);
  for(int i=0; i<count; i++)
  {
    cmyth_proginfo_t programme = m_dll->proglist_get_item(conflicting, i);
    if(programme)
    {
      //assert(m_dll->proginfo_rec_status(programme) == RS_RECORDING); //
      CStdString name, path;

      path = m_dll->proginfo_pathname(programme);
      name = m_dll->proginfo_title(programme);

      CLog::Log(LOGDEBUG, "TV: get conflicting: %s", name.c_str());
      CFileItemPtr item(FillProgramme(programme));

      url.SetFileName("recordings/" + path);
      url.GetURL(item->m_strPath);

      url.SetFileName("files/" + path +  ".png");
      url.GetURL(path);
      item->SetThumbnailImage(path);

      if(m_dll->proginfo_rec_status(programme) != RS_RECORDING)
      name += " (" + item->m_dateTime.GetAsLocalizedDateTime() + ")";
      else
      {
      name += " (Recording)";
      item->SetThumbnailImage("");
      }

      item->SetLabel(name);

      conflicts.Add(item);
      m_dll->ref_release(programme);
    }
  }
  m_dll->ref_release(conflicting);
  Release();
  return true;
}

void PVRClientMythTv::GetChannelList(CFileItemList &channels)
{

}

int PVRClientMythTv::GetNumChannels()
{
  return 0;
}

void PVRClientMythTv::GetEPGForChannel(int bouquet, int channel, CFileItemList &channelData)
{

}

void PVRClientMythTv::Process()
{
  int process = 0;
  //char buf[128];
  //int  next;
  //int  xbmcevent;

  //struct timeval to;

  //while(!m_bStop)
  //{
  //  /* check if there are any new events */
  //  to.tv_sec = 0;
  //  to.tv_usec = 100000;
  //  if(m_dll->event_select(m_event, &to) <= 0)
  //    continue;

  //  next = m_dll->event_get(m_event, buf, sizeof(buf));
  //  buf[sizeof(buf)-1] = 0;

  //  switch (next) {
  //case CMYTH_EVENT_UNKNOWN:
  //  CLog::Log(LOGDEBUG, "%s - MythTV unknown event (error?)", __FUNCTION__);
  //  break;
  //case CMYTH_EVENT_CLOSE:
  //  CLog::Log(LOGDEBUG, "%s - MythTV event CMYTH_EVENT_CLOSE", __FUNCTION__);
  //  xbmcevent = next;
  //  break;
  //case CMYTH_EVENT_RECORDING_LIST_CHANGE:
  //  CLog::Log(LOGDEBUG, "%s - MythTV event RECORDING_LIST_CHANGE", __FUNCTION__);
  //  xbmcevent = next;
  //  break;
  //case CMYTH_EVENT_SCHEDULE_CHANGE:
  //  CLog::Log(LOGDEBUG, "%s - MythTV event SCHEDULE_CHANGE", __FUNCTION__);
  //  xbmcevent = next;
  //  break;
  //case CMYTH_EVENT_DONE_RECORDING:
  //  CLog::Log(LOGDEBUG, "%s - MythTV event DONE_RECORDING", __FUNCTION__);
  //  xbmcevent = next;
  //  break;
  //case CMYTH_EVENT_QUIT_LIVETV:
  //  CLog::Log(LOGDEBUG, "%s - MythTV event QUIT_LIVETV", __FUNCTION__);
  //  xbmcevent = 0;
  //  break;
  //case CMYTH_EVENT_LIVETV_CHAIN_UPDATE:
  //  CLog::Log(LOGDEBUG, "%s - MythTV event %s", __FUNCTION__, buf);
  //  xbmcevent = 0;
  //  break;
  //case CMYTH_EVENT_SIGNAL:
  //  CLog::Log(LOGDEBUG, "%s - MythTV event SIGNAL", __FUNCTION__);
  //  xbmcevent = 0;
  //  break;
  //case CMYTH_EVENT_ASK_RECORDING:
  //  CLog::Log(LOGDEBUG, "%s - MythTV event CMYTH_EVENT_ASK_RECORDING", __FUNCTION__);
  //  xbmcevent = 0;
  //  break;
  //  }

  //  { CSingleLock lock(m_section);
  //  if(m_manager && xbmcevent != PVR_EVENT_UNKNOWN)
  //    m_manager->OnMessage(xbmcevent, buf);
  //  }
  //}
}

CEPGInfoTag PVRClientMythTv::FillProgramme(cmyth_proginfo_t programme)
{
  CEPGInfoTag tag = new CEPGInfoTag(


}