/*
 *      Copyright (c) 2006 elupus (Joakim Plate)
 *      Copyright (C) 2006-2013 Team XBMC
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
#include <Platinum/Source/Platinum/Platinum.h>
#include <Platinum/Source/Devices/MediaRenderer/PltMediaController.h>
#include <Platinum/Source/Devices/MediaServer/PltDidl.h>

#include "UPnPPlayer.h"
#include "UPnP.h"
#include "UPnPInternal.h"
#include "FileItem.h"
#include "threads/Event.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "utils/Variant.h"
#include "GUIInfoManager.h"
#include "ThumbLoader.h"
#include "video/VideoThumbLoader.h"
#include "music/MusicThumbLoader.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogHelper.h"
#include "Application.h"
#include "dialogs/GUIDialogBusy.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"

using namespace KODI::MESSAGING;

using KODI::MESSAGING::HELPERS::DialogResponse;

NPT_SET_LOCAL_LOGGER("xbmc.upnp.player")

namespace UPNP
{

class CUPnPPlayerController
  : public PLT_MediaControllerDelegate
{
public:
  CUPnPPlayerController(PLT_MediaController* control, PLT_DeviceDataReference& device, IPlayerCallback& callback)
    : m_control(control)
    , m_transport(NULL)
    , m_device(device)
    , m_instance(0)
    , m_callback(callback)
    , m_postime(0)
  {
    memset(&m_posinfo, 0, sizeof(m_posinfo));
    m_device->FindServiceByType("urn:schemas-upnp-org:service:AVTransport:1", m_transport);
  }

  virtual void OnSetAVTransportURIResult(NPT_Result res, PLT_DeviceDataReference& device, void* userdata)
  {
    if(NPT_FAILED(res))
      CLog::Log(LOGERROR, "UPNP: CUPnPPlayer : OnSetAVTransportURIResult failed");
    m_resstatus = res;
    m_resevent.Set();
  }

  virtual void OnPlayResult(NPT_Result res, PLT_DeviceDataReference& device, void* userdata)
  {
    if(NPT_FAILED(res))
      CLog::Log(LOGERROR, "UPNP: CUPnPPlayer : OnPlayResult failed");
    m_resstatus = res;
    m_resevent.Set();
  }

  virtual void OnStopResult(NPT_Result res, PLT_DeviceDataReference& device, void* userdata)
  {
    if(NPT_FAILED(res))
      CLog::Log(LOGERROR, "UPNP: CUPnPPlayer : OnStopResult failed");
    m_resstatus = res;
    m_resevent.Set();
  }

  virtual void OnGetMediaInfoResult(NPT_Result res, PLT_DeviceDataReference& device, PLT_MediaInfo* info, void* userdata)
  {
    if(NPT_FAILED(res) || info == NULL)
      CLog::Log(LOGERROR, "UPNP: CUPnPPlayer : OnGetMediaInfoResult failed");
  }

  virtual void OnGetTransportInfoResult(NPT_Result res, PLT_DeviceDataReference& device, PLT_TransportInfo* info, void* userdata)
  {
    CSingleLock lock(m_section);

    if(NPT_FAILED(res))
    {
      CLog::Log(LOGERROR, "UPNP: CUPnPPlayer : OnGetTransportInfoResult failed");
      m_trainfo.cur_speed            = "0";
      m_trainfo.cur_transport_state  = "STOPPED";
      m_trainfo.cur_transport_status = "ERROR_OCCURED";
    }
    else
      m_trainfo = *info;
    m_traevnt.Set();
  }

  void UpdatePositionInfo()
  {
    if(m_postime == 0
    || m_postime > CTimeUtils::GetFrameTime())
      return;

    m_control->GetTransportInfo(m_device, m_instance, this);
    m_control->GetPositionInfo(m_device, m_instance, this);
    m_postime = 0;
  }

  virtual void OnGetPositionInfoResult(NPT_Result res, PLT_DeviceDataReference& device, PLT_PositionInfo* info, void* userdata)
  {
    CSingleLock lock(m_section);

    if(NPT_FAILED(res) || info == NULL)
    {
      CLog::Log(LOGERROR, "UPNP: CUPnPPlayer : OnGetMediaInfoResult failed");
      m_posinfo = PLT_PositionInfo();
    }
    else
      m_posinfo = *info;
    m_postime = CTimeUtils::GetFrameTime() + 500;
    m_posevnt.Set();
  }


  ~CUPnPPlayerController()
  {
  }

  PLT_MediaController*     m_control;
  PLT_Service *            m_transport;
  PLT_DeviceDataReference  m_device;
  NPT_UInt32               m_instance;
  IPlayerCallback&         m_callback;

  NPT_Result               m_resstatus;
  CEvent                   m_resevent;

  CCriticalSection         m_section;
  unsigned int             m_postime;

  CEvent                   m_posevnt;
  PLT_PositionInfo         m_posinfo;

  CEvent                   m_traevnt;
  PLT_TransportInfo        m_trainfo;
};

CUPnPPlayer::CUPnPPlayer(IPlayerCallback& callback, const char* uuid)
: IPlayer(callback)
, m_control(NULL)
, m_delegate(NULL)
, m_started(false)
, m_stopremote(false)
{
  m_control  = CUPnP::GetInstance()->m_MediaController;

  PLT_DeviceDataReference device;
  if(NPT_SUCCEEDED(m_control->FindRenderer(uuid, device)))
  {
    m_delegate = new CUPnPPlayerController(m_control, device, callback);
    CUPnP::RegisterUserdata(m_delegate);
  }
  else
    CLog::Log(LOGERROR, "UPNP: CUPnPPlayer couldn't find device as %s", uuid);
}

CUPnPPlayer::~CUPnPPlayer()
{
  CloseFile();
  CUPnP::UnregisterUserdata(m_delegate);
  delete m_delegate;
}

static NPT_Result WaitOnEvent(CEvent& event, XbmcThreads::EndTime& timeout, CGUIDialogBusy*& dialog)
{
  if(event.WaitMSec(0))
    return NPT_SUCCESS;

  if (!CGUIDialogBusy::WaitOnEvent(event))
    return NPT_FAILURE;

  return NPT_SUCCESS;
}

int CUPnPPlayer::PlayFile(const CFileItem& file, const CPlayerOptions& options, CGUIDialogBusy*& dialog, XbmcThreads::EndTime& timeout)
{
  CFileItem item(file);
  NPT_Reference<CThumbLoader> thumb_loader;
  NPT_Reference<PLT_MediaObject> obj;
  NPT_String path(file.GetPath().c_str());
  NPT_String tmp, resource;
  EMediaControllerQuirks quirks = EMEDIACONTROLLERQUIRKS_NONE;

  NPT_CHECK_POINTER_LABEL_SEVERE(m_delegate, failed);

  if (file.IsVideoDb())
    thumb_loader = NPT_Reference<CThumbLoader>(new CVideoThumbLoader());
  else if (item.IsMusicDb())
    thumb_loader = NPT_Reference<CThumbLoader>(new CMusicThumbLoader());

  obj = BuildObject(item, path, false, thumb_loader, NULL, CUPnP::GetServer(), UPnPPlayer);
  if(obj.IsNull()) goto failed;

  NPT_CHECK_LABEL_SEVERE(PLT_Didl::ToDidl(*obj, "", tmp), failed_todidl);
  tmp.Insert(didl_header, 0);
  tmp.Append(didl_footer);

  quirks = GetMediaControllerQuirks(m_delegate->m_device.AsPointer());
  if (quirks & EMEDIACONTROLLERQUIRKS_X_MKV)
  {
    for (NPT_Cardinal i=0; i< obj->m_Resources.GetItemCount(); i++) {
      if (obj->m_Resources[i].m_ProtocolInfo.GetContentType().Compare("video/x-matroska") == 0) {
        CLog::Log(LOGDEBUG, "CUPnPPlayer::PlayFile(%s): applying video/x-mkv quirk", file.GetPath().c_str());
        NPT_String protocolInfo = obj->m_Resources[i].m_ProtocolInfo.ToString();
        protocolInfo.Replace(":video/x-matroska:", ":video/x-mkv:");
        obj->m_Resources[i].m_ProtocolInfo = PLT_ProtocolInfo(protocolInfo);
      }
    }
  }

  /* The resource uri's are stored in the Didl. We must choose the best resource
   * for the playback device */
  NPT_Cardinal res_index;
  NPT_CHECK_LABEL_SEVERE(m_control->FindBestResource(m_delegate->m_device, *obj, res_index), failed_findbestresource);

  // get the transport info to evaluate the TransportState to be able to
  // determine whether we first need to call Stop()
  timeout.Set(timeout.GetInitialTimeoutValue());
  NPT_CHECK_LABEL_SEVERE(m_control->GetTransportInfo(m_delegate->m_device
                                                     , m_delegate->m_instance
                                                     , m_delegate), failed_gettransportinfo);
  NPT_CHECK_LABEL_SEVERE(WaitOnEvent(m_delegate->m_traevnt, timeout, dialog), failed_gettransportinfo);

  if (m_delegate->m_trainfo.cur_transport_state != "NO_MEDIA_PRESENT" &&
      m_delegate->m_trainfo.cur_transport_state != "STOPPED")
  {
    timeout.Set(timeout.GetInitialTimeoutValue());
    NPT_CHECK_LABEL_SEVERE(m_control->Stop(m_delegate->m_device
                                           , m_delegate->m_instance
                                           , m_delegate), failed_stop);
    NPT_CHECK_LABEL_SEVERE(WaitOnEvent(m_delegate->m_resevent, timeout, dialog), failed_stop);
    NPT_CHECK_LABEL_SEVERE(m_delegate->m_resstatus, failed_stop);
  }


  timeout.Set(timeout.GetInitialTimeoutValue());
  NPT_CHECK_LABEL_SEVERE(m_control->SetAVTransportURI(m_delegate->m_device
                                                    , m_delegate->m_instance
                                                    , obj->m_Resources[res_index].m_Uri
                                                    , (const char*)tmp
                                                    , m_delegate), failed_setavtransporturi);
  NPT_CHECK_LABEL_SEVERE(WaitOnEvent(m_delegate->m_resevent, timeout, dialog), failed_setavtransporturi);
  NPT_CHECK_LABEL_SEVERE(m_delegate->m_resstatus, failed_setavtransporturi);

  timeout.Set(timeout.GetInitialTimeoutValue());
  NPT_CHECK_LABEL_SEVERE(m_control->Play(m_delegate->m_device
                                       , m_delegate->m_instance
                                       , "1"
                                       , m_delegate), failed_play);
  NPT_CHECK_LABEL_SEVERE(WaitOnEvent(m_delegate->m_resevent, timeout, dialog), failed_play);
  NPT_CHECK_LABEL_SEVERE(m_delegate->m_resstatus, failed_play);


  /* wait for PLAYING state */
  timeout.Set(timeout.GetInitialTimeoutValue());
  do {
    NPT_CHECK_LABEL_SEVERE(m_control->GetTransportInfo(m_delegate->m_device
                                                     , m_delegate->m_instance
                                                     , m_delegate), failed_waitplaying);


    { CSingleLock lock(m_delegate->m_section);
      if(m_delegate->m_trainfo.cur_transport_state == "PLAYING"
      || m_delegate->m_trainfo.cur_transport_state == "PAUSED_PLAYBACK")
        break;

      if(m_delegate->m_trainfo.cur_transport_state  == "STOPPED"
      && m_delegate->m_trainfo.cur_transport_status != "OK")
      {
        CLog::Log(LOGERROR, "UPNP: CUPnPPlayer::OpenFile - remote player signalled error %s", file.GetPath().c_str());
        return NPT_FAILURE;
      }
    }

    NPT_CHECK_LABEL_SEVERE(WaitOnEvent(m_delegate->m_traevnt, timeout, dialog), failed_waitplaying);

  } while(!timeout.IsTimePast());

  if(options.starttime > 0)
  {
    /* many upnp units won't load file properly until after play (including xbmc) */
    NPT_CHECK_LABEL(m_control->Seek(m_delegate->m_device
                                    , m_delegate->m_instance
                                    , "REL_TIME"
                                    , PLT_Didl::FormatTimeStamp((NPT_UInt32)options.starttime)
                                    , m_delegate), failed_seek);
  }

  return NPT_SUCCESS;
failed_todidl:
  CLog::Log(LOGERROR, "CUPnPPlayer::PlayFile(%s) failed to serialize item into DIDL-Lite", file.GetPath().c_str());
  return NPT_FAILURE;
failed_findbestresource:
  CLog::Log(LOGERROR, "CUPnPPlayer::PlayFile(%s) failed to find a matching resource", file.GetPath().c_str());
  return NPT_FAILURE;
failed_gettransportinfo:
  CLog::Log(LOGERROR, "CUPnPPlayer::PlayFile(%s): call to GetTransportInfo failed", file.GetPath().c_str());
  return NPT_FAILURE;
failed_stop:
  CLog::Log(LOGERROR, "CUPnPPlayer::PlayFile(%s) failed to stop current playback", file.GetPath().c_str());
  return NPT_FAILURE;
failed_setavtransporturi:
  CLog::Log(LOGERROR, "CUPnPPlayer::PlayFile(%s) failed to set the playback URI", file.GetPath().c_str());
  return NPT_FAILURE;
failed_play:
  CLog::Log(LOGERROR, "CUPnPPlayer::PlayFile(%s) failed to start playback", file.GetPath().c_str());
  return NPT_FAILURE;
failed_waitplaying:
  CLog::Log(LOGERROR, "CUPnPPlayer::PlayFile(%s) failed to wait for PLAYING state", file.GetPath().c_str());
  return NPT_FAILURE;
failed_seek:
  CLog::Log(LOGERROR, "CUPnPPlayer::PlayFile(%s) failed to seek to start offset", file.GetPath().c_str());
  return NPT_FAILURE;
failed:
  CLog::Log(LOGERROR, "CUPnPPlayer::PlayFile(%s) failed", file.GetPath().c_str());
  return NPT_FAILURE;
}

bool CUPnPPlayer::OpenFile(const CFileItem& file, const CPlayerOptions& options)
{
  CGUIDialogBusy* dialog = NULL;
  XbmcThreads::EndTime timeout(10000);

  /* if no path we want to attach to a already playing player */
  if(file.GetPath() == "")
  {
    NPT_CHECK_LABEL_SEVERE(m_control->GetTransportInfo(m_delegate->m_device
                                                     , m_delegate->m_instance
                                                     , m_delegate), failed);

    NPT_CHECK_LABEL_SEVERE(WaitOnEvent(m_delegate->m_traevnt, timeout, dialog), failed);

    /* make sure the attached player is actually playing */
    { CSingleLock lock(m_delegate->m_section);
      if(m_delegate->m_trainfo.cur_transport_state != "PLAYING"
      && m_delegate->m_trainfo.cur_transport_state != "PAUSED_PLAYBACK")
        goto failed;
    }
  }
  else
    NPT_CHECK_LABEL_SEVERE(PlayFile(file, options, dialog, timeout), failed);

  m_stopremote = true;
  m_started = true;
  m_callback.OnPlayBackStarted();
  NPT_CHECK_LABEL_SEVERE(m_control->GetPositionInfo(m_delegate->m_device
                                                  , m_delegate->m_instance
                                                  , m_delegate), failed);
  NPT_CHECK_LABEL_SEVERE(m_control->GetMediaInfo(m_delegate->m_device
                                               , m_delegate->m_instance
                                               , m_delegate), failed);

  if(dialog)
    dialog->Close();

  return true;
failed:
  CLog::Log(LOGERROR, "UPNP: CUPnPPlayer::OpenFile - unable to open file %s", file.GetPath().c_str());
  if(dialog)
    dialog->Close();
  return false;
}

bool CUPnPPlayer::QueueNextFile(const CFileItem& file)
{
  CFileItem item(file);
  NPT_Reference<CThumbLoader> thumb_loader;
  NPT_Reference<PLT_MediaObject> obj;
  NPT_String path(file.GetPath().c_str());
  NPT_String tmp;

  if (file.IsVideoDb())
    thumb_loader = NPT_Reference<CThumbLoader>(new CVideoThumbLoader());
  else if (item.IsMusicDb())
    thumb_loader = NPT_Reference<CThumbLoader>(new CMusicThumbLoader());


  obj = BuildObject(item, path, 0, thumb_loader, NULL, CUPnP::GetServer(), UPnPPlayer);
  if(!obj.IsNull())
  {
    NPT_CHECK_LABEL_SEVERE(PLT_Didl::ToDidl(*obj, "", tmp), failed);
    tmp.Insert(didl_header, 0);
    tmp.Append(didl_footer);
  }

  NPT_CHECK_LABEL_WARNING(m_control->SetNextAVTransportURI(m_delegate->m_device
                                                         , m_delegate->m_instance
                                                         , file.GetPath().c_str()
                                                         , (const char*)tmp
                                                         , m_delegate), failed);
  if(!m_delegate->m_resevent.WaitMSec(10000)) goto failed;
  NPT_CHECK_LABEL_WARNING(m_delegate->m_resstatus, failed);
  return true;

failed:
  CLog::Log(LOGERROR, "UPNP: CUPnPPlayer::QueueNextFile - unable to queue file %s", file.GetPath().c_str());
  return false;
}

bool CUPnPPlayer::CloseFile(bool reopen)
{
  NPT_CHECK_POINTER_LABEL_SEVERE(m_delegate, failed);
  if(m_stopremote)
  {
    NPT_CHECK_LABEL(m_control->Stop(m_delegate->m_device
                                  , m_delegate->m_instance
                                  , m_delegate), failed);
    if(!m_delegate->m_resevent.WaitMSec(10000)) goto failed;
    NPT_CHECK_LABEL(m_delegate->m_resstatus, failed);
  }

  if(m_started)
  {
    m_started = false;
    m_callback.OnPlayBackStopped();
  }

  return true;
failed:
  CLog::Log(LOGERROR, "UPNP: CUPnPPlayer::CloseFile - unable to stop playback");
  return false;
}

void CUPnPPlayer::Pause()
{
  if(IsPaused())
    NPT_CHECK_LABEL(m_control->Play(m_delegate->m_device
                                  , m_delegate->m_instance
                                  , "1"
                                  , m_delegate), failed);
  else
    NPT_CHECK_LABEL(m_control->Pause(m_delegate->m_device
                                   , m_delegate->m_instance
                                   , m_delegate), failed);

  return;
failed:
  CLog::Log(LOGERROR, "UPNP: CUPnPPlayer::CloseFile - unable to pause/unpause playback");
  return;
}

void CUPnPPlayer::SeekTime(__int64 ms)
{
  NPT_CHECK_LABEL(m_control->Seek(m_delegate->m_device
                                , m_delegate->m_instance
                                , "REL_TIME", PLT_Didl::FormatTimeStamp((NPT_UInt32)(ms / 1000))
                                , m_delegate), failed);

  g_infoManager.SetDisplayAfterSeek();
  return;
failed:
  CLog::Log(LOGERROR, "UPNP: CUPnPPlayer::SeekTime - unable to seek playback");
}

float CUPnPPlayer::GetPercentage()
{
  int64_t tot = GetTotalTime();
  if(tot)
    return 100.0f * GetTime() / tot;
  else
    return 0.0f;
}

void CUPnPPlayer::SeekPercentage(float percent)
{
  int64_t tot = GetTotalTime();
  if (tot)
    SeekTime((int64_t)(tot * percent / 100));
}

void CUPnPPlayer::Seek(bool bPlus, bool bLargeStep, bool bChapterOverride)
{
}

void CUPnPPlayer::DoAudioWork()
{
  NPT_String data;
  NPT_CHECK_POINTER_LABEL_SEVERE(m_delegate, failed);
  m_delegate->UpdatePositionInfo();

  if(m_started) {
    NPT_String uri, meta;
    NPT_CHECK_LABEL(m_delegate->m_transport->GetStateVariableValue("CurrentTrackURI", uri), failed);
    NPT_CHECK_LABEL(m_delegate->m_transport->GetStateVariableValue("CurrentTrackMetadata", meta), failed);

    if(m_current_uri  != (const char*)uri
    || m_current_meta != (const char*)meta) {
      m_current_uri  = (const char*)uri;
      m_current_meta = (const char*)meta;
      CFileItemPtr item = GetFileItem(uri, meta);
      g_application.CurrentFileItem() = *item;
      CApplicationMessenger::GetInstance().PostMsg(TMSG_UPDATE_CURRENT_ITEM, 0, -1, static_cast<void*>(new CFileItem(*item)));
    }

    NPT_CHECK_LABEL(m_delegate->m_transport->GetStateVariableValue("TransportState", data), failed);
    if(data == "STOPPED")
    {
      m_started = false;
      m_callback.OnPlayBackEnded();
    }
  }
  return;
failed:
  return;
}

bool CUPnPPlayer::IsPlaying() const
{
  NPT_String data;
  NPT_CHECK_POINTER_LABEL_SEVERE(m_delegate, failed);
  NPT_CHECK_LABEL(m_delegate->m_transport->GetStateVariableValue("TransportState", data), failed);
  return data != "STOPPED";
failed:
  return false;
}

bool CUPnPPlayer::IsPaused() const
{
  NPT_String data;
  NPT_CHECK_POINTER_LABEL_SEVERE(m_delegate, failed);
  NPT_CHECK_LABEL(m_delegate->m_transport->GetStateVariableValue("TransportState", data), failed);
  return data == "PAUSED_PLAYBACK";
failed:
  return false;
}

void CUPnPPlayer::SetVolume(float volume)
{
  NPT_CHECK_POINTER_LABEL_SEVERE(m_delegate, failed);
  NPT_CHECK_LABEL(m_control->SetVolume(m_delegate->m_device
                                     , m_delegate->m_instance
                                     , "Master", (int)(volume * 100)
                                     , m_delegate), failed);
  return;
failed:
  CLog::Log(LOGERROR, "UPNP: CUPnPPlayer - unable to set volume");
  return;
}

int64_t CUPnPPlayer::GetTime()
{
  NPT_CHECK_POINTER_LABEL_SEVERE(m_delegate, failed);
  return m_delegate->m_posinfo.rel_time.ToMillis();
failed:
  return 0;
}

int64_t CUPnPPlayer::GetTotalTime()
{
  NPT_CHECK_POINTER_LABEL_SEVERE(m_delegate, failed);
  return m_delegate->m_posinfo.track_duration.ToMillis();
failed:
  return 0;
};

std::string CUPnPPlayer::GetPlayingTitle()
{
  return "";
};

bool CUPnPPlayer::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
    case ACTION_STOP:
      if(IsPlaying())
      {
        //stop on remote system
        m_stopremote = HELPERS::ShowYesNoDialogText(CVariant{37022}, CVariant{37023}) == DialogResponse::YES;
        
        return false; /* let normal code handle the action */
      }
    default:
      return false;
  }
}

void CUPnPPlayer::SetSpeed(float speed)
{

}

float CUPnPPlayer::GetSpeed()
{
  if (IsPaused())
    return 0;
  else
    return 1;
}

} /* namespace UPNP */
