/*
* UPnP Support for XBMC
* Copyright (c) 2006 c0diq (Sylvain Rebaud)
* Portions Copyright (c) by the authors of libPlatinum
*
* http://www.plutinosoft.com/blog/category/platinum/
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "threads/SystemClock.h"
#include "UPnP.h"
#include "UPnPInternal.h"
#include "UPnPServer.h"
#include "utils/URIUtils.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "network/Network.h"
#include "utils/log.h"
#include "Platinum.h"
#include "pictures/PictureInfoTag.h"
#include "pictures/GUIWindowSlideShow.h"
#include "URL.h"
#include "settings/GUISettings.h"
#include "GUIUserMessages.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "guilib/GUIWindowManager.h"
#include "GUIInfoManager.h"
#include "utils/TimeUtils.h"
#include "guilib/Key.h"
#include "Util.h"

using namespace std;
using namespace UPNP;

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
NPT_Console::Output(const char* message)
{
    CLog::Log(LOGDEBUG, "%s", message);
}

namespace UPNP
{

/*----------------------------------------------------------------------
|   static
+---------------------------------------------------------------------*/
CUPnP* CUPnP::upnp = NULL;

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
|   CUPnPRenderer
+---------------------------------------------------------------------*/
class CUPnPRenderer : public PLT_MediaRenderer
{
public:
    CUPnPRenderer(const char*  friendly_name,
                  bool         show_ip = false,
                  const char*  uuid = NULL,
                  unsigned int port = 0);

    void UpdateState();

    // Http server handler
    virtual NPT_Result ProcessHttpRequest(NPT_HttpRequest&              request,
                                          const NPT_HttpRequestContext& context,
                                          NPT_HttpResponse&             response);

    // AVTransport methods
    virtual NPT_Result OnNext(PLT_ActionReference& action);
    virtual NPT_Result OnPause(PLT_ActionReference& action);
    virtual NPT_Result OnPlay(PLT_ActionReference& action);
    virtual NPT_Result OnPrevious(PLT_ActionReference& action);
    virtual NPT_Result OnStop(PLT_ActionReference& action);
    virtual NPT_Result OnSeek(PLT_ActionReference& action);
    virtual NPT_Result OnSetAVTransportURI(PLT_ActionReference& action);

    // RenderingControl methods
    virtual NPT_Result OnSetVolume(PLT_ActionReference& action);
    virtual NPT_Result OnSetMute(PLT_ActionReference& action);

private:
    NPT_Result SetupServices();
    NPT_Result GetMetadata(NPT_String& meta);
    NPT_Result PlayMedia(const char* uri,
                         const char* metadata = NULL,
                         PLT_Action* action = NULL);
    NPT_Mutex m_state;
};

/*----------------------------------------------------------------------
|   CUPnPRenderer::CUPnPRenderer
+---------------------------------------------------------------------*/
CUPnPRenderer::CUPnPRenderer(const char*  friendly_name,
                             bool         show_ip /* = false */,
                             const char*  uuid /* = NULL */,
                             unsigned int port /* = 0 */) :
    PLT_MediaRenderer(friendly_name,
                      show_ip,
                      uuid,
                      port)
{
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::SetupServices
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::SetupServices()
{
    NPT_CHECK(PLT_MediaRenderer::SetupServices());

    // update what we can play
    PLT_Service* service = NULL;
    NPT_CHECK_FATAL(FindServiceByType("urn:schemas-upnp-org:service:ConnectionManager:1", service));
    service->SetStateVariable("SinkProtocolInfo"
        ,"http-get:*:*:*"
        ",xbmc-get:*:*:*"
        ",http-get:*:audio/mpegurl:*"
        ",http-get:*:audio/mpeg:*"
        ",http-get:*:audio/mpeg3:*"
        ",http-get:*:audio/mp3:*"
        ",http-get:*:audio/mp4:*"
        ",http-get:*:audio/basic:*"
        ",http-get:*:audio/midi:*"
        ",http-get:*:audio/ulaw:*"
        ",http-get:*:audio/ogg:*"
        ",http-get:*:audio/DVI4:*"
        ",http-get:*:audio/G722:*"
        ",http-get:*:audio/G723:*"
        ",http-get:*:audio/G726-16:*"
        ",http-get:*:audio/G726-24:*"
        ",http-get:*:audio/G726-32:*"
        ",http-get:*:audio/G726-40:*"
        ",http-get:*:audio/G728:*"
        ",http-get:*:audio/G729:*"
        ",http-get:*:audio/G729D:*"
        ",http-get:*:audio/G729E:*"
        ",http-get:*:audio/GSM:*"
        ",http-get:*:audio/GSM-EFR:*"
        ",http-get:*:audio/L8:*"
        ",http-get:*:audio/L16:*"
        ",http-get:*:audio/LPC:*"
        ",http-get:*:audio/MPA:*"
        ",http-get:*:audio/PCMA:*"
        ",http-get:*:audio/PCMU:*"
        ",http-get:*:audio/QCELP:*"
        ",http-get:*:audio/RED:*"
        ",http-get:*:audio/VDVI:*"
        ",http-get:*:audio/ac3:*"
        ",http-get:*:audio/vorbis:*"
        ",http-get:*:audio/speex:*"
        ",http-get:*:audio/x-aiff:*"
        ",http-get:*:audio/x-pn-realaudio:*"
        ",http-get:*:audio/x-realaudio:*"
        ",http-get:*:audio/x-wav:*"
        ",http-get:*:audio/x-ms-wma:*"
        ",http-get:*:audio/x-mpegurl:*"
        ",http-get:*:application/x-shockwave-flash:*"
        ",http-get:*:application/ogg:*"
        ",http-get:*:application/sdp:*"
        ",http-get:*:image/gif:*"
        ",http-get:*:image/jpeg:*"
        ",http-get:*:image/ief:*"
        ",http-get:*:image/png:*"
        ",http-get:*:image/tiff:*"
        ",http-get:*:video/avi:*"
        ",http-get:*:video/mpeg:*"
        ",http-get:*:video/fli:*"
        ",http-get:*:video/flv:*"
        ",http-get:*:video/quicktime:*"
        ",http-get:*:video/vnd.vivo:*"
        ",http-get:*:video/vc1:*"
        ",http-get:*:video/ogg:*"
        ",http-get:*:video/mp4:*"
        ",http-get:*:video/BT656:*"
        ",http-get:*:video/CelB:*"
        ",http-get:*:video/JPEG:*"
        ",http-get:*:video/H261:*"
        ",http-get:*:video/H263:*"
        ",http-get:*:video/H263-1998:*"
        ",http-get:*:video/H263-2000:*"
        ",http-get:*:video/MPV:*"
        ",http-get:*:video/MP2T:*"
        ",http-get:*:video/MP1S:*"
        ",http-get:*:video/MP2P:*"
        ",http-get:*:video/BMPEG:*"
        ",http-get:*:video/x-ms-wmv:*"
        ",http-get:*:video/x-ms-avi:*"
        ",http-get:*:video/x-flv:*"
        ",http-get:*:video/x-fli:*"
        ",http-get:*:video/x-ms-asf:*"
        ",http-get:*:video/x-ms-asx:*"
        ",http-get:*:video/x-ms-wmx:*"
        ",http-get:*:video/x-ms-wvx:*"
        ",http-get:*:video/x-msvideo:*"
        );
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::ProcessHttpRequest
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::ProcessHttpRequest(NPT_HttpRequest&              request,
                                  const NPT_HttpRequestContext& context,
                                  NPT_HttpResponse&             response)
{
    // get the address of who sent us some data back
    NPT_String  ip_address = context.GetRemoteAddress().GetIpAddress().ToString();
    NPT_String  method     = request.GetMethod();
    NPT_String  protocol   = request.GetProtocol();
    NPT_HttpUrl url        = request.GetUrl();

    if (url.GetPath() == "/thumb.jpg") {
        NPT_HttpUrlQuery query(url.GetQuery());
        NPT_String filepath = query.GetField("path");
        if (!filepath.IsEmpty()) {
            NPT_HttpEntity* entity = response.GetEntity();
            if (entity == NULL) return NPT_ERROR_INVALID_STATE;

            // check the method
            if (request.GetMethod() != NPT_HTTP_METHOD_GET &&
                request.GetMethod() != NPT_HTTP_METHOD_HEAD) {
                response.SetStatus(405, "Method Not Allowed");
                return NPT_SUCCESS;
            }

            // ensure that the request's path is a valid thumb path
            if (URIUtils::IsRemote(filepath.GetChars()) ||
                !filepath.StartsWith(g_settings.GetUserDataFolder())) {
                response.SetStatus(404, "Not Found");
                return NPT_SUCCESS;
            }

            // prevent hackers from accessing files outside of our root
            if ((filepath.Find("/..") >= 0) || (filepath.Find("\\..") >=0)) {
                return NPT_FAILURE;
            }

            // open the file
            NPT_File file(filepath);
            NPT_Result result = file.Open(NPT_FILE_OPEN_MODE_READ);
            if (NPT_FAILED(result)) {
                response.SetStatus(404, "Not Found");
                return NPT_SUCCESS;
            }
            NPT_InputStreamReference stream;
            file.GetInputStream(stream);
            entity->SetContentType(GetMimeType(filepath));
            entity->SetInputStream(stream, true);

            return NPT_SUCCESS;
        }
    }

    return PLT_MediaRenderer::ProcessHttpGetRequest(request, context, response);
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::UpdateState
+---------------------------------------------------------------------*/
void
CUPnPRenderer::UpdateState()
{
    NPT_AutoLock lock(m_state);

    PLT_Service *avt, *rct;
    if (NPT_FAILED(FindServiceByType("urn:schemas-upnp-org:service:AVTransport:1", avt)))
        return;
    if (NPT_FAILED(FindServiceByType("urn:schemas-upnp-org:service:RenderingControl:1", rct)))
        return;

    /* don't update state while transitioning */
    NPT_String state;
    avt->GetStateVariableValue("TransportState", state);
    if(state == "TRANSITIONING")
        return;

    CStdString buffer;
    int volume;
    if (g_settings.m_bMute) {
        rct->SetStateVariable("Mute", "1");
    } else {
        rct->SetStateVariable("Mute", "0");
    }
    volume = g_application.GetVolume();

    buffer.Format("%d", volume);
    rct->SetStateVariable("Volume", buffer.c_str());

    buffer.Format("%d", 256 * (volume * 60 - 60) / 100);
    rct->SetStateVariable("VolumeDb", buffer.c_str());

    if (g_application.IsPlaying() || g_application.IsPaused()) {
        if (g_application.IsPaused()) {
            avt->SetStateVariable("TransportState", "PAUSED_PLAYBACK");
        } else {
            avt->SetStateVariable("TransportState", "PLAYING");
        }

        avt->SetStateVariable("TransportStatus", "OK");
        avt->SetStateVariable("TransportPlaySpeed", (const char*)NPT_String::FromInteger(g_application.GetPlaySpeed()));
        avt->SetStateVariable("NumberOfTracks", "1");
        avt->SetStateVariable("CurrentTrack", "1");

        buffer = g_infoManager.GetCurrentPlayTime(TIME_FORMAT_HH_MM_SS);
        avt->SetStateVariable("RelativeTimePosition", buffer.c_str());
        avt->SetStateVariable("AbsoluteTimePosition", buffer.c_str());

        buffer = g_infoManager.GetDuration(TIME_FORMAT_HH_MM_SS);
        if (buffer.length() > 0) {
          avt->SetStateVariable("CurrentTrackDuration", buffer.c_str());
          avt->SetStateVariable("CurrentMediaDuration", buffer.c_str());
        } else {
          avt->SetStateVariable("CurrentTrackDuration", "00:00:00");
          avt->SetStateVariable("CurrentMediaDuration", "00:00:00");
        }

        avt->SetStateVariable("AVTransportURI", g_application.CurrentFile().c_str());
        avt->SetStateVariable("CurrentTrackURI", g_application.CurrentFile().c_str());

        NPT_String metadata;
        avt->GetStateVariableValue("AVTransportURIMetaData", metadata);
        // try to recreate the didl dynamically if not set
        if (metadata.IsEmpty()) {
            GetMetadata(metadata);
        }
        avt->SetStateVariable("CurrentTrackMetadata", metadata);
        avt->SetStateVariable("AVTransportURIMetaData", metadata);
    } else if (g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW) {
        avt->SetStateVariable("TransportState", "PLAYING");

        avt->SetStateVariable("AVTransportURI" , g_infoManager.GetPictureLabel(SLIDE_FILE_PATH));
        avt->SetStateVariable("CurrentTrackURI", g_infoManager.GetPictureLabel(SLIDE_FILE_PATH));
        avt->SetStateVariable("TransportPlaySpeed", "1");

        CGUIWindowSlideShow *slideshow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
        if (slideshow)
        {
          CStdString index;
          index.Format("%d", slideshow->NumSlides());
          avt->SetStateVariable("NumberOfTracks", index.c_str());
          index.Format("%d", slideshow->CurrentSlide());
          avt->SetStateVariable("CurrentTrack", index.c_str());

        }

        avt->SetStateVariable("CurrentTrackMetadata", "");
        avt->SetStateVariable("AVTransportURIMetaData", "");

    } else {
        avt->SetStateVariable("TransportState", "STOPPED");
        avt->SetStateVariable("TransportPlaySpeed", "1");
        avt->SetStateVariable("NumberOfTracks", "0");
        avt->SetStateVariable("CurrentTrack", "0");
        avt->SetStateVariable("RelativeTimePosition", "00:00:00");
        avt->SetStateVariable("AbsoluteTimePosition", "00:00:00");
        avt->SetStateVariable("CurrentTrackDuration", "00:00:00");
        avt->SetStateVariable("CurrentMediaDuration", "00:00:00");
    }
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::GetMetadata
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::GetMetadata(NPT_String& meta)
{
    NPT_Result res = NPT_FAILURE;
    const CFileItem &item = g_application.CurrentFileItem();
    NPT_String file_path;
    PLT_MediaObject* object = BuildObject(item, file_path, false);
    if (object) {
        // fetch the path to the thumbnail
        CStdString thumb = g_infoManager.GetImage(MUSICPLAYER_COVER, -1); //TODO: Only audio for now

        NPT_String ip;
        if (g_application.getNetwork().GetFirstConnectedInterface()) {
            ip = g_application.getNetwork().GetFirstConnectedInterface()->GetCurrentIPAddress().c_str();
        }
        // build url, use the internal device http server to serv the image
        NPT_HttpUrlQuery query;
        query.AddField("path", thumb.c_str());
        PLT_AlbumArtInfo art;
        art.uri = NPT_HttpUrl(
            ip,
            m_URLDescription.GetPort(),
            "/thumb.jpg",
            query.ToString()).ToString();
        art.dlna_profile = "JPEG_TN";
        object->m_ExtraInfo.album_arts.Add(art);

        res = PLT_Didl::ToDidl(*object, "*", meta);
        delete object;
    }
    return res;
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::OnNext
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::OnNext(PLT_ActionReference& action)
{
    if (g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW) {
        CAction action(ACTION_NEXT_PICTURE);
        CApplicationMessenger::Get().SendAction(action, WINDOW_SLIDESHOW);
    } else {
        CApplicationMessenger::Get().PlayListPlayerNext();
    }
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::OnPause
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::OnPause(PLT_ActionReference& action)
{
    if (g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW) {
        CAction action(ACTION_PAUSE);
        CApplicationMessenger::Get().SendAction(action, WINDOW_SLIDESHOW);
    } else if (!g_application.IsPaused())
      CApplicationMessenger::Get().MediaPause();
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::OnPlay
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::OnPlay(PLT_ActionReference& action)
{
    if (g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW) {
        return NPT_SUCCESS;
    } else if (g_application.IsPaused()) {
      CApplicationMessenger::Get().MediaPause();
    } else if (!g_application.IsPlaying()) {
        NPT_String uri, meta;
        PLT_Service* service;
        // look for value set previously by SetAVTransportURI
        NPT_CHECK_SEVERE(FindServiceByType("urn:schemas-upnp-org:service:AVTransport:1", service));
        NPT_CHECK_SEVERE(service->GetStateVariableValue("AVTransportURI", uri));
        NPT_CHECK_SEVERE(service->GetStateVariableValue("AVTransportURIMetaData", meta));

        // if not set, use the current file being played
        PlayMedia(uri, meta);
    }
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::OnPrevious
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::OnPrevious(PLT_ActionReference& action)
{
    if (g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW) {
        CAction action(ACTION_PREV_PICTURE);
        CApplicationMessenger::Get().SendAction(action, WINDOW_SLIDESHOW);
    } else {
        CApplicationMessenger::Get().PlayListPlayerPrevious();
    }
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::OnStop
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::OnStop(PLT_ActionReference& action)
{
    if (g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW) {
        CAction action(ACTION_STOP);
        CApplicationMessenger::Get().SendAction(action, WINDOW_SLIDESHOW);
    } else {
        CApplicationMessenger::Get().MediaStop();
    }
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::OnSetAVTransportURI
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::OnSetAVTransportURI(PLT_ActionReference& action)
{
    NPT_String uri, meta;
    PLT_Service* service;
    NPT_CHECK_SEVERE(FindServiceByType("urn:schemas-upnp-org:service:AVTransport:1", service));

    NPT_CHECK_SEVERE(action->GetArgumentValue("CurrentURI", uri));
    NPT_CHECK_SEVERE(action->GetArgumentValue("CurrentURIMetaData", meta));

    // if not playing already, just keep around uri & metadata
    // and wait for play command
    if (!g_application.IsPlaying() && g_windowManager.GetActiveWindow() != WINDOW_SLIDESHOW) {
        service->SetStateVariable("TransportState", "STOPPED");
        service->SetStateVariable("TransportStatus", "OK");
        service->SetStateVariable("TransportPlaySpeed", "1");
        service->SetStateVariable("AVTransportURI", uri);
        service->SetStateVariable("AVTransportURIMetaData", meta);

        NPT_CHECK_SEVERE(action->SetArgumentsOutFromStateVariable());
        return NPT_SUCCESS;
    }

    return PlayMedia(uri, meta, action.AsPointer());
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::PlayMedia
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::PlayMedia(const char* uri, const char* meta, PLT_Action* action)
{
    bool bImageFile = false;
    PLT_Service* service;
    NPT_CHECK_SEVERE(FindServiceByType("urn:schemas-upnp-org:service:AVTransport:1", service));

    { NPT_AutoLock lock(m_state);
      service->SetStateVariable("TransportState", "TRANSITIONING");
      service->SetStateVariable("TransportStatus", "OK");
    }

    PLT_MediaObjectListReference list;
    PLT_MediaObject*             object = NULL;

    if (meta && NPT_SUCCEEDED(PLT_Didl::FromDidl(meta, list))) {
        list->Get(0, object);
    }

    if (object) {
        CFileItem item(uri, false);

        PLT_MediaItemResource* res = object->m_Resources.GetFirstItem();
        for(NPT_Cardinal i = 0; i < object->m_Resources.GetItemCount(); i++) {
            if(object->m_Resources[i].m_Uri == uri) {
                res = &object->m_Resources[i];
                break;
            }
        }
        for(NPT_Cardinal i = 0; i < object->m_Resources.GetItemCount(); i++) {
            if(object->m_Resources[i].m_ProtocolInfo.ToString().StartsWith("xbmc-get:")) {
                res = &object->m_Resources[i];
                item.SetPath(CStdString(res->m_Uri));
                break;
            }
        }

        if (res && res->m_ProtocolInfo.IsValid()) {
            item.SetMimeType((const char*)res->m_ProtocolInfo.GetContentType());
        }

        item.m_dateTime.SetFromDateString((const char*)object->m_Date);
        item.m_strTitle = (const char*)object->m_Title;
        item.SetLabel((const char*)object->m_Title);
        item.SetLabelPreformated(true);
        if (object->m_ExtraInfo.album_arts.GetItem(0)) {
            // only considers first album art
            item.SetThumbnailImage((const char*)object->m_ExtraInfo.album_arts.GetItem(0)->uri);
        }
        if (object->m_ObjectClass.type.StartsWith("object.item.audioItem")) {
            if(NPT_SUCCEEDED(PopulateTagFromObject(*item.GetMusicInfoTag(), *object, res)))
                item.SetLabelPreformated(false);
        } else if (object->m_ObjectClass.type.StartsWith("object.item.videoItem")) {
            if(NPT_SUCCEEDED(PopulateTagFromObject(*item.GetVideoInfoTag(), *object, res)))
                item.SetLabelPreformated(false);
        } else if (object->m_ObjectClass.type.StartsWith("object.item.imageItem")) {
            bImageFile = true;
        }
        bImageFile?CApplicationMessenger::Get().PictureShow(item.GetPath())
                  :CApplicationMessenger::Get().MediaPlay(item);
    } else {
        bImageFile = NPT_String(PLT_MediaObject::GetUPnPClass(uri)).StartsWith("object.item.imageItem", true);

        bImageFile?CApplicationMessenger::Get().PictureShow((const char*)uri)
                  :CApplicationMessenger::Get().MediaPlay((const char*)uri);
    }

    if (g_application.IsPlaying() || g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW) {
        NPT_AutoLock lock(m_state);
        service->SetStateVariable("TransportState", "PLAYING");
        service->SetStateVariable("TransportStatus", "OK");
        service->SetStateVariable("AVTransportURI", uri);
        service->SetStateVariable("AVTransportURIMetaData", meta);
    } else {
        NPT_AutoLock lock(m_state);
        service->SetStateVariable("TransportState", "STOPPED");
        service->SetStateVariable("TransportStatus", "ERROR_OCCURRED");
    }

    if (action) {
        NPT_CHECK_SEVERE(action->SetArgumentsOutFromStateVariable());
    }
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::OnSetVolume
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::OnSetVolume(PLT_ActionReference& action)
{
    NPT_String volume;
    NPT_CHECK_SEVERE(action->GetArgumentValue("DesiredVolume", volume));
    g_application.SetVolume((float)strtod((const char*)volume, NULL));
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::OnSetMute
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::OnSetMute(PLT_ActionReference& action)
{
    NPT_String mute;
    NPT_CHECK_SEVERE(action->GetArgumentValue("DesiredMute",mute));
    if((mute == "1") ^ g_settings.m_bMute)
        g_application.ToggleMute();
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::OnSeek
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::OnSeek(PLT_ActionReference& action)
{
    if (!g_application.IsPlaying()) return NPT_ERROR_INVALID_STATE;

    NPT_String unit, target;
    NPT_CHECK_SEVERE(action->GetArgumentValue("Unit", unit));
    NPT_CHECK_SEVERE(action->GetArgumentValue("Target", target));

    if (!unit.Compare("REL_TIME")) {
        // converts target to seconds
        NPT_UInt32 seconds;
        NPT_CHECK_SEVERE(PLT_Didl::ParseTimeStamp(target, seconds));
        g_application.SeekTime(seconds);
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CRendererReferenceHolder class
+---------------------------------------------------------------------*/
class CRendererReferenceHolder
{
public:
    PLT_DeviceHostReference m_Device;
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
            CStdString id = item_id;
            CURL::Encode(id);
            path += id.c_str();
            path += "/";
        }

        CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH);
        message.SetStringParam(path.GetChars());
        g_windowManager.SendThreadMessage(message);
    }
};


/*----------------------------------------------------------------------
|   CUPnP::CUPnP
+---------------------------------------------------------------------*/
CUPnP::CUPnP() :
    m_MediaBrowser(NULL),
    m_ServerHolder(new CDeviceHostReferenceHolder()),
    m_RendererHolder(new CRendererReferenceHolder()),
    m_CtrlPointHolder(new CCtrlPointReferenceHolder())
{
    // initialize upnp context
    m_UPnP = new PLT_UPnP();

    // keep main IP around
    if (g_application.getNetwork().GetFirstConnectedInterface()) {
        m_IP = g_application.getNetwork().GetFirstConnectedInterface()->GetCurrentIPAddress().c_str();
    }
    NPT_List<NPT_IpAddress> list;
    if (NPT_SUCCEEDED(PLT_UPnPMessageHelper::GetIPAddresses(list))) {
        m_IP = (*(list.GetFirstItem())).ToString();
    }

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
    StopServer();

    delete m_UPnP;
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
|   CUPnP::StartClient
+---------------------------------------------------------------------*/
void
CUPnP::StartClient()
{
    if (!m_CtrlPointHolder->m_CtrlPoint.IsNull()) return;

    // create controlpoint
    m_CtrlPointHolder->m_CtrlPoint = new PLT_CtrlPoint();

    // start it
    m_UPnP->AddCtrlPoint(m_CtrlPointHolder->m_CtrlPoint);

    // start browser
    m_MediaBrowser = new CMediaBrowser(m_CtrlPointHolder->m_CtrlPoint);
}

/*----------------------------------------------------------------------
|   CUPnP::StopClient
+---------------------------------------------------------------------*/
void
CUPnP::StopClient()
{
    if (m_CtrlPointHolder->m_CtrlPoint.IsNull()) return;

    m_UPnP->RemoveCtrlPoint(m_CtrlPointHolder->m_CtrlPoint);
    m_CtrlPointHolder->m_CtrlPoint = NULL;

    delete m_MediaBrowser;
    m_MediaBrowser = NULL;
}

/*----------------------------------------------------------------------
|   CUPnP::CreateServer
+---------------------------------------------------------------------*/
CUPnPServer*
CUPnP::CreateServer(int port /* = 0 */)
{
    CUPnPServer* device =
        new CUPnPServer(g_infoManager.GetLabel(SYSTEM_FRIENDLY_NAME),
                        g_settings.m_UPnPUUIDServer.length()?g_settings.m_UPnPUUIDServer.c_str():NULL,
                        port);

    // trying to set optional upnp values for XP UPnP UI Icons to detect us
    // but it doesn't work anyways as it requires multicast for XP to detect us
    device->m_PresentationURL =
        NPT_HttpUrl(m_IP,
                    atoi(g_guiSettings.GetString("services.webserverport")),
                    "/").ToString();

    device->m_ModelName        = "XBMC Media Center";
    device->m_ModelNumber      = g_infoManager.GetVersion().c_str();
    device->m_ModelDescription = "XBMC Media Center - Media Server";
    device->m_ModelURL         = "http://www.xbmc.org/";
    device->m_Manufacturer     = "Team XBMC";
    device->m_ManufacturerURL  = "http://www.xbmc.org/";

    device->SetDelegate(device);
    return device;
}

/*----------------------------------------------------------------------
|   CUPnP::StartServer
+---------------------------------------------------------------------*/
void
CUPnP::StartServer()
{
    if (!m_ServerHolder->m_Device.IsNull()) return;

    // load upnpserver.xml so that g_settings.m_vecUPnPMusiCMediaSources, etc.. are loaded
    CStdString filename;
    URIUtils::AddFileToFolder(g_settings.GetUserDataFolder(), "upnpserver.xml", filename);
    g_settings.LoadUPnPXml(filename);

    // create the server with a XBox compatible friendlyname and UUID from upnpserver.xml if found
    m_ServerHolder->m_Device = CreateServer(g_settings.m_UPnPPortServer);

    // start server
    NPT_Result res = m_UPnP->AddDevice(m_ServerHolder->m_Device);
    if (NPT_FAILED(res)) {
        // if the upnp device port was not 0, it could have failed because
        // of port being in used, so restart with a random port
        if (g_settings.m_UPnPPortServer > 0) m_ServerHolder->m_Device = CreateServer(0);

        res = m_UPnP->AddDevice(m_ServerHolder->m_Device);
    }

    // save port but don't overwrite saved settings if port was random
    if (NPT_SUCCEEDED(res)) {
        if (g_settings.m_UPnPPortServer == 0) {
            g_settings.m_UPnPPortServer = m_ServerHolder->m_Device->GetPort();
        }
        CUPnPServer::m_MaxReturnedItems = UPNP_DEFAULT_MAX_RETURNED_ITEMS;
        if (g_settings.m_UPnPMaxReturnedItems > 0) {
            // must be > UPNP_DEFAULT_MIN_RETURNED_ITEMS
            CUPnPServer::m_MaxReturnedItems = max(UPNP_DEFAULT_MIN_RETURNED_ITEMS, g_settings.m_UPnPMaxReturnedItems);
        }
        g_settings.m_UPnPMaxReturnedItems = CUPnPServer::m_MaxReturnedItems;
    }

    // save UUID
    g_settings.m_UPnPUUIDServer = m_ServerHolder->m_Device->GetUUID();
    g_settings.SaveUPnPXml(filename);
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
        new CUPnPRenderer(g_infoManager.GetLabel(SYSTEM_FRIENDLY_NAME),
                          false,
                          (g_settings.m_UPnPUUIDRenderer.length() ? g_settings.m_UPnPUUIDRenderer.c_str() : NULL),
                          port);

    device->m_PresentationURL =
        NPT_HttpUrl(m_IP,
                    atoi(g_guiSettings.GetString("services.webserverport")),
                    "/").ToString();
    device->m_ModelName        = "XBMC Media Center";
    device->m_ModelNumber      = g_infoManager.GetVersion().c_str();
    device->m_ModelDescription = "XBMC Media Center - Media Renderer";
    device->m_ModelURL         = "http://www.xbmc.org/";
    device->m_Manufacturer     = "Team XBMC";
    device->m_ManufacturerURL  = "http://www.xbmc.org/";

    return device;
}

/*----------------------------------------------------------------------
|   CUPnP::StartRenderer
+---------------------------------------------------------------------*/
void CUPnP::StartRenderer()
{
    if (!m_RendererHolder->m_Device.IsNull()) return;

    CStdString filename;
    URIUtils::AddFileToFolder(g_settings.GetUserDataFolder(), "upnpserver.xml", filename);
    g_settings.LoadUPnPXml(filename);

    m_RendererHolder->m_Device = CreateRenderer(g_settings.m_UPnPPortRenderer);

    NPT_Result res = m_UPnP->AddDevice(m_RendererHolder->m_Device);

    // failed most likely because port is in use, try again with random port now
    if (NPT_FAILED(res) && g_settings.m_UPnPPortRenderer != 0) {
        m_RendererHolder->m_Device = CreateRenderer(0);

        res = m_UPnP->AddDevice(m_RendererHolder->m_Device);
    }

    // save port but don't overwrite saved settings if random
    if (NPT_SUCCEEDED(res) && g_settings.m_UPnPPortRenderer == 0) {
        g_settings.m_UPnPPortRenderer = m_RendererHolder->m_Device->GetPort();
    }

    // save UUID
    g_settings.m_UPnPUUIDRenderer = m_RendererHolder->m_Device->GetUUID();
    g_settings.SaveUPnPXml(filename);
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

} /* namespace UPNP */
