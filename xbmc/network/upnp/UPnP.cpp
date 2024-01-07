/*
 * UPnP Support for XBMC
 *  Copyright (c) 2006 c0diq (Sylvain Rebaud)
 *      Portions Copyright (c) by the authors of libPlatinum
 *      http://www.plutinosoft.com/blog/category/platinum/
 *  Copyright (C) 2006-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "UPnP.h"

#include "FileItem.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "UPnPInternal.h"
#include "UPnPRenderer.h"
#include "UPnPServer.h"
#include "UPnPSettings.h"
#include "URL.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "messaging/ApplicationMessenger.h"
#include "network/Network.h"
#include "profiles/ProfileManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/SystemInfo.h"
#include "utils/TimeUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"

#include <memory>
#include <mutex>
#include <set>

#include <Platinum/Source/Platinum/Platinum.h>

using namespace UPNP;
using namespace KODI::MESSAGING;

#define UPNP_DEFAULT_MAX_RETURNED_ITEMS 200
#define UPNP_DEFAULT_MIN_RETURNED_ITEMS 30

/*
# Play speed
#    1 normal
#    0 invalid
DLNA_ORG_PS = 'DLNA.ORG_PS'
DLNA_ORG_PS_VAL = '1'

# Conversion Indicator
#    1 transcoded
#    0 not transcoded
DLNA_ORG_CI = 'DLNA.ORG_CI'
DLNA_ORG_CI_VAL = '0'

# Operations
#    00 not time seek range, not range
#    01 range supported
#    10 time seek range supported
#    11 both supported
DLNA_ORG_OP = 'DLNA.ORG_OP'
DLNA_ORG_OP_VAL = '01'

# Flags
#    senderPaced                      80000000  31
#    lsopTimeBasedSeekSupported       40000000  30
#    lsopByteBasedSeekSupported       20000000  29
#    playcontainerSupported           10000000  28
#    s0IncreasingSupported            08000000  27
#    sNIncreasingSupported            04000000  26
#    rtspPauseSupported               02000000  25
#    streamingTransferModeSupported   01000000  24
#    interactiveTransferModeSupported 00800000  23
#    backgroundTransferModeSupported  00400000  22
#    connectionStallingSupported      00200000  21
#    dlnaVersion15Supported           00100000  20
DLNA_ORG_FLAGS = 'DLNA.ORG_FLAGS'
DLNA_ORG_FLAGS_VAL = '01500000000000000000000000000000'
*/

/*----------------------------------------------------------------------
|   NPT_Console::Output
+---------------------------------------------------------------------*/
void NPT_Console::Output(const char* msg)
{
}

spdlog::level::level_enum ConvertLogLevel(int nptLogLevel)
{
  if (nptLogLevel >= NPT_LOG_LEVEL_FATAL)
    return spdlog::level::critical;
  if (nptLogLevel >= NPT_LOG_LEVEL_SEVERE)
    return spdlog::level::err;
  if (nptLogLevel >= NPT_LOG_LEVEL_WARNING)
    return spdlog::level::warn;
  if (nptLogLevel >= NPT_LOG_LEVEL_FINE)
    return spdlog::level::info;
  if (nptLogLevel >= NPT_LOG_LEVEL_FINER)
    return spdlog::level::debug;

  return spdlog::level::trace;
}

void UPnPLogger(const NPT_LogRecord* record)
{
  static Logger logger = CServiceBroker::GetLogging().GetLogger("Platinum");
  if (CServiceBroker::GetLogging().CanLogComponent(LOGUPNP))
    logger->log(ConvertLogLevel(record->m_Level), "[{}]: {}", record->m_LoggerName,
                record->m_Message);
}

namespace UPNP
{

/*----------------------------------------------------------------------
|   static
+---------------------------------------------------------------------*/
CUPnP* CUPnP::upnp = NULL;
static NPT_List<void*> g_UserData;
static NPT_Mutex g_UserDataLock;

/*----------------------------------------------------------------------
|   CDeviceHostReferenceHolder class
+---------------------------------------------------------------------*/
class CDeviceHostReferenceHolder
{
public:
  PLT_DeviceHostReference m_Device;
};

/*----------------------------------------------------------------------
|   CCtrlPointReferenceHolder class
+---------------------------------------------------------------------*/
class CCtrlPointReferenceHolder
{
public:
  PLT_CtrlPointReference m_CtrlPoint;
};

/*----------------------------------------------------------------------
|   CUPnPCleaner class
+---------------------------------------------------------------------*/
class CUPnPCleaner : public NPT_Thread
{
public:
  explicit CUPnPCleaner(CUPnP* upnp) : NPT_Thread(true), m_UPnP(upnp) {}
  void Run() override { delete m_UPnP; }

  CUPnP* m_UPnP;
};

/*----------------------------------------------------------------------
|   CMediaBrowser class
+---------------------------------------------------------------------*/
class CMediaBrowser : public PLT_SyncMediaBrowser, public PLT_MediaContainerChangesListener
{
public:
  explicit CMediaBrowser(PLT_CtrlPointReference& ctrlPoint)
    : PLT_SyncMediaBrowser(ctrlPoint, true),
      m_logger(CServiceBroker::GetLogging().GetLogger("UPNP::CMediaBrowser"))
  {
    SetContainerListener(this);
  }

  // PLT_MediaBrowser methods
  bool OnMSAdded(PLT_DeviceDataReference& device) override
  {
    CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH);
    message.SetStringParam("upnp://");
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(message);

    return PLT_SyncMediaBrowser::OnMSAdded(device);
  }
  void OnMSRemoved(PLT_DeviceDataReference& device) override
  {
    PLT_SyncMediaBrowser::OnMSRemoved(device);

    CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH);
    message.SetStringParam("upnp://");
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(message);

    PLT_SyncMediaBrowser::OnMSRemoved(device);
  }

  // PLT_MediaContainerChangesListener methods
  void OnContainerChanged(PLT_DeviceDataReference& device,
                          const char* item_id,
                          const char* update_id) override
  {
    NPT_String path = "upnp://" + device->GetUUID() + "/";
    if (!NPT_StringsEqual(item_id, "0"))
    {
      std::string id(CURL::Encode(item_id));
      URIUtils::AddSlashAtEnd(id);
      path += id.c_str();
    }

    m_logger->debug("notified container update {}", (const char*)path);
    CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH);
    message.SetStringParam(path.GetChars());
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(message);
  }

  bool MarkWatched(const CFileItem& item, const bool watched)
  {
    if (watched)
    {
      CFileItem temp(item);
      temp.SetProperty("original_listitem_url", item.GetPath());
      return SaveFileState(temp, CBookmark(), watched);
    }
    else
    {
      m_logger->debug("Marking video item {} as watched", item.GetPath());

      std::set<std::pair<NPT_String, NPT_String>> values;
      values.insert(std::make_pair("<upnp:playCount>1</upnp:playCount>",
                                   "<upnp:playCount>0</upnp:playCount>"));
      return InvokeUpdateObject(item.GetPath().c_str(), values);
    }
  }

  bool SaveFileState(const CFileItem& item, const CBookmark& bookmark, const bool updatePlayCount)
  {
    std::string path = item.GetProperty("original_listitem_url").asString();
    if (!item.HasVideoInfoTag() || path.empty())
    {
      return false;
    }

    std::set<std::pair<NPT_String, NPT_String>> values;
    if (item.GetVideoInfoTag()->GetResumePoint().timeInSeconds != bookmark.timeInSeconds)
    {
      m_logger->debug("Updating resume point for item {}", path);
      long time = (long)bookmark.timeInSeconds;
      if (time < 0)
        time = 0;

      values.insert(std::make_pair(
          NPT_String::Format("<upnp:lastPlaybackPosition>%ld</upnp:lastPlaybackPosition>",
                             (long)item.GetVideoInfoTag()->GetResumePoint().timeInSeconds),
          NPT_String::Format("<upnp:lastPlaybackPosition>%ld</upnp:lastPlaybackPosition>", time)));

      NPT_String curr_value = "<xbmc:lastPlayerState>";
      PLT_Didl::AppendXmlEscape(curr_value,
                                item.GetVideoInfoTag()->GetResumePoint().playerState.c_str());
      curr_value += "</xbmc:lastPlayerState>";
      NPT_String new_value = "<xbmc:lastPlayerState>";
      PLT_Didl::AppendXmlEscape(new_value, bookmark.playerState.c_str());
      new_value += "</xbmc:lastPlayerState>";
      values.insert(std::make_pair(curr_value, new_value));
    }
    if (updatePlayCount)
    {
      m_logger->debug("Marking video item {} as watched", path);
      values.insert(std::make_pair("<upnp:playCount>0</upnp:playCount>",
                                   "<upnp:playCount>1</upnp:playCount>"));
    }

    return InvokeUpdateObject(path.c_str(), values);
  }

  bool UpdateItem(const std::string& path, const CFileItem& item)
  {
    if (path.empty())
      return false;

    std::set<std::pair<NPT_String, NPT_String>> values;
    if (item.HasVideoInfoTag())
    {
      // handle playcount
      const CVideoInfoTag* details = item.GetVideoInfoTag();
      int playcountOld = 0, playcountNew = 0;
      if (details->GetPlayCount() <= 0)
        playcountOld = 1;
      else
        playcountNew = details->GetPlayCount();

      values.insert(
          std::make_pair(NPT_String::Format("<upnp:playCount>%d</upnp:playCount>", playcountOld),
                         NPT_String::Format("<upnp:playCount>%d</upnp:playCount>", playcountNew)));

      // handle lastplayed
      CDateTime lastPlayedOld, lastPlayedNew;
      if (!details->m_lastPlayed.IsValid())
        lastPlayedOld = CDateTime::GetCurrentDateTime();
      else
        lastPlayedNew = details->m_lastPlayed;

      values.insert(
          std::make_pair(NPT_String::Format("<upnp:lastPlaybackTime>%s</upnp:lastPlaybackTime>",
                                            lastPlayedOld.GetAsW3CDateTime().c_str()),
                         NPT_String::Format("<upnp:lastPlaybackTime>%s</upnp:lastPlaybackTime>",
                                            lastPlayedNew.GetAsW3CDateTime().c_str())));

      // handle resume point
      long resumePointOld = 0L, resumePointNew = 0L;
      if (details->GetResumePoint().timeInSeconds <= 0)
        resumePointOld = 1;
      else
        resumePointNew = static_cast<long>(details->GetResumePoint().timeInSeconds);

      values.insert(std::make_pair(
          NPT_String::Format("<upnp:lastPlaybackPosition>%ld</upnp:lastPlaybackPosition>",
                             resumePointOld),
          NPT_String::Format("<upnp:lastPlaybackPosition>%ld</upnp:lastPlaybackPosition>",
                             resumePointNew)));
    }

    return InvokeUpdateObject(path.c_str(), values);
  }

  bool InvokeUpdateObject(const char* id, const std::set<std::pair<NPT_String, NPT_String>>& values)
  {
    CURL url(id);
    PLT_DeviceDataReference device;
    PLT_Service* cds;
    PLT_ActionReference action;
    NPT_String curr_value, new_value;

    m_logger->debug("attempting to invoke UpdateObject for {}", id);

    // check this server supports UpdateObject action
    NPT_CHECK_LABEL(FindServer(url.GetHostName().c_str(), device), failed);
    NPT_CHECK_LABEL(device->FindServiceById("urn:upnp-org:serviceId:ContentDirectory", cds),
                    failed);

    NPT_CHECK_LABEL(m_CtrlPoint->CreateAction(device,
                                              "urn:schemas-upnp-org:service:ContentDirectory:1",
                                              "UpdateObject", action),
                    failed);

    NPT_CHECK_LABEL(action->SetArgumentValue("ObjectID", url.GetFileName().c_str()), failed);

    // put together the current and the new value string
    for (std::set<std::pair<NPT_String, NPT_String>>::const_iterator value = values.begin();
         value != values.end(); ++value)
    {
      if (!curr_value.IsEmpty())
        curr_value.Append(",");
      if (!new_value.IsEmpty())
        new_value.Append(",");

      curr_value.Append(value->first);
      new_value.Append(value->second);
    }
    NPT_CHECK_LABEL(action->SetArgumentValue("CurrentTagValue", curr_value), failed);
    NPT_CHECK_LABEL(action->SetArgumentValue("NewTagValue", new_value), failed);

    NPT_CHECK_LABEL(m_CtrlPoint->InvokeAction(action, NULL), failed);

    m_logger->debug("invoked UpdateObject successfully");
    return true;

  failed:
    m_logger->info("invoking UpdateObject failed");
    return false;
  }

private:
  Logger m_logger;
};

/*----------------------------------------------------------------------
|   CMediaController class
+---------------------------------------------------------------------*/
class CMediaController : public PLT_MediaControllerDelegate, public PLT_MediaController
{
public:
  explicit CMediaController(PLT_CtrlPointReference& ctrl_point) : PLT_MediaController(ctrl_point)
  {
    PLT_MediaController::SetDelegate(this);
  }

  ~CMediaController() override
  {
    for (const auto& itRenderer : m_registeredRenderers)
      unregisterRenderer(itRenderer);
    m_registeredRenderers.clear();
  }

#define CHECK_USERDATA_RETURN(userdata) \
  do \
  { \
    if (!g_UserData.Contains(userdata)) \
      return; \
  } while (0)

  void OnStopResult(NPT_Result res, PLT_DeviceDataReference& device, void* userdata) override
  {
    NPT_AutoLock lock(g_UserDataLock);
    CHECK_USERDATA_RETURN(userdata);
    static_cast<PLT_MediaControllerDelegate*>(userdata)->OnStopResult(res, device, userdata);
  }

  void OnSetPlayModeResult(NPT_Result res, PLT_DeviceDataReference& device, void* userdata) override
  {
    NPT_AutoLock lock(g_UserDataLock);
    CHECK_USERDATA_RETURN(userdata);
    static_cast<PLT_MediaControllerDelegate*>(userdata)->OnSetPlayModeResult(res, device, userdata);
  }

  void OnSetAVTransportURIResult(NPT_Result res,
                                 PLT_DeviceDataReference& device,
                                 void* userdata) override
  {
    NPT_AutoLock lock(g_UserDataLock);
    CHECK_USERDATA_RETURN(userdata);
    static_cast<PLT_MediaControllerDelegate*>(userdata)->OnSetAVTransportURIResult(res, device,
                                                                                   userdata);
  }

  void OnSeekResult(NPT_Result res, PLT_DeviceDataReference& device, void* userdata) override
  {
    NPT_AutoLock lock(g_UserDataLock);
    CHECK_USERDATA_RETURN(userdata);
    static_cast<PLT_MediaControllerDelegate*>(userdata)->OnSeekResult(res, device, userdata);
  }

  void OnPreviousResult(NPT_Result res, PLT_DeviceDataReference& device, void* userdata) override
  {
    NPT_AutoLock lock(g_UserDataLock);
    CHECK_USERDATA_RETURN(userdata);
    static_cast<PLT_MediaControllerDelegate*>(userdata)->OnPreviousResult(res, device, userdata);
  }

  void OnPlayResult(NPT_Result res, PLT_DeviceDataReference& device, void* userdata) override
  {
    NPT_AutoLock lock(g_UserDataLock);
    CHECK_USERDATA_RETURN(userdata);
    static_cast<PLT_MediaControllerDelegate*>(userdata)->OnPlayResult(res, device, userdata);
  }

  void OnPauseResult(NPT_Result res, PLT_DeviceDataReference& device, void* userdata) override
  {
    NPT_AutoLock lock(g_UserDataLock);
    CHECK_USERDATA_RETURN(userdata);
    static_cast<PLT_MediaControllerDelegate*>(userdata)->OnPauseResult(res, device, userdata);
  }

  void OnNextResult(NPT_Result res, PLT_DeviceDataReference& device, void* userdata) override
  {
    NPT_AutoLock lock(g_UserDataLock);
    CHECK_USERDATA_RETURN(userdata);
    static_cast<PLT_MediaControllerDelegate*>(userdata)->OnNextResult(res, device, userdata);
  }

  void OnGetMediaInfoResult(NPT_Result res,
                            PLT_DeviceDataReference& device,
                            PLT_MediaInfo* info,
                            void* userdata) override
  {
    NPT_AutoLock lock(g_UserDataLock);
    CHECK_USERDATA_RETURN(userdata);
    static_cast<PLT_MediaControllerDelegate*>(userdata)->OnGetMediaInfoResult(res, device, info,
                                                                              userdata);
  }

  void OnGetPositionInfoResult(NPT_Result res,
                               PLT_DeviceDataReference& device,
                               PLT_PositionInfo* info,
                               void* userdata) override
  {
    NPT_AutoLock lock(g_UserDataLock);
    CHECK_USERDATA_RETURN(userdata);
    static_cast<PLT_MediaControllerDelegate*>(userdata)->OnGetPositionInfoResult(res, device, info,
                                                                                 userdata);
  }

  void OnGetTransportInfoResult(NPT_Result res,
                                PLT_DeviceDataReference& device,
                                PLT_TransportInfo* info,
                                void* userdata) override
  {
    NPT_AutoLock lock(g_UserDataLock);
    CHECK_USERDATA_RETURN(userdata);
    static_cast<PLT_MediaControllerDelegate*>(userdata)->OnGetTransportInfoResult(res, device, info,
                                                                                  userdata);
  }

  bool OnMRAdded(PLT_DeviceDataReference& device) override
  {
    if (device->GetUUID().IsEmpty() || device->GetUUID().GetChars() == NULL)
      return false;

    CPlayerCoreFactory& playerCoreFactory = CServiceBroker::GetPlayerCoreFactory();

    playerCoreFactory.OnPlayerDiscovered((const char*)device->GetUUID(),
                                         (const char*)device->GetFriendlyName());

    m_registeredRenderers.insert(std::string(device->GetUUID().GetChars()));
    return true;
  }

  void OnMRRemoved(PLT_DeviceDataReference& device) override
  {
    if (device->GetUUID().IsEmpty() || device->GetUUID().GetChars() == NULL)
      return;

    std::string uuid(device->GetUUID().GetChars());
    unregisterRenderer(uuid);
    m_registeredRenderers.erase(uuid);
  }

private:
  void unregisterRenderer(const std::string& deviceUUID)
  {
    CPlayerCoreFactory& playerCoreFactory = CServiceBroker::GetPlayerCoreFactory();

    playerCoreFactory.OnPlayerRemoved(deviceUUID);
  }

  std::set<std::string> m_registeredRenderers;
};

/*----------------------------------------------------------------------
|   CUPnP::CUPnP
+---------------------------------------------------------------------*/
CUPnP::CUPnP()
  : m_MediaBrowser(NULL),
    m_MediaController(NULL),
    m_LogHandler(NULL),
    m_ServerHolder(new CDeviceHostReferenceHolder()),
    m_RendererHolder(new CRendererReferenceHolder()),
    m_CtrlPointHolder(new CCtrlPointReferenceHolder())
{
  NPT_LogManager::GetDefault().Configure("plist:.level=FINE;.handlers=CustomHandler;");
  NPT_LogHandler::Create("xbmc", "CustomHandler", m_LogHandler);
  m_LogHandler->SetCustomHandlerFunction(&UPnPLogger);

  // initialize upnp context
  m_UPnP = new PLT_UPnP();

  // keep main IP around
  if (CServiceBroker::GetNetwork().GetFirstConnectedInterface())
  {
    m_IP = CServiceBroker::GetNetwork().GetFirstConnectedInterface()->GetCurrentIPAddress().c_str();
  }
  NPT_List<NPT_IpAddress> list;
  if (NPT_SUCCEEDED(PLT_UPnPMessageHelper::GetIPAddresses(list)) && list.GetItemCount())
  {
    m_IP = (*(list.GetFirstItem())).ToString();
  }
  else if (m_IP.empty())
    m_IP = "localhost";

  // start upnp monitoring
  m_UPnP->Start();
}

/*----------------------------------------------------------------------
|   CUPnP::~CUPnP
+---------------------------------------------------------------------*/
CUPnP::~CUPnP()
{
  m_UPnP->Stop();
  StopClient();
  StopController();
  StopServer();

  delete m_UPnP;
  delete m_LogHandler;
  delete m_ServerHolder;
  delete m_RendererHolder;
  delete m_CtrlPointHolder;
}

/*----------------------------------------------------------------------
|   CUPnP::GetInstance
+---------------------------------------------------------------------*/
CUPnP* CUPnP::GetInstance()
{
  if (!upnp)
  {
    upnp = new CUPnP();
  }

  return upnp;
}

/*----------------------------------------------------------------------
|   CUPnP::ReleaseInstance
+---------------------------------------------------------------------*/
void CUPnP::ReleaseInstance(bool bWait)
{
  if (upnp)
  {
    CUPnP* _upnp = upnp;
    upnp = NULL;

    if (bWait)
    {
      delete _upnp;
    }
    else
    {
      // since it takes a while to clean up
      // starts a detached thread to do this
      CUPnPCleaner* cleaner = new CUPnPCleaner(_upnp);
      cleaner->Start();
    }
  }
}

/*----------------------------------------------------------------------
|   CUPnP::GetServer
+---------------------------------------------------------------------*/
CUPnPServer* CUPnP::GetServer()
{
  if (upnp)
    return static_cast<CUPnPServer*>(upnp->m_ServerHolder->m_Device.AsPointer());
  return NULL;
}

/*----------------------------------------------------------------------
|   CUPnP::MarkWatched
+---------------------------------------------------------------------*/
bool CUPnP::MarkWatched(const CFileItem& item, const bool watched)
{
  if (upnp && upnp->m_MediaBrowser)
  {
    // dynamic_cast is safe here, avoids polluting CUPnP.h header file
    CMediaBrowser* browser = dynamic_cast<CMediaBrowser*>(upnp->m_MediaBrowser);
    if (browser)
      return browser->MarkWatched(item, watched);
  }
  return false;
}

/*----------------------------------------------------------------------
|   CUPnP::SaveFileState
+---------------------------------------------------------------------*/
bool CUPnP::SaveFileState(const CFileItem& item,
                          const CBookmark& bookmark,
                          const bool updatePlayCount)
{
  if (upnp && upnp->m_MediaBrowser)
  {
    // dynamic_cast is safe here, avoids polluting CUPnP.h header file
    CMediaBrowser* browser = dynamic_cast<CMediaBrowser*>(upnp->m_MediaBrowser);
    if (browser)
      return browser->SaveFileState(item, bookmark, updatePlayCount);
  }
  return false;
}

/*----------------------------------------------------------------------
|   CUPnP::CreateControlPoint
+---------------------------------------------------------------------*/
void CUPnP::CreateControlPoint()
{
  if (!m_CtrlPointHolder->m_CtrlPoint.IsNull())
    return;

  // create controlpoint
  m_CtrlPointHolder->m_CtrlPoint = new PLT_CtrlPoint();

  // start it
  m_UPnP->AddCtrlPoint(m_CtrlPointHolder->m_CtrlPoint);
}

/*----------------------------------------------------------------------
|   CUPnP::DestroyControlPoint
+---------------------------------------------------------------------*/
void CUPnP::DestroyControlPoint()
{
  if (m_CtrlPointHolder->m_CtrlPoint.IsNull())
    return;

  m_UPnP->RemoveCtrlPoint(m_CtrlPointHolder->m_CtrlPoint);
  m_CtrlPointHolder->m_CtrlPoint = NULL;
}

/*----------------------------------------------------------------------
|   CUPnP::UpdateItem
+---------------------------------------------------------------------*/
bool CUPnP::UpdateItem(const std::string& path, const CFileItem& item)
{
  if (upnp && upnp->m_MediaBrowser)
  {
    // dynamic_cast is safe here, avoids polluting CUPnP.h header file
    CMediaBrowser* browser = dynamic_cast<CMediaBrowser*>(upnp->m_MediaBrowser);
    if (browser)
      return browser->UpdateItem(path, item);
  }
  return false;
}

/*----------------------------------------------------------------------
|   CUPnP::StartClient
+---------------------------------------------------------------------*/
void CUPnP::StartClient()
{
  std::unique_lock<CCriticalSection> lock(m_lockMediaBrowser);
  if (m_MediaBrowser != NULL)
    return;

  CreateControlPoint();

  // start browser
  m_MediaBrowser = new CMediaBrowser(m_CtrlPointHolder->m_CtrlPoint);
}

/*----------------------------------------------------------------------
|   CUPnP::StopClient
+---------------------------------------------------------------------*/
void CUPnP::StopClient()
{
  std::unique_lock<CCriticalSection> lock(m_lockMediaBrowser);
  if (m_MediaBrowser == NULL)
    return;

  delete m_MediaBrowser;
  m_MediaBrowser = NULL;

  if (!IsControllerStarted())
    DestroyControlPoint();
}

/*----------------------------------------------------------------------
|   CUPnP::StartController
+---------------------------------------------------------------------*/
void CUPnP::StartController()
{
  if (m_MediaController != NULL)
    return;

  CreateControlPoint();

  m_MediaController = new CMediaController(m_CtrlPointHolder->m_CtrlPoint);
}

/*----------------------------------------------------------------------
|   CUPnP::StopController
+---------------------------------------------------------------------*/
void CUPnP::StopController()
{
  if (m_MediaController == NULL)
    return;

  delete m_MediaController;
  m_MediaController = NULL;

  if (!IsClientStarted())
    DestroyControlPoint();
}

/*----------------------------------------------------------------------
|   CUPnP::CreateServer
+---------------------------------------------------------------------*/
CUPnPServer* CUPnP::CreateServer(int port /* = 0 */)
{
  CUPnPServer* device = new CUPnPServer(CSysInfo::GetDeviceName().c_str(),
                                        CUPnPSettings::GetInstance().GetServerUUID().length()
                                            ? CUPnPSettings::GetInstance().GetServerUUID().c_str()
                                            : NULL,
                                        port);

  // trying to set optional upnp values for XP UPnP UI Icons to detect us
  // but it doesn't work anyways as it requires multicast for XP to detect us
  device->m_PresentationURL =
      NPT_HttpUrl(m_IP.c_str(),
                  CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
                      CSettings::SETTING_SERVICES_WEBSERVERPORT),
                  "/")
          .ToString();

  device->m_ModelName = "Kodi";
  device->m_ModelNumber = CSysInfo::GetVersion().c_str();
  device->m_ModelDescription = "Kodi - Media Server";
  device->m_ModelURL = "http://kodi.tv/";
  device->m_Manufacturer = "XBMC Foundation";
  device->m_ManufacturerURL = "http://kodi.tv/";

  device->SetDelegate(device);
  return device;
}

/*----------------------------------------------------------------------
|   CUPnP::StartServer
+---------------------------------------------------------------------*/
bool CUPnP::StartServer()
{
  if (!m_ServerHolder->m_Device.IsNull())
    return false;

  const std::shared_ptr<CProfileManager> profileManager =
      CServiceBroker::GetSettingsComponent()->GetProfileManager();

  // load upnpserver.xml
  std::string filename =
      URIUtils::AddFileToFolder(profileManager->GetUserDataFolder(), "upnpserver.xml");
  CUPnPSettings::GetInstance().Load(filename);

  // create the server with a XBox compatible friendlyname and UUID from upnpserver.xml if found
  m_ServerHolder->m_Device = CreateServer(CUPnPSettings::GetInstance().GetServerPort());

  // start server
  NPT_Result res = m_UPnP->AddDevice(m_ServerHolder->m_Device);
  if (NPT_FAILED(res))
  {
    // if the upnp device port was not 0, it could have failed because
    // of port being in used, so restart with a random port
    if (CUPnPSettings::GetInstance().GetServerPort() > 0)
      m_ServerHolder->m_Device = CreateServer(0);

    res = m_UPnP->AddDevice(m_ServerHolder->m_Device);
  }

  // save port but don't overwrite saved settings if port was random
  if (NPT_SUCCEEDED(res))
  {
    if (CUPnPSettings::GetInstance().GetServerPort() == 0)
    {
      CUPnPSettings::GetInstance().SetServerPort(m_ServerHolder->m_Device->GetPort());
    }
    CUPnPServer::m_MaxReturnedItems = UPNP_DEFAULT_MAX_RETURNED_ITEMS;
    if (CUPnPSettings::GetInstance().GetMaximumReturnedItems() > 0)
    {
      // must be > UPNP_DEFAULT_MIN_RETURNED_ITEMS
      CUPnPServer::m_MaxReturnedItems = std::max(
          UPNP_DEFAULT_MIN_RETURNED_ITEMS, CUPnPSettings::GetInstance().GetMaximumReturnedItems());
    }
    CUPnPSettings::GetInstance().SetMaximumReturnedItems(CUPnPServer::m_MaxReturnedItems);
  }

  // save UUID
  CUPnPSettings::GetInstance().SetServerUUID(m_ServerHolder->m_Device->GetUUID().GetChars());
  return CUPnPSettings::GetInstance().Save(filename);
}

/*----------------------------------------------------------------------
|   CUPnP::StopServer
+---------------------------------------------------------------------*/
void CUPnP::StopServer()
{
  if (m_ServerHolder->m_Device.IsNull())
    return;

  m_UPnP->RemoveDevice(m_ServerHolder->m_Device);
  m_ServerHolder->m_Device = NULL;
}

/*----------------------------------------------------------------------
|   CUPnP::CreateRenderer
+---------------------------------------------------------------------*/
CUPnPRenderer* CUPnP::CreateRenderer(int port /* = 0 */)
{
  CUPnPRenderer* device =
      new CUPnPRenderer(CSysInfo::GetDeviceName().c_str(), false,
                        (CUPnPSettings::GetInstance().GetRendererUUID().length()
                             ? CUPnPSettings::GetInstance().GetRendererUUID().c_str()
                             : NULL),
                        port);

  device->m_PresentationURL =
      NPT_HttpUrl(m_IP.c_str(),
                  CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
                      CSettings::SETTING_SERVICES_WEBSERVERPORT),
                  "/")
          .ToString();
  device->m_ModelName = "Kodi";
  device->m_ModelNumber = CSysInfo::GetVersion().c_str();
  device->m_ModelDescription = "Kodi - Media Renderer";
  device->m_ModelURL = "http://kodi.tv/";
  device->m_Manufacturer = "XBMC Foundation";
  device->m_ManufacturerURL = "http://kodi.tv/";

  return device;
}

/*----------------------------------------------------------------------
|   CUPnP::StartRenderer
+---------------------------------------------------------------------*/
bool CUPnP::StartRenderer()
{
  if (!m_RendererHolder->m_Device.IsNull())
    return false;

  const std::shared_ptr<CProfileManager> profileManager =
      CServiceBroker::GetSettingsComponent()->GetProfileManager();

  std::string filename =
      URIUtils::AddFileToFolder(profileManager->GetUserDataFolder(), "upnpserver.xml");
  CUPnPSettings::GetInstance().Load(filename);

  m_RendererHolder->m_Device = CreateRenderer(CUPnPSettings::GetInstance().GetRendererPort());

  NPT_Result res = m_UPnP->AddDevice(m_RendererHolder->m_Device);

  // failed most likely because port is in use, try again with random port now
  if (NPT_FAILED(res) && CUPnPSettings::GetInstance().GetRendererPort() != 0)
  {
    m_RendererHolder->m_Device = CreateRenderer(0);

    res = m_UPnP->AddDevice(m_RendererHolder->m_Device);
  }

  // save port but don't overwrite saved settings if random
  if (NPT_SUCCEEDED(res) && CUPnPSettings::GetInstance().GetRendererPort() == 0)
  {
    CUPnPSettings::GetInstance().SetRendererPort(m_RendererHolder->m_Device->GetPort());
  }

  // save UUID
  CUPnPSettings::GetInstance().SetRendererUUID(m_RendererHolder->m_Device->GetUUID().GetChars());
  return CUPnPSettings::GetInstance().Save(filename);
}

/*----------------------------------------------------------------------
|   CUPnP::StopRenderer
+---------------------------------------------------------------------*/
void CUPnP::StopRenderer()
{
  if (m_RendererHolder->m_Device.IsNull())
    return;

  m_UPnP->RemoveDevice(m_RendererHolder->m_Device);
  m_RendererHolder->m_Device = NULL;
}

/*----------------------------------------------------------------------
|   CUPnP::UpdateState
+---------------------------------------------------------------------*/
void CUPnP::UpdateState()
{
  if (!m_RendererHolder->m_Device.IsNull())
    static_cast<CUPnPRenderer*>(m_RendererHolder->m_Device.AsPointer())->UpdateState();
}

void CUPnP::RegisterUserdata(void* ptr)
{
  NPT_AutoLock lock(g_UserDataLock);
  g_UserData.Add(ptr);
}

void CUPnP::UnregisterUserdata(void* ptr)
{
  NPT_AutoLock lock(g_UserDataLock);
  g_UserData.Remove(ptr);
}

} /* namespace UPNP */
