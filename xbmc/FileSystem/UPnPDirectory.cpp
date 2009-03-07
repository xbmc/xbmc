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


#include "stdafx.h"
#include "Util.h"
#include "UPnPDirectory.h"
#include "UPnP.h"
#include "Platinum.h"
#include "PltSyncMediaBrowser.h"
#include "VideoInfoTag.h"
#include "MusicInfoTag.h"
#include "FileItem.h"
#include "GUIWindowManager.h"
#include "GUIDialogProgress.h"

using namespace MUSIC_INFO;
using namespace DIRECTORY;
using namespace XFILE;

namespace
{
  static const NPT_String JoinString(const NPT_List<NPT_String>& array, const NPT_String& delimiter)
  {
    NPT_String result;

    for(NPT_List<NPT_String>::Iterator it = array.GetFirstItem(); it; it++ )
        result += (*it) + delimiter;

    if(result.IsEmpty())
        return "";
    else
        return result.SubString(delimiter.GetLength());
  }

}

namespace DIRECTORY
{
/*----------------------------------------------------------------------
|   CProtocolFinder
+---------------------------------------------------------------------*/
class CProtocolFinder {
public:
    CProtocolFinder(const char* protocol) : m_Protocol(protocol) {}
    bool operator()(const PLT_MediaItemResource& resource) const {
        return (resource.m_ProtocolInfo.Compare(m_Protocol, true) == 0);
    }
private:
    NPT_String m_Protocol;
};

/*----------------------------------------------------------------------
|   CUPnPDirectory::GetFriendlyName
+---------------------------------------------------------------------*/
const char*
CUPnPDirectory::GetFriendlyName(const char* url)
{
    NPT_String path = url;
    if (!path.EndsWith("/")) path += "/";

    if (path.Left(7).Compare("upnp://", true) != 0) {
        return NULL;
    } else if (path.Compare("upnp://", true) == 0) {
        return "UPnP Media Servers (Auto-Discover)";
    }

    // look for nextslash
    int next_slash = path.Find('/', 7);
    if (next_slash == -1)
        return NULL;

    NPT_String uuid = path.SubString(7, next_slash-7);
    NPT_String object_id = path.SubString(next_slash+1, path.GetLength()-next_slash-2);

    // look for device
    PLT_DeviceDataReference* device;
    const NPT_Lock<PLT_DeviceMap>& devices = CUPnP::GetInstance()->m_MediaBrowser->GetMediaServers();
    if (NPT_FAILED(devices.Get(uuid, device)) || device == NULL)
        return NULL;

    return (const char*)(*device)->GetFriendlyName();
}

/*----------------------------------------------------------------------
|   CUPnPDirectory::GetDirectory
+---------------------------------------------------------------------*/
bool
CUPnPDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
    CGUIDialogProgress* dlgProgress = NULL;

    CUPnP* upnp = CUPnP::GetInstance();

    /* upnp should never be cached, it has internal cache */
    items.SetCacheToDisc(CFileItemList::CACHE_NEVER);

    // start client if it hasn't been done yet
    bool client_started = upnp->IsClientStarted();
    upnp->StartClient();

    // We accept upnp://devuuid/[item_id/]
    NPT_String path = strPath.c_str();
    if (!path.StartsWith("upnp://", true)) {
        return false;
    }

    if (path.Compare("upnp://", true) == 0) {
        // root -> get list of devices
        const NPT_Lock<PLT_DeviceMap>& devices = upnp->m_MediaBrowser->GetMediaServers();
        const NPT_List<PLT_DeviceMapEntry*>& entries = devices.GetEntries();
        NPT_List<PLT_DeviceMapEntry*>::Iterator entry = entries.GetFirstItem();
        while (entry) {
            PLT_DeviceDataReference device = (*entry)->GetValue();
            NPT_String name = device->GetFriendlyName();
            NPT_String uuid = (*entry)->GetKey();

            CFileItemPtr pItem(new CFileItem((const char*)name));
            pItem->m_strPath = (const char*) "upnp://" + uuid + "/";
            pItem->m_bIsFolder = true;
            pItem->SetThumbnailImage((const char*)device->GetIconUrl("image/jpeg"));

            items.Add(pItem);

            ++entry;
        }
    } else {
        if (!path.EndsWith("/")) path += "/";

        // look for nextslash
        int next_slash = path.Find('/', 7);

        NPT_String uuid = (next_slash==-1)?path.SubString(7):path.SubString(7, next_slash-7);
        NPT_String object_id = (next_slash==-1)?"":path.SubString(next_slash+1);
        object_id.TrimRight("/");
        if (object_id.GetLength()) {
            CStdString tmp = (char*) object_id;
            CUtil::UrlDecode(tmp);
            object_id = tmp;
        }

        // look for device in our list
        // (and wait for it to respond for 5 secs if we're just starting upnp client)
        NPT_TimeStamp watchdog;
        NPT_System::GetCurrentTimeStamp(watchdog);
        watchdog += 5.f;

        PLT_DeviceDataReference* device;
        for (;;) {
            const NPT_Lock<PLT_DeviceMap>& devices = upnp->m_MediaBrowser->GetMediaServers();
            if (NPT_SUCCEEDED(devices.Get(uuid, device)) && device)
                break;

            // fail right away if device not found and upnp client was already running
            if (client_started)
                goto failure;

            // otherwise check if we've waited long enough without success
            NPT_TimeStamp now;
            NPT_System::GetCurrentTimeStamp(now);
            if (now > watchdog)
                goto failure;

            // sleep a bit and try again
            NPT_System::Sleep(NPT_TimeInterval(1, 0));
        }

        // issue a browse request with object_id
        // if object_id is empty use "0" for root
        object_id = object_id.IsEmpty()?"0":object_id;

        // just a guess as to what types of files we want
        bool video = true;
        bool audio = true;
        bool image = true;
        m_strFileMask.TrimLeft("/");
        if (!m_strFileMask.IsEmpty()) {
            video = m_strFileMask.Find(".wmv") >= 0;
            audio = m_strFileMask.Find(".wma") >= 0;
            image = m_strFileMask.Find(".jpg") >= 0;
        }

        // special case for Windows Media Connect and WMP11 when looking for root
        // We can target which root subfolder we want based on directory mask
        if (object_id == "0" && (((*device)->GetFriendlyName().Find("Windows Media Connect", 0, true) >= 0) ||
                                 ((*device)->m_ModelName == "Windows Media Player Sharing"))) {

            // look for a specific type to differentiate which folder we want
            if (audio && !video && !image) {
                // music
                object_id = "1";
            } else if (!audio && video && !image) {
                // video
                object_id = "2";
            } else if (!audio && !video && image) {
                // pictures
                object_id = "3";
            }
        }

#ifdef DISABLE_SPECIALCASE
        // same thing but special case for XBMC
        if (object_id == "0" && (((*device)->m_ModelName.Find("XBMC", 0, true) >= 0) ||
                                 ((*device)->m_ModelName.Find("Xbox Media Center", 0, true) >= 0))) {
            // look for a specific type to differentiate which folder we want
            if (audio && !video && !image) {
                // music
                object_id = "virtualpath://upnpmusic";
            } else if (!audio && video && !image) {
                // video
                object_id = "virtualpath://upnpvideo";
            } else if (!audio && !video && image) {
                // pictures
                object_id = "virtualpath://upnppictures";
            }
        }
#endif
        // bring up dialog if object is not cached
        if (!upnp->m_MediaBrowser->IsCached(uuid, object_id)) {
            dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
            if (dlgProgress) {
                dlgProgress->ShowProgressBar(false);
                dlgProgress->SetCanCancel(false);
                dlgProgress->SetHeading(20334);
                dlgProgress->SetLine(0, 194);
                dlgProgress->SetLine(1, "");
                dlgProgress->SetLine(2, "");
                dlgProgress->StartModal();
            }
        }

        // if error, return now, the device could have gone away
        // this will make us go back to the sources list
        PLT_MediaObjectListReference list;
        NPT_Result res = upnp->m_MediaBrowser->Browse(*device, object_id, list);
        if (NPT_FAILED(res)) goto failure;

        // empty list is ok
        if (list.IsNull()) goto cleanup;

        PLT_MediaObjectList::Iterator entry = list->GetFirstItem();
        while (entry) {
            // disregard items with wrong class/type
            if( (!video && (*entry)->m_ObjectClass.type.CompareN("object.item.videoitem", 21,true) == 0)
             || (!audio && (*entry)->m_ObjectClass.type.CompareN("object.item.audioitem", 21,true) == 0)
             || (!image && (*entry)->m_ObjectClass.type.CompareN("object.item.imageitem", 21,true) == 0) )
            {
                ++entry;
                continue;
            }

            // never show empty containers in media views
            if((*entry)->IsContainer()) {
                if( (audio || video || image)
                 && ((PLT_MediaContainer*)(*entry))->m_ChildrenCount == 0) {
                    ++entry;
                    continue;
                }
            }

            CFileItemPtr pItem(new CFileItem((const char*)(*entry)->m_Title));
            pItem->SetLabelPreformated(true);
            pItem->m_bIsFolder = (*entry)->IsContainer();

            // if it's a container, format a string as upnp://uuid/object_id
            if (pItem->m_bIsFolder) {
                CStdString id = (char*) (*entry)->m_ObjectID;
                CUtil::URLEncode(id);
                pItem->m_strPath = (const char*) "upnp://" + uuid + "/" + id.c_str() + "/";
            } else {
                if ((*entry)->m_Resources.GetItemCount()) {
                    PLT_MediaItemResource& resource = (*entry)->m_Resources[0];

                    // look for a resource with "xbmc-get" protocol
                    // if we can't find one, keep the first resource
                    NPT_ContainerFind((*entry)->m_Resources,
                                      CProtocolFinder("xbmc-get"),
                                      resource);

                    CLog::Log(LOGDEBUG, "CUPnPDirectory::GetDirectory - resource protocol info '%s'", (const char*)(resource.m_ProtocolInfo));

                    // if it's an item, path is the first url to the item
                    // we hope the server made the first one reachable for us
                    // (it could be a format we dont know how to play however)
                    pItem->m_strPath = (const char*) resource.m_Uri;

                    // set metadata
                    if (resource.m_Size > 0) {
                        pItem->m_dwSize  = resource.m_Size;
                    }

                    // set a general content type
                    CStdString type = (const char*)(*entry)->m_ObjectClass.type.Left(21);
                    if     (type.Equals("object.item.videoitem"))
                        pItem->SetContentType("video/octet-stream");
                    else if(type.Equals("object.item.audioitem"))
                        pItem->SetContentType("audio/octet-stream");
                    else if(type.Equals("object.item.imageitem"))
                        pItem->SetContentType("image/octet-stream");

                    // look for content type in protocol info
                    if (resource.m_ProtocolInfo.GetLength()) {
                        char proto[1024];
                        char dummy1[1024];
                        char ct[1204];
                        char dummy2[1024];
                        int fields = sscanf(resource.m_ProtocolInfo, "%[^:]:%[^:]:%[^:]:%[^:]", proto, dummy1, ct, dummy2);
                        if (fields == 4) {
                            if (strcmp(ct, "application/octet-stream") != 0) {
                                pItem->SetContentType(ct);
                            }
                        } else {
                            CLog::Log(LOGERROR, "CUPnPDirectory::GetDirectory - invalid protocol info '%s'", (const char*)(resource.m_ProtocolInfo));
                        }
                    }

                    // look for date?
                    if((*entry)->m_Description.date.GetLength()) {
                        SYSTEMTIME time = {};
                        sscanf((*entry)->m_Description.date, "%hu-%hu-%huT%hu:%hu:%hu",
                               &time.wYear, &time.wMonth, &time.wDay, &time.wHour, &time.wMinute, &time.wSecond);
                        pItem->m_dateTime = time;
                    }

                    // look for metadata
                    if( (*entry)->m_ObjectClass.type.CompareN("object.item.videoitem", 21,true) == 0 ) {
                        pItem->SetLabelPreformated(false);
                        CVideoInfoTag* tag = pItem->GetVideoInfoTag();
                        CStdStringArray strings, strings2;
                        CStdString buffer;

                        tag->m_strTitle = (*entry)->m_Title;
                        tag->m_strGenre = JoinString((*entry)->m_Affiliation.genre, " / ");
                        tag->m_strDirector = (*entry)->m_People.director;
                        tag->m_strTagLine = (*entry)->m_Description.description;
                        tag->m_strPlot = (*entry)->m_Description.long_description;
                        tag->m_strRuntime.Format("%d",resource.m_Duration);
                    } else if( (*entry)->m_ObjectClass.type.CompareN("object.item.audioitem", 21,true) == 0 ) {
                        pItem->SetLabelPreformated(false);
                        CMusicInfoTag* tag = pItem->GetMusicInfoTag();
                        CStdStringArray strings;
                        CStdString buffer;

                        tag->SetDuration(resource.m_Duration);
                        tag->SetTitle((const char*) (*entry)->m_Title);
                        tag->SetGenre((const char*) JoinString((*entry)->m_Affiliation.genre, " / "));
                        tag->SetAlbum((const char*) (*entry)->m_Affiliation.album);
                        tag->SetTrackNumber((*entry)->m_MiscInfo.original_track_number);
                        if((*entry)->m_People.artists.GetItemCount()) {
                            tag->SetAlbumArtist((const char*)(*entry)->m_People.artists.GetFirstItem()->name);
                            tag->SetArtist((const char*)(*entry)->m_People.artists.GetFirstItem()->name);
                        } else {
                            tag->SetAlbumArtist((const char*)(*entry)->m_Creator);
                            tag->SetArtist((const char*)(*entry)->m_Creator);
                        }

                        tag->SetLoaded();
                    } else if( (*entry)->m_ObjectClass.type.CompareN("object.item.imageitem", 21,true) == 0 ) {
                      //CPictureInfoTag* tag = pItem->GetPictureInfoTag();
                    }
                }
            }

            // if there is a thumbnail available set it here
            if((*entry)->m_ExtraInfo.album_art_uri.GetLength())
                pItem->SetThumbnailImage((const char*) (*entry)->m_ExtraInfo.album_art_uri);
            else if((*entry)->m_Description.icon_uri.GetLength())
                pItem->SetThumbnailImage((const char*) (*entry)->m_Description.icon_uri);

            items.Add(pItem);

            ++entry;
        }
    }

cleanup:
    if (dlgProgress) dlgProgress->Close();
    return true;

failure:
    if (dlgProgress) dlgProgress->Close();
    return false;
}
}
