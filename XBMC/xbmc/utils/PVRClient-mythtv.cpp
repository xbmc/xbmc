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
  m_mythEvents = CCMythSession::AquireSession();
  m_mythEvents->SetListener(this);

  m_sourceID = sourceID;
  m_manager = callback;
}

PVRClientMythTv::~PVRClientMythTv()
{
  CLog::Log(LOGERROR, "%s - deconstructor", __FUNCTION__);
  Release();
}

void PVRClientMythTv::Release()
{
  if(m_recorder) ///
  {
    m_dll->ref_release(m_recorder);
    m_recorder = NULL;
  }
  if(m_session)
  {
    XFILE::CCMythSession::ReleaseSession(m_session);
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
    return false;
  }

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

      CLog::Log(LOGDEBUG, "TV: get schedules: %s", name.c_str());

      //CFileItemPtr item(new CFileItem("", false));
      //m_session->UpdateItem(*item, programme);

      //if(m_dll->proginfo_rec_status(programme) != RS_RECORDING)
      //  name += " (" + item->m_dateTime.GetAsLocalizedDateTime() + ")";
      //else
      //{
      //  name += " (Recording)";
      //  item->SetThumbnailImage("");
      //}

      //item->SetLabel(name);

      //results.Add(item);

      m_dll->ref_release(programme);
    }
  }
  m_dll->ref_release(scheduled);
  //Release();
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
      CStdString name, path, recgroup, sourceName, recstatus;

      path = m_dll->proginfo_pathname(programme);
      name = m_dll->proginfo_title(programme);
      recgroup = m_dll->proginfo_recgroup(programme);
  
      CLog::Log(LOGDEBUG, "TV: get upcoming: %s. RecStatus: %s. RecGroup: %s. SourceID: ", name.c_str(), recstatus.c_str(), recgroup.c_str());
      //CFileItemPtr item(new CFileItem(FillProgrammeTag(programme)));
      //results.Add(item);
      m_dll->ref_release(programme);
    }
  }

  m_dll->ref_release(pending);
  //Release();
  return true;
}

bool PVRClientMythTv::GetConflicting(CFileItemList &conflicts)
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
      /*CFileItemPtr item(new CFileItem(FillProgrammeTag(programme)));

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

      conflicts.Add(item);*/
      m_dll->ref_release(programme);
    }
  }
  m_dll->ref_release(conflicting);
  //Release();
  return true;
}

bool PVRClientMythTv::GetAllRecordings(CFileItemList &results)
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

  cmyth_proglist_t list = m_dll->proglist_get_all_recorded(control);
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

      results.Add(item);
      m_dll->ref_release(program);*/
    }

    /*if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
      results.AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551, LABEL_MASKS("%Z (%J)", "%I", "%L", ""));
    else
      results.AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%Z (%J)", "%I", "%L", ""));
    results.AddSortMethod(SORT_METHOD_SIZE, 553, LABEL_MASKS("%Z (%J)", "%I", "%L", "%I"));
    results.AddSortMethod(SORT_METHOD_DATE, 552, LABEL_MASKS("%Z", "%J %Q", "%L", "%J"));*/

  }
  m_dll->ref_release(list);
  return true;
}
void PVRClientMythTv::GetChannelList(CFileItemList &channels)
{

}

int  PVRClientMythTv::GetNumChannels()
{
  return 0;
}

void PVRClientMythTv::GetEPGForChannel(int bouquet, int channel, CFileItemList &channelData)
{

}

void PVRClientMythTv::Process()
{
  int process = 0;
}

void PVRClientMythTv::Process()
{
  while (!m_bStop)
  {
    WaitForSingleObject(m_hWorkerEvent, INFINITE);
    if (m_bStop)
      break;
    int iNrCachedTracks = m_RadioTrackQueue->size();
    if (iNrCachedTracks == 0)
    {
      RequestRadioTracks();
    }
    CSingleLock lock(m_lockCache);
    iNrCachedTracks = m_RadioTrackQueue->size();
    CacheTrackThumb(iNrCachedTracks);
  }
  CLog::Log(LOGINFO,"LastFM thread terminated");
}

CEPGInfoTag PVRClientMythTv::FillProgrammeTag(cmyth_proginfo_t programme)
{
  CEPGInfoTag tag(m_sourceID);
  switch (m_dll->proginfo_rec_status(programme))
  {
    case RS_DELETED:
      tag.m_recStatus = rsDeleted;
      break;
    case RS_STOPPED:
      tag.m_recStatus = rsStopped;
      break;
    case RS_RECORDED:
      tag.m_recStatus = rsRecorded;
      break;
    case RS_WILL_RECORD:
      tag.m_recStatus = rsWillRecord;
      break;
    case RS_DONT_RECORD:
      tag.m_recStatus = rsDontRecord;
      break;
    case RS_PREVIOUS_RECORDING:
      tag.m_recStatus = rsPrevRecording;
      break;
    case RS_CURRENT_RECORDING:
      tag.m_recStatus = rsCurrentRecording;
      break;
    case RS_EARLIER_RECORDING:
      tag.m_recStatus = rsEarlierRecording;
      break;
    case RS_TOO_MANY_RECORDINGS:
      tag.m_recStatus = rsTooManyRecordings;
      break;
    case RS_CANCELLED:
      tag.m_recStatus = rsCancelled;
      break;
    case RS_CONFLICT:
      tag.m_recStatus = rsConflict;
      break;
    case RS_LATER_SHOWING:
      tag.m_recStatus = rsLaterShowing;
      break;
    case RS_REPEAT:
      tag.m_recStatus = rsRepeat;
      break;
    case RS_LOW_DISKSPACE:
      tag.m_recStatus = rsLowDiskspace;
      break;
    case RS_TUNER_BUSY:
      tag.m_recStatus = rsTunerBusy;
      break;
    default:
      tag.m_recStatus = rsUnknown;
  }
  
  return tag;

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