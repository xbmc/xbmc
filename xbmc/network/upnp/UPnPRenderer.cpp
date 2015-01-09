/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "network/Network.h"
#include "UPnPRenderer.h"
#include "UPnP.h"
#include "UPnPInternal.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "FileItem.h"
#include "filesystem/SpecialProtocol.h"
#include "GUIInfoManager.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/Key.h"
#include "pictures/GUIWindowSlideShow.h"
#include "pictures/PictureInfoTag.h"
#include "interfaces/AnnouncementManager.h"
#include "settings/Settings.h"
#include "TextureDatabase.h"
#include "ThumbLoader.h"
#include "URL.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/StringUtils.h"
#include "playlists/PlayList.h"
#include "GUIUserMessages.h"

NPT_SET_LOCAL_LOGGER("xbmc.upnp.renderer")

using namespace ANNOUNCEMENT;

namespace UPNP
{

/*----------------------------------------------------------------------
|   CUPnPRenderer::CUPnPRenderer
+---------------------------------------------------------------------*/
CUPnPRenderer::CUPnPRenderer(const char* friendly_name, bool show_ip /*= false*/,
                             const char* uuid /*= NULL*/, unsigned int port /*= 0*/)
    : PLT_MediaRenderer(friendly_name, show_ip, uuid, port)
{
    CAnnouncementManager::Get().AddAnnouncer(this);
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::~CUPnPRenderer
+---------------------------------------------------------------------*/
CUPnPRenderer::~CUPnPRenderer()
{
    CAnnouncementManager::Get().RemoveAnnouncer(this);
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
        ",http-get:*:audio/mkv:*"
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
        ",http-get:*:audio/flac:*"
        ",http-get:*:audio/x-flac:*"
        ",http-get:*:audio/x-aiff:*"
        ",http-get:*:audio/x-pn-realaudio:*"
        ",http-get:*:audio/x-realaudio:*"
        ",http-get:*:audio/x-wav:*"
        ",http-get:*:audio/x-matroska:*"
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
        ",http-get:*:video/divx:*"
        ",http-get:*:video/mpeg:*"
        ",http-get:*:video/fli:*"
        ",http-get:*:video/flv:*"
        ",http-get:*:video/quicktime:*"
        ",http-get:*:video/vnd.vivo:*"
        ",http-get:*:video/vc1:*"
        ",http-get:*:video/ogg:*"
        ",http-get:*:video/mp4:*"
        ",http-get:*:video/mkv:*"
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
        ",http-get:*:video/xvid:*"
        ",http-get:*:video/x-divx:*"
        ",http-get:*:video/x-matroska:*"
        ",http-get:*:video/x-mkv:*"
        ",http-get:*:video/x-ms-wmv:*"
        ",http-get:*:video/x-ms-avi:*"
        ",http-get:*:video/x-flv:*"
        ",http-get:*:video/x-fli:*"
        ",http-get:*:video/x-ms-asf:*"
        ",http-get:*:video/x-ms-asx:*"
        ",http-get:*:video/x-ms-wmx:*"
        ",http-get:*:video/x-ms-wvx:*"
        ",http-get:*:video/x-msvideo:*"
        ",http-get:*:video/x-xvid:*"
        );

    NPT_CHECK_FATAL(FindServiceByType("urn:schemas-upnp-org:service:AVTransport:1", service));
    service->SetStateVariable("NextAVTransportURI", "");
    service->SetStateVariable("NextAVTransportURIMetadata", "");

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::ProcessHttpRequest
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::ProcessHttpGetRequest(NPT_HttpRequest&              request,
                                  const NPT_HttpRequestContext& context,
                                  NPT_HttpResponse&             response)
{
    // get the address of who sent us some data back
    NPT_String  ip_address = context.GetRemoteAddress().GetIpAddress().ToString();
    NPT_String  method     = request.GetMethod();
    NPT_String  protocol   = request.GetProtocol();
    NPT_HttpUrl url        = request.GetUrl();

    if (url.GetPath() == "/thumb") {
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

            // prevent hackers from accessing files outside of our root
            if ((filepath.Find("/..") >= 0) || (filepath.Find("\\..") >=0)) {
                return NPT_FAILURE;
            }

            // open the file
            std::string path (CURL::Decode((const char*) filepath));
            NPT_File file(path.c_str());
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
|   CUPnPRenderer::Announce
+---------------------------------------------------------------------*/
void
CUPnPRenderer::Announce(AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
    if (strcmp(sender, "xbmc") != 0)
      return;

    NPT_AutoLock lock(m_state);
    PLT_Service *avt, *rct;

    if (flag == Player) {
        if (NPT_FAILED(FindServiceByType("urn:schemas-upnp-org:service:AVTransport:1", avt)))
            return;
        if (strcmp(message, "OnPlay") == 0) {
            avt->SetStateVariable("AVTransportURI", g_application.CurrentFile().c_str());
            avt->SetStateVariable("CurrentTrackURI", g_application.CurrentFile().c_str());

            NPT_String meta;
            if (NPT_SUCCEEDED(GetMetadata(meta))) {
                avt->SetStateVariable("CurrentTrackMetadata", meta);
                avt->SetStateVariable("AVTransportURIMetaData", meta);
            }

            avt->SetStateVariable("TransportPlaySpeed", NPT_String::FromInteger(data["speed"].asInteger()));
            avt->SetStateVariable("TransportState", "PLAYING");

            /* this could be a transition to next track, so clear next */
            avt->SetStateVariable("NextAVTransportURI", "");
            avt->SetStateVariable("NextAVTransportURIMetaData", "");
        }
        else if (strcmp(message, "OnPause") == 0) {
            avt->SetStateVariable("TransportPlaySpeed", NPT_String::FromInteger(data["speed"].asInteger()));
            avt->SetStateVariable("TransportState", "PAUSED_PLAYBACK");
        }
        else if (strcmp(message, "OnSpeedChanged") == 0) {
            avt->SetStateVariable("TransportPlaySpeed", NPT_String::FromInteger(data["speed"].asInteger()));
        }
    }
    else if (flag == Application && strcmp(message, "OnVolumeChanged") == 0) {
        if (NPT_FAILED(FindServiceByType("urn:schemas-upnp-org:service:RenderingControl:1", rct)))
            return;

        std::string buffer;

        buffer = StringUtils::Format("%" PRId64, data["volume"].asInteger());
        rct->SetStateVariable("Volume", buffer.c_str());

        buffer = StringUtils::Format("%" PRId64, 256 * (data["volume"].asInteger() * 60 - 60) / 100);
        rct->SetStateVariable("VolumeDb", buffer.c_str());

        rct->SetStateVariable("Mute", data["muted"].asBoolean() ? "1" : "0");
    }
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::UpdateState
+---------------------------------------------------------------------*/
void
CUPnPRenderer::UpdateState()
{
    NPT_AutoLock lock(m_state);

    PLT_Service *avt;

    if (NPT_FAILED(FindServiceByType("urn:schemas-upnp-org:service:AVTransport:1", avt)))
        return;

    /* don't update state while transitioning */
    NPT_String state;
    avt->GetStateVariableValue("TransportState", state);
    if(state == "TRANSITIONING")
        return;

    avt->SetStateVariable("TransportStatus", "OK");

    if (g_application.m_pPlayer->IsPlaying() || g_application.m_pPlayer->IsPausedPlayback()) {
        avt->SetStateVariable("NumberOfTracks", "1");
        avt->SetStateVariable("CurrentTrack", "1");

        std::string buffer = g_infoManager.GetCurrentPlayTime(TIME_FORMAT_HH_MM_SS);
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

    } else if (g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW) {
        avt->SetStateVariable("TransportState", "PLAYING");

        avt->SetStateVariable("AVTransportURI" , g_infoManager.GetPictureLabel(SLIDE_FILE_PATH).c_str());
        avt->SetStateVariable("CurrentTrackURI", g_infoManager.GetPictureLabel(SLIDE_FILE_PATH).c_str());
        avt->SetStateVariable("TransportPlaySpeed", "1");

        CGUIWindowSlideShow *slideshow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
        if (slideshow)
        {
          std::string index;
          index = StringUtils::Format("%d", slideshow->NumSlides());
          avt->SetStateVariable("NumberOfTracks", index.c_str());
          index = StringUtils::Format("%d", slideshow->CurrentSlide());
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
        avt->SetStateVariable("NextAVTransportURI", "");
        avt->SetStateVariable("NextAVTransportURIMetaData", "");
    }
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::SetupIcons
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::SetupIcons()
{
    NPT_String file_root = CSpecialProtocol::TranslatePath("special://xbmc/media/").c_str();
    AddIcon(
        PLT_DeviceIcon("image/png", 256, 256, 8, "/icon256x256.png"),
        file_root);
    AddIcon(
        PLT_DeviceIcon("image/png", 120, 120, 8, "/icon120x120.png"),
        file_root);
    AddIcon(
        PLT_DeviceIcon("image/png", 48, 48, 8, "/icon48x48.png"),
        file_root);
    AddIcon(
        PLT_DeviceIcon("image/png", 32, 32, 8, "/icon32x32.png"),
        file_root);
    AddIcon(
        PLT_DeviceIcon("image/png", 16, 16, 8, "/icon16x16.png"),
        file_root);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::GetMetadata
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::GetMetadata(NPT_String& meta)
{
    NPT_Result res = NPT_FAILURE;
    CFileItem item(g_application.CurrentFileItem());
    NPT_String file_path, tmp;

    // we pass an empty CThumbLoader reference, as it can't be used
    // without CUPnPServer enabled
    NPT_Reference<CThumbLoader> thumb_loader;
    PLT_MediaObject* object = BuildObject(item, file_path, false, thumb_loader);
    if (object) {
        // fetch the item's artwork
        std::string thumb;
        if (object->m_ObjectClass.type == "object.item.audioItem.musicTrack")
            thumb = g_infoManager.GetImage(MUSICPLAYER_COVER, -1);
        else
            thumb = g_infoManager.GetImage(VIDEOPLAYER_COVER, -1);

        thumb = CTextureUtils::GetWrappedImageURL(thumb);

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
            "/thumb",
            query.ToString()).ToString();
        // Set DLNA profileID by extension, defaulting to JPEG.
        if (URIUtils::HasExtension(item.GetArt("thumb"), ".png")) {
            art.dlna_profile = "PNG_TN";
        } else {
            art.dlna_profile = "JPEG_TN";
        }
	object->m_ExtraInfo.album_arts.Add(art);

        res = PLT_Didl::ToDidl(*object, "*", tmp);
        meta = didl_header + tmp + didl_footer;
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
    } else if (!g_application.m_pPlayer->IsPausedPlayback())
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
    } else if (g_application.m_pPlayer->IsPausedPlayback()) {
      CApplicationMessenger::Get().MediaPause();
    } else if (!g_application.m_pPlayer->IsPlaying()) {
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
    if (!g_application.m_pPlayer->IsPlaying() && g_windowManager.GetActiveWindow() != WINDOW_SLIDESHOW) {
        service->SetStateVariable("TransportState", "STOPPED");
        service->SetStateVariable("TransportStatus", "OK");
        service->SetStateVariable("TransportPlaySpeed", "1");
        service->SetStateVariable("AVTransportURI", uri);
        service->SetStateVariable("AVTransportURIMetaData", meta);
        service->SetStateVariable("NextAVTransportURI", "");
        service->SetStateVariable("NextAVTransportURIMetaData", "");

        NPT_CHECK_SEVERE(action->SetArgumentsOutFromStateVariable());
        return NPT_SUCCESS;
    }

    return PlayMedia(uri, meta, action.AsPointer());
}

/*----------------------------------------------------------------------
 |   CUPnPRenderer::OnSetAVTransportURI
 +---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::OnSetNextAVTransportURI(PLT_ActionReference& action)
{
    NPT_String uri, meta;
    PLT_Service* service;
    NPT_CHECK_SEVERE(FindServiceByType("urn:schemas-upnp-org:service:AVTransport:1", service));

    NPT_CHECK_SEVERE(action->GetArgumentValue("NextURI", uri));
    NPT_CHECK_SEVERE(action->GetArgumentValue("NextURIMetaData", meta));

    CFileItemPtr item = GetFileItem(uri, meta);
    if (!item) {
        return NPT_FAILURE;
    }

    if (g_application.m_pPlayer->IsPlaying()) {

        int playlist = PLAYLIST_MUSIC;
        if(item->IsVideo())
          playlist = PLAYLIST_VIDEO;

        {   CSingleLock lock(g_graphicsContext);
            g_playlistPlayer.ClearPlaylist(playlist);
            g_playlistPlayer.Add(playlist, item);

            g_playlistPlayer.SetCurrentSong(-1);
            g_playlistPlayer.SetCurrentPlaylist(playlist);
        }

        CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
        g_windowManager.SendThreadMessage(msg);


        service->SetStateVariable("NextAVTransportURI", uri);
        service->SetStateVariable("NextAVTransportURIMetaData", meta);

        NPT_CHECK_SEVERE(action->SetArgumentsOutFromStateVariable());

        return NPT_SUCCESS;

  } else if (g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW) {
        return NPT_FAILURE;
  } else {
        return NPT_FAILURE;
  }
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::PlayMedia
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::PlayMedia(const NPT_String& uri, const NPT_String& meta, PLT_Action* action)
{
    PLT_Service* service;
    NPT_CHECK_SEVERE(FindServiceByType("urn:schemas-upnp-org:service:AVTransport:1", service));

    { NPT_AutoLock lock(m_state);
      service->SetStateVariable("TransportState", "TRANSITIONING");
      service->SetStateVariable("TransportStatus", "OK");
    }

    CFileItemPtr item = GetFileItem(uri, meta);
    if (!item) {
        return NPT_FAILURE;
    }

    if (item->IsPicture()) {
        CApplicationMessenger::Get().PictureShow(item->GetPath());
    } else {
        CApplicationMessenger::Get().MediaPlay(*item, false);
    }

    if (g_application.m_pPlayer->IsPlaying() || g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW) {
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

    service->SetStateVariable("NextAVTransportURI", "");
    service->SetStateVariable("NextAVTransportURIMetaData", "");

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
    if((mute == "1") ^ g_application.IsMuted())
        g_application.ToggleMute();
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::OnSeek
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::OnSeek(PLT_ActionReference& action)
{
    if (!g_application.m_pPlayer->IsPlaying()) return NPT_ERROR_INVALID_STATE;

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

} /* namespace UPNP */

