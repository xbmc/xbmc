/*
 * UPnP Support for XBMC
 *      Copyright (c) 2006 c0diq (Sylvain Rebaud)
 *      Portions Copyright (c) by the authors of libPlatinum
 *      http://www.plutinosoft.com/blog/category/platinum/
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

#include <set>
#include <Platinum/Source/Platinum/Platinum.h>

#include "threads/SystemClock.h"
#include "UPnP.h"
#include "UPnPInternal.h"
#include "UPnPRenderer.h"
#include "UPnPServer.h"
#include "UPnPSettings.h"
#include "utils/URIUtils.h"
#include "Application.h"
#include "messaging/ApplicationMessenger.h"
#include "network/Network.h"
#include "utils/log.h"
#include "URL.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "profiles/ProfilesManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "GUIUserMessages.h"
#include "FileItem.h"
#include "guilib/GUIWindowManager.h"
#include "utils/TimeUtils.h"
#include "video/VideoInfoTag.h"
#include "input/Key.h"
#include "Util.h"
#include "utils/SystemInfo.h"

using namespace UPNP;
using namespace KODI::MESSAGING;

NPT_SET_LOCAL_LOGGER("xbmc.upnp")

#define UPNP_DEFAULT_MAX_RETURNED_ITEMS 200
#define UPNP_DEFAULT_MIN_RETURNED_ITEMS 30

/*
# Play speed
#    1 normal
#    0 invalid
DLNA_ORG_PS = 'DLNA.ORG_PS'
DLNA_ORG_PS_VAL = '1'

# Convertion Indicator
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
void
NPT_Console::Output(const char* msg) { }

int ConvertLogLevel(int nptLogLevel)
{
    if (nptLogLevel >= NPT_LOG_LEVEL_FATAL)
        return LOGFATAL;
    if (nptLogLevel >= NPT_LOG_LEVEL_SEVERE)
        return LOGERROR;
    if (nptLogLevel >= NPT_LOG_LEVEL_WARNING)
        return LOGWARNING;
    if (nptLogLevel >= NPT_LOG_LEVEL_INFO)
        return LOGNOTICE;
    if (nptLogLevel >= NPT_LOG_LEVEL_FINE)
        return LOGINFO;

    return LOGDEBUG;
}

void
UPnPLogger(const NPT_LogRecord* record)
{
    if (!g_advancedSettings.CanLogComponent(LOGUPNP))
        return;

    CLog::Log(ConvertLogLevel(record->m_Level), "Platinum [%s]: %s", record->m_LoggerName, record->m_Message);
}

namespace UPNP
{

/*----------------------------------------------------------------------
|   static
+---------------------------------------------------------------------*/
CUPnP* CUPnP::upnp = NULL;
static NPT_List<void*> g_UserData;
static NPT_Mutex       g_UserDataLock;

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
    CUPnPCleaner(CUPnP* upnp) : NPT_Thread(true), m_UPnP(upnp) {}
    void Run() {
        delete m_UPnP;
    }

    CUPnP* m_UPnP;
};

/*----------------------------------------------------------------------
|   CMediaBrowser class
+---------------------------------------------------------------------*/
class CMediaBrowser : public PLT_SyncMediaBrowser,
                      public PLT_MediaContainerChangesListener
{
public:
    CMediaBrowser(PLT_CtrlPointReference& ctrlPoint)
        : PLT_SyncMediaBrowser(ctrlPoint, true)
    {
        SetContainerListener(this);
    }

    // PLT_MediaBrowser methods
    virtual bool OnMSAdded(PLT_DeviceDataReference& device)
    {
        CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH);
        message.SetStringParam("upnp://");
        g_windowManager.SendThreadMessage(message);

        return PLT_SyncMediaBrowser::OnMSAdded(device);
    }
    virtual void OnMSRemoved(PLT_DeviceDataReference& device)
    {
        PLT_SyncMediaBrowser::OnMSRemoved(device);

        CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH);
        message.SetStringParam("upnp://");
        g_windowManager.SendThreadMessage(message);

        PLT_SyncMediaBrowser::OnMSRemoved(device);
    }

    // PLT_MediaContainerChangesListener methods
    virtual void OnContainerChanged(PLT_DeviceDataReference& device,
                                    const char*              item_id,
                                    const char*              update_id)
    {
        NPT_String path = "upnp://"+device->GetUUID()+"/";
        if (!NPT_StringsEqual(item_id, "0")) {
            std::string id(CURL::Encode(item_id));
            URIUtils::AddSlashAtEnd(id);
            path += id.c_str();
        }

        CLog::Log(LOGDEBUG, "UPNP: notfified container update %s", (const char*)path);
        CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH);
        message.SetStringParam(path.GetChars());
        g_windowManager.SendThreadMessage(message);
    }

    bool MarkWatched(const CFileItem& item, const bool watched)
    {
        if (watched) {
            CFileItem temp(item);
            temp.SetProperty("original_listitem_url", item.GetPath());
            return SaveFileState(temp, CBookmark(), watched);
        }
        else {
            CLog::Log(LOGDEBUG, "UPNP: Marking video item %s as watched", item.GetPath().c_str());
            return InvokeUpdateObject(item.GetPath().c_str(), "<upnp:playCount>1</upnp:playCount>", "<upnp:playCount>0</upnp:playCount>");
        }
    }

    bool SaveFileState(const CFileItem& item, const CBookmark& bookmark, const bool updatePlayCount)
    {
        std::string path = item.GetProperty("original_listitem_url").asString();
        if (!item.HasVideoInfoTag() || path.empty())  {
          return false;
        }

        NPT_String curr_value;
        NPT_String new_value;

        if (item.GetVideoInfoTag()->m_resumePoint.timeInSeconds != bookmark.timeInSeconds) {
            CLog::Log(LOGDEBUG, "UPNP: Updating resume point for item %s", path.c_str());
            long time = (long)bookmark.timeInSeconds;
            if (time < 0) time = 0;
            curr_value.Append(NPT_String::Format("<upnp:lastPlaybackPosition>%ld</upnp:lastPlaybackPosition>",
                                                 (long)item.GetVideoInfoTag()->m_resumePoint.timeInSeconds));
            new_value.Append(NPT_String::Format("<upnp:lastPlaybackPosition>%ld</upnp:lastPlaybackPosition>", time));
        }
        if (updatePlayCount) {
            CLog::Log(LOGDEBUG, "UPNP: Marking video item %s as watched", path.c_str());
            if (!curr_value.IsEmpty()) curr_value.Append(",");
            if (!new_value.IsEmpty()) new_value.Append(",");
            curr_value.Append("<upnp:playCount>0</upnp:playCount>");
            new_value.Append("<upnp:playCount>1</upnp:playCount>");
        }

        return InvokeUpdateObject(path.c_str(), (const char*)curr_value, (const char*)new_value);
    }

    bool InvokeUpdateObject(const char* id, const char* curr_value, const char* new_value)
    {
        CURL url(id);
        PLT_DeviceDataReference device;
        PLT_Service* cds;
        PLT_ActionReference action;

        CLog::Log(LOGDEBUG, "UPNP: attempting to invoke UpdateObject for %s", id);

        // check this server supports UpdateObject action
        NPT_CHECK_LABEL(FindServer(url.GetHostName().c_str(), device),failed);
        NPT_CHECK_LABEL(device->FindServiceById("urn:upnp-org:serviceId:ContentDirectory", cds),failed);

        NPT_CHECK_LABEL(m_CtrlPoint->CreateAction(
            device,
            "urn:schemas-upnp-org:service:ContentDirectory:1",
            "UpdateObject",
            action), failed);

        NPT_CHECK_LABEL(action->SetArgumentValue("ObjectID", url.GetFileName().c_str()), failed);
        NPT_CHECK_LABEL(action->SetArgumentValue("CurrentTagValue", curr_value), failed);
        NPT_CHECK_LABEL(action->SetArgumentValue("NewTagValue", new_value), failed);

        NPT_CHECK_LABEL(m_CtrlPoint->InvokeAction(action, NULL),failed);

        CLog::Log(LOGDEBUG, "UPNP: invoked UpdateObject successfully");
        return true;

    failed:
        CLog::Log(LOGINFO, "UPNP: invoking UpdateObject failed");
        return false;
    }
};


/*----------------------------------------------------------------------
|   CMediaController class
+---------------------------------------------------------------------*/
class CMediaController
  : public PLT_MediaControllerDelegate
  , public PLT_MediaController
{
public:
  CMediaController(PLT_CtrlPointReference& ctrl_point)
    : PLT_MediaController(ctrl_point)
  {
    PLT_MediaController::SetDelegate(this);
  }

  ~CMediaController()
  {
    for (std::set<std::string>::const_iterator itRenderer = m_registeredRenderers.begin(); itRenderer != m_registeredRenderers.end(); ++itRenderer)
      unregisterRenderer(*itRenderer);
    m_registeredRenderers.clear();
  }

#define CHECK_USERDATA_RETURN(userdata) do {     \
  if (!g_UserData.Contains(userdata))            \
      return;                                    \
  } while(0)

  virtual void OnStopResult(NPT_Result res, PLT_DeviceDataReference& device, void* userdata)
  { CHECK_USERDATA_RETURN(userdata);
    static_cast<PLT_MediaControllerDelegate*>(userdata)->OnStopResult(res, device, userdata);
  }

  virtual void OnSetPlayModeResult(NPT_Result res, PLT_DeviceDataReference& device, void* userdata)
  { CHECK_USERDATA_RETURN(userdata);
    static_cast<PLT_MediaControllerDelegate*>(userdata)->OnSetPlayModeResult(res, device, userdata);
  }

  virtual void OnSetAVTransportURIResult(NPT_Result res, PLT_DeviceDataReference& device, void* userdata)
  { CHECK_USERDATA_RETURN(userdata);
    static_cast<PLT_MediaControllerDelegate*>(userdata)->OnSetAVTransportURIResult(res, device, userdata);
  }

  virtual void OnSeekResult(NPT_Result res, PLT_DeviceDataReference& device, void* userdata)
  { CHECK_USERDATA_RETURN(userdata);
    static_cast<PLT_MediaControllerDelegate*>(userdata)->OnSeekResult(res, device, userdata);
  }

  virtual void OnPreviousResult(NPT_Result res, PLT_DeviceDataReference& device, void* userdata)
  { CHECK_USERDATA_RETURN(userdata);
    static_cast<PLT_MediaControllerDelegate*>(userdata)->OnPreviousResult(res, device, userdata);
  }

  virtual void OnPlayResult(NPT_Result res, PLT_DeviceDataReference& device, void* userdata)
  { CHECK_USERDATA_RETURN(userdata);
    static_cast<PLT_MediaControllerDelegate*>(userdata)->OnPlayResult(res, device, userdata);
  }

  virtual void OnPauseResult(NPT_Result res, PLT_DeviceDataReference& device, void* userdata)
  { CHECK_USERDATA_RETURN(userdata);
    static_cast<PLT_MediaControllerDelegate*>(userdata)->OnPauseResult(res, device, userdata);
  }

  virtual void OnNextResult(NPT_Result res, PLT_DeviceDataReference& device, void* userdata)
  { CHECK_USERDATA_RETURN(userdata);
    static_cast<PLT_MediaControllerDelegate*>(userdata)->OnNextResult(res, device, userdata);
  }

  virtual void OnGetMediaInfoResult(NPT_Result res, PLT_DeviceDataReference& device, PLT_MediaInfo* info, void* userdata)
  { CHECK_USERDATA_RETURN(userdata);
    static_cast<PLT_MediaControllerDelegate*>(userdata)->OnGetMediaInfoResult(res, device, info, userdata);
  }

  virtual void OnGetPositionInfoResult(NPT_Result res, PLT_DeviceDataReference& device, PLT_PositionInfo* info, void* userdata)
  { CHECK_USERDATA_RETURN(userdata);
    static_cast<PLT_MediaControllerDelegate*>(userdata)->OnGetPositionInfoResult(res, device, info, userdata);
  }

  virtual void OnGetTransportInfoResult(NPT_Result res, PLT_DeviceDataReference& device, PLT_TransportInfo* info, void* userdata)
  { CHECK_USERDATA_RETURN(userdata);
    static_cast<PLT_MediaControllerDelegate*>(userdata)->OnGetTransportInfoResult(res, device, info, userdata);
  }

  virtual bool OnMRAdded(PLT_DeviceDataReference& device )
  {
    if (device->GetUUID().IsEmpty() || device->GetUUID().GetChars() == NULL)
      return false;

    CPlayerCoreFactory::GetInstance().OnPlayerDiscovered((const char*)device->GetUUID()
                                          ,(const char*)device->GetFriendlyName());
    
    m_registeredRenderers.insert(std::string(device->GetUUID().GetChars()));
    return true;
  }

  virtual void OnMRRemoved(PLT_DeviceDataReference& device )
  {
    if (device->GetUUID().IsEmpty() || device->GetUUID().GetChars() == NULL)
      return;

    std::string uuid(device->GetUUID().GetChars());
    unregisterRenderer(uuid);
    m_registeredRenderers.erase(uuid);
  }

private:
  void unregisterRenderer(const std::string &deviceUUID)
  {
    CPlayerCoreFactory::GetInstance().OnPlayerRemoved(deviceUUID);
  }

  std::set<std::string> m_registeredRenderers;
};

/*----------------------------------------------------------------------
|   CUPnP::CUPnP
+---------------------------------------------------------------------*/
CUPnP::CUPnP() :
    m_MediaBrowser(NULL),
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
    if (g_application.getNetwork().GetFirstConnectedInterface()) {
        m_IP = g_application.getNetwork().GetFirstConnectedInterface()->GetCurrentIPAddress().c_str();
    }
    NPT_List<NPT_IpAddress> list;
    if (NPT_SUCCEEDED(PLT_UPnPMessageHelper::GetIPAddresses(list)) && list.GetItemCount()) {
        m_IP = (*(list.GetFirstItem())).ToString();
    }
    else if(m_IP.empty())
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
CUPnP*
CUPnP::GetInstance()
{
    if (!upnp) {
        upnp = new CUPnP();
    }

    return upnp;
}

/*----------------------------------------------------------------------
|   CUPnP::ReleaseInstance
+---------------------------------------------------------------------*/
void
CUPnP::ReleaseInstance(bool bWait)
{
    if (upnp) {
        CUPnP* _upnp = upnp;
        upnp = NULL;

        if (bWait) {
            delete _upnp;
        } else {
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
  if(upnp)
    return (CUPnPServer*)upnp->m_ServerHolder->m_Device.AsPointer();
  return NULL;
}

/*----------------------------------------------------------------------
|   CUPnP::MarkWatched
+---------------------------------------------------------------------*/
bool
CUPnP::MarkWatched(const CFileItem& item, const bool watched)
{
    if (upnp && upnp->m_MediaBrowser) {
        // dynamic_cast is safe here, avoids polluting CUPnP.h header file
        CMediaBrowser* browser = dynamic_cast<CMediaBrowser*>(upnp->m_MediaBrowser);
        return browser->MarkWatched(item, watched);
    }
    return false;
}

/*----------------------------------------------------------------------
|   CUPnP::SaveFileState
+---------------------------------------------------------------------*/
bool
CUPnP::SaveFileState(const CFileItem& item, const CBookmark& bookmark, const bool updatePlayCount)
{
    if (upnp && upnp->m_MediaBrowser) {
        // dynamic_cast is safe here, avoids polluting CUPnP.h header file
        CMediaBrowser* browser = dynamic_cast<CMediaBrowser*>(upnp->m_MediaBrowser);
        return browser->SaveFileState(item, bookmark, updatePlayCount);
    }
    return false;
}

/*----------------------------------------------------------------------
|   CUPnP::CreateControlPoint
+---------------------------------------------------------------------*/
void
CUPnP::CreateControlPoint()
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
void
CUPnP::DestroyControlPoint()
{
    if (m_CtrlPointHolder->m_CtrlPoint.IsNull())
        return;

    m_UPnP->RemoveCtrlPoint(m_CtrlPointHolder->m_CtrlPoint);
    m_CtrlPointHolder->m_CtrlPoint = NULL;
}

/*----------------------------------------------------------------------
|   CUPnP::StartClient
+---------------------------------------------------------------------*/
void
CUPnP::StartClient()
{
    if (m_MediaBrowser != NULL)
        return;

    CreateControlPoint();

    // start browser
    m_MediaBrowser = new CMediaBrowser(m_CtrlPointHolder->m_CtrlPoint);
}

/*----------------------------------------------------------------------
|   CUPnP::StopClient
+---------------------------------------------------------------------*/
void
CUPnP::StopClient()
{
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
void
CUPnP::StartController()
{
    if (m_MediaController != NULL)
        return;

    CreateControlPoint();

    m_MediaController = new CMediaController(m_CtrlPointHolder->m_CtrlPoint);
}

/*----------------------------------------------------------------------
|   CUPnP::StopController
+---------------------------------------------------------------------*/
void
CUPnP::StopController()
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
CUPnPServer*
CUPnP::CreateServer(int port /* = 0 */)
{
    CUPnPServer* device =
        new CUPnPServer(CSysInfo::GetDeviceName().c_str(),
                        CUPnPSettings::GetInstance().GetServerUUID().length() ? CUPnPSettings::GetInstance().GetServerUUID().c_str() : NULL,
                        port);

    // trying to set optional upnp values for XP UPnP UI Icons to detect us
    // but it doesn't work anyways as it requires multicast for XP to detect us
    device->m_PresentationURL =
        NPT_HttpUrl(m_IP.c_str(),
                    CSettings::GetInstance().GetInt(CSettings::SETTING_SERVICES_WEBSERVERPORT),
                    "/").ToString();

    device->m_ModelName        = "Kodi";
    device->m_ModelNumber      = CSysInfo::GetVersion().c_str();
    device->m_ModelDescription = "Kodi - Media Server";
    device->m_ModelURL         = "http://kodi.tv/";
    device->m_Manufacturer     = "XBMC Foundation";
    device->m_ManufacturerURL  = "http://kodi.tv/";

    device->SetDelegate(device);
    return device;
}

/*----------------------------------------------------------------------
|   CUPnP::StartServer
+---------------------------------------------------------------------*/
bool
CUPnP::StartServer()
{
    if (!m_ServerHolder->m_Device.IsNull()) return false;

    // load upnpserver.xml
    std::string filename = URIUtils::AddFileToFolder(CProfilesManager::GetInstance().GetUserDataFolder(), "upnpserver.xml");
    CUPnPSettings::GetInstance().Load(filename);

    // create the server with a XBox compatible friendlyname and UUID from upnpserver.xml if found
    m_ServerHolder->m_Device = CreateServer(CUPnPSettings::GetInstance().GetServerPort());

    // start server
    NPT_Result res = m_UPnP->AddDevice(m_ServerHolder->m_Device);
    if (NPT_FAILED(res)) {
        // if the upnp device port was not 0, it could have failed because
        // of port being in used, so restart with a random port
        if (CUPnPSettings::GetInstance().GetServerPort() > 0) m_ServerHolder->m_Device = CreateServer(0);

        res = m_UPnP->AddDevice(m_ServerHolder->m_Device);
    }

    // save port but don't overwrite saved settings if port was random
    if (NPT_SUCCEEDED(res)) {
        if (CUPnPSettings::GetInstance().GetServerPort() == 0) {
            CUPnPSettings::GetInstance().SetServerPort(m_ServerHolder->m_Device->GetPort());
        }
        CUPnPServer::m_MaxReturnedItems = UPNP_DEFAULT_MAX_RETURNED_ITEMS;
        if (CUPnPSettings::GetInstance().GetMaximumReturnedItems() > 0) {
            // must be > UPNP_DEFAULT_MIN_RETURNED_ITEMS
            CUPnPServer::m_MaxReturnedItems = std::max(UPNP_DEFAULT_MIN_RETURNED_ITEMS, CUPnPSettings::GetInstance().GetMaximumReturnedItems());
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
void
CUPnP::StopServer()
{
    if (m_ServerHolder->m_Device.IsNull()) return;

    m_UPnP->RemoveDevice(m_ServerHolder->m_Device);
    m_ServerHolder->m_Device = NULL;
}

/*----------------------------------------------------------------------
|   CUPnP::CreateRenderer
+---------------------------------------------------------------------*/
CUPnPRenderer*
CUPnP::CreateRenderer(int port /* = 0 */)
{
    CUPnPRenderer* device =
        new CUPnPRenderer(CSysInfo::GetDeviceName().c_str(),
                          false,
                          (CUPnPSettings::GetInstance().GetRendererUUID().length() ? CUPnPSettings::GetInstance().GetRendererUUID().c_str() : NULL),
                          port);

    device->m_PresentationURL =
        NPT_HttpUrl(m_IP.c_str(),
                    CSettings::GetInstance().GetInt(CSettings::SETTING_SERVICES_WEBSERVERPORT),
                    "/").ToString();
    device->m_ModelName        = "Kodi";
    device->m_ModelNumber      = CSysInfo::GetVersion().c_str();
    device->m_ModelDescription = "Kodi - Media Renderer";
    device->m_ModelURL         = "http://kodi.tv/";
    device->m_Manufacturer     = "XBMC Foundation";
    device->m_ManufacturerURL  = "http://kodi.tv/";

    return device;
}

/*----------------------------------------------------------------------
|   CUPnP::StartRenderer
+---------------------------------------------------------------------*/
bool CUPnP::StartRenderer()
{
    if (!m_RendererHolder->m_Device.IsNull()) return false;

    std::string filename = URIUtils::AddFileToFolder(CProfilesManager::GetInstance().GetUserDataFolder(), "upnpserver.xml");
    CUPnPSettings::GetInstance().Load(filename);

    m_RendererHolder->m_Device = CreateRenderer(CUPnPSettings::GetInstance().GetRendererPort());

    NPT_Result res = m_UPnP->AddDevice(m_RendererHolder->m_Device);

    // failed most likely because port is in use, try again with random port now
    if (NPT_FAILED(res) && CUPnPSettings::GetInstance().GetRendererPort() != 0) {
        m_RendererHolder->m_Device = CreateRenderer(0);

        res = m_UPnP->AddDevice(m_RendererHolder->m_Device);
    }

    // save port but don't overwrite saved settings if random
    if (NPT_SUCCEEDED(res) && CUPnPSettings::GetInstance().GetRendererPort() == 0) {
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
    if (m_RendererHolder->m_Device.IsNull()) return;

    m_UPnP->RemoveDevice(m_RendererHolder->m_Device);
    m_RendererHolder->m_Device = NULL;
}

/*----------------------------------------------------------------------
|   CUPnP::UpdateState
+---------------------------------------------------------------------*/
void CUPnP::UpdateState()
{
  if (!m_RendererHolder->m_Device.IsNull())
      ((CUPnPRenderer*)m_RendererHolder->m_Device.AsPointer())->UpdateState();
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
