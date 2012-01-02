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

#include "UPnPDirectory.h"
#include "URL.h"
#include "network/UPnP.h"
#include "Platinum.h"
#include "PltSyncMediaBrowser.h"
#include "video/VideoInfoTag.h"
#include "FileItem.h"
#include "utils/log.h"

using namespace MUSIC_INFO;
using namespace XFILE;

namespace XFILE
{
/*----------------------------------------------------------------------
|   CProtocolFinder
+---------------------------------------------------------------------*/
class CProtocolFinder {
public:
    CProtocolFinder(const char* protocol) : m_Protocol(protocol) {}
    bool operator()(const PLT_MediaItemResource& resource) const {
        return (resource.m_ProtocolInfo.ToString().Compare(m_Protocol, true) == 0);
    }
private:
    NPT_String m_Protocol;
};

static CStdString GetContentMapping(NPT_String& objectClass)
{
    struct SClassMapping
    {
        const char* ObjectClass;
        const char* Content;
    };
    static const SClassMapping mapping[] = {
          { "object.item.videoItem.videoBroadcast", "episodes"      }
        , { "object.item.videoItem.musicVideoClip", "musicvideos"  }
        , { "object.item.videoItem"               , "movies"       }
        , { "object.item.audioItem.musicTrack"    , "songs"        }
        , { "object.item.audioItem"               , "songs"        }
        , { "object.item.imageItem.photo"         , "photos"       }
        , { "object.item.imageItem"               , "photos"       }
        , { "object.container.album.videoAlbum"   , "tvshows"      }
        , { "object.container.album.musicAlbum"   , "albums"       }
        , { "object.container.album.photoAlbum"   , "photos"       }
        , { "object.container.album"              , "albums"       }
        , { "object.container.person"             , "artists"      }
        , { NULL                                  , NULL           }
    };
    for(const SClassMapping* map = mapping; map->ObjectClass; map++)
    {
        if(objectClass.StartsWith(map->ObjectClass, true))
        {
          return map->Content;
          break;
        }
    }
    return "unknown";
}

static bool FindDeviceWait(CUPnP* upnp, const char* uuid, PLT_DeviceDataReference& device)
{
    bool client_started = upnp->IsClientStarted();
    upnp->StartClient();

    // look for device in our list
    // (and wait for it to respond for 5 secs if we're just starting upnp client)
    NPT_TimeStamp watchdog;
    NPT_System::GetCurrentTimeStamp(watchdog);
    watchdog += 5.f;

    for (;;) {
        if (NPT_SUCCEEDED(upnp->m_MediaBrowser->FindServer(uuid, device)) && !device.IsNull())
            break;

        // fail right away if device not found and upnp client was already running
        if (client_started)
            return false;

        // otherwise check if we've waited long enough without success
        NPT_TimeStamp now;
        NPT_System::GetCurrentTimeStamp(now);
        if (now > watchdog)
            return false;

        // sleep a bit and try again
        NPT_System::Sleep(NPT_TimeInterval(1, 0));
    }

    return !device.IsNull();
}

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
    PLT_DeviceDataReference device;
    if(!FindDeviceWait(CUPnP::GetInstance(), uuid, device))
        return NULL;

    return (const char*)device->GetFriendlyName();
}

/*----------------------------------------------------------------------
|   CUPnPDirectory::GetDirectory
+---------------------------------------------------------------------*/
bool CUPnPDirectory::GetResource(const CURL& path, CFileItem &item)
{
    if(path.GetProtocol() != "upnp")
      return false;

    CUPnP* upnp = CUPnP::GetInstance();
    if(!upnp)
        return false;

    CStdString uuid   = path.GetHostName();
    CStdString object = path.GetFileName();
    object.TrimRight("/");
    CURL::Decode(object);

    PLT_DeviceDataReference device;
    if(!FindDeviceWait(upnp, uuid.c_str(), device))
        return false;

    PLT_MediaObjectListReference list;
    if (NPT_FAILED(upnp->m_MediaBrowser->BrowseSync(device, object.c_str(), list, true)))
        return false;

    PLT_MediaObjectList::Iterator entry = list->GetFirstItem();
    if (entry == 0)
        return false;

    PLT_MediaItemResource resource;

    // look for a resource with "xbmc-get" protocol
    // if we can't find one, keep the first resource
    if(NPT_FAILED(NPT_ContainerFind((*entry)->m_Resources,
                      CProtocolFinder("xbmc-get"), resource))) {
        if((*entry)->m_Resources.GetItemCount())
            resource = (*entry)->m_Resources[0];
        else
            return false;
    }

    // store original path so we remember it
    item.SetProperty("original_listitem_url",  item.GetPath());
    item.SetProperty("original_listitem_mime", item.GetMimeType(false));

    // if it's an item, path is the first url to the item
    // we hope the server made the first one reachable for us
    // (it could be a format we dont know how to play however)
    item.SetPath((const char*) resource.m_Uri);

    // look for content type in protocol info
    if (resource.m_ProtocolInfo.IsValid()) {
        CLog::Log(LOGDEBUG, "CUPnPDirectory::GetResource - resource protocol info '%s'",
            (const char*)(resource.m_ProtocolInfo.ToString()));

        if (resource.m_ProtocolInfo.GetContentType().Compare("application/octet-stream") != 0) {
            item.SetMimeType((const char*)resource.m_ProtocolInfo.GetContentType());
        }
    } else {
        CLog::Log(LOGERROR, "CUPnPDirectory::GetResource - invalid protocol info '%s'",
            (const char*)(resource.m_ProtocolInfo.ToString()));
    }

    // look for subtitles
    unsigned subs = 0;
    for(unsigned r = 0; r < (*entry)->m_Resources.GetItemCount(); r++)
    {
        PLT_MediaItemResource& res  = (*entry)->m_Resources[r];
        PLT_ProtocolInfo&      info = res.m_ProtocolInfo;
        static const char* allowed[] = { "text/srt"
                                       , "text/ssa"
                                       , "text/sub"
                                       , "text/idx" };
        for(unsigned type = 0; type < sizeof(allowed)/sizeof(allowed[0]); type++)
        {
            if(info.Match(PLT_ProtocolInfo("*", "*", allowed[type], "*")))
            {
                CStdString prop;
                prop.Format("upnp:subtitle:%d", ++subs);
                item.SetProperty(prop, (const char*)res.m_Uri);
                break;
            }
        }
    }

    return true;
}


/*----------------------------------------------------------------------
|   CUPnPDirectory::GetDirectory
+---------------------------------------------------------------------*/
bool
CUPnPDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
    CUPnP* upnp = CUPnP::GetInstance();

    /* upnp should never be cached, it has internal cache */
    items.SetCacheToDisc(CFileItemList::CACHE_NEVER);

    // We accept upnp://devuuid/[item_id/]
    NPT_String path = strPath.c_str();
    if (!path.StartsWith("upnp://", true)) {
        return false;
    }

    if (path.Compare("upnp://", true) == 0) {
        upnp->StartClient();

        // root -> get list of devices
        const NPT_Lock<PLT_DeviceDataReferenceList>& devices = upnp->m_MediaBrowser->GetMediaServers();
        NPT_List<PLT_DeviceDataReference>::Iterator device = devices.GetFirstItem();
        while (device) {
            NPT_String name = (*device)->GetFriendlyName();
            NPT_String uuid = (*device)->GetUUID();

            CFileItemPtr pItem(new CFileItem((const char*)name));
            pItem->SetPath(CStdString((const char*) "upnp://" + uuid + "/"));
            pItem->m_bIsFolder = true;
            pItem->SetThumbnailImage((const char*)(*device)->GetIconUrl("image/jpeg"));

            items.Add(pItem);

            ++device;
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
            CURL::Decode(tmp);
            object_id = tmp;
        }

        // try to find the device with wait on startup
        PLT_DeviceDataReference device;
        if (!FindDeviceWait(upnp, uuid, device))
            goto failure;

        // issue a browse request with object_id
        // if object_id is empty use "0" for root
        object_id = object_id.IsEmpty()?"0":object_id;

        // remember a count of object classes
        std::map<NPT_String, int> classes;

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
        if (object_id == "0" && ((device->GetFriendlyName().Find("Windows Media Connect", 0, true) >= 0) ||
                                 (device->m_ModelName == "Windows Media Player Sharing"))) {

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
        if (object_id == "0" && ((device->m_ModelName.Find("XBMC", 0, true) >= 0) ||
                                 (device->m_ModelName.Find("Xbox Media Center", 0, true) >= 0))) {
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

        // if error, return now, the device could have gone away
        // this will make us go back to the sources list
        PLT_MediaObjectListReference list;
        NPT_Result res = upnp->m_MediaBrowser->BrowseSync(device, object_id, list);
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

            NPT_String ObjectClass = (*entry)->m_ObjectClass.type.ToLowercase();

            // keep count of classes
            classes[(*entry)->m_ObjectClass.type]++;

            CFileItemPtr pItem(new CFileItem((const char*)(*entry)->m_Title));
            pItem->SetLabelPreformated(true);
            pItem->m_strTitle = (const char*)(*entry)->m_Title;
            pItem->m_bIsFolder = (*entry)->IsContainer();

            CStdString id = (char*) (*entry)->m_ObjectID;
            CURL::Encode(id);
            pItem->SetPath(CStdString((const char*) "upnp://" + uuid + "/" + id.c_str()));

            // if it's a container, format a string as upnp://uuid/object_id
            if (pItem->m_bIsFolder) {
                pItem->SetPath(pItem->GetPath() + "/");

                // look for metadata
                if( ObjectClass.StartsWith("object.container.album.videoalbum") ) {
                    pItem->SetLabelPreformated(false);
                    CUPnP::PopulateTagFromObject(*pItem->GetVideoInfoTag(), *(*entry), NULL);

                } else if( ObjectClass.StartsWith("object.container.album.photoalbum")) {
                  //CPictureInfoTag* tag = pItem->GetPictureInfoTag();

                } else if( ObjectClass.StartsWith("object.container.album") ) {
                    pItem->SetLabelPreformated(false);
                    CUPnP::PopulateTagFromObject(*pItem->GetMusicInfoTag(), *(*entry), NULL);
                }

            } else {

                // set a general content type
                if (ObjectClass.StartsWith("object.item.videoitem"))
                    pItem->SetMimeType("video/octet-stream");
                else if(ObjectClass.StartsWith("object.item.audioitem"))
                    pItem->SetMimeType("audio/octet-stream");
                else if(ObjectClass.StartsWith("object.item.imageitem"))
                    pItem->SetMimeType("image/octet-stream");

                if ((*entry)->m_Resources.GetItemCount()) {
                    PLT_MediaItemResource& resource = (*entry)->m_Resources[0];

                    // set metadata
                    if (resource.m_Size != (NPT_LargeSize)-1) {
                        pItem->m_dwSize  = resource.m_Size;
                    }

                    // look for metadata
                    if( ObjectClass.StartsWith("object.item.videoitem") ) {
                        pItem->SetLabelPreformated(false);
                        CUPnP::PopulateTagFromObject(*pItem->GetVideoInfoTag(), *(*entry), &resource);

                    } else if( ObjectClass.StartsWith("object.item.audioitem") ) {
                        pItem->SetLabelPreformated(false);
                        CUPnP::PopulateTagFromObject(*pItem->GetMusicInfoTag(), *(*entry), &resource);

                    } else if( ObjectClass.StartsWith("object.item.imageitem") ) {
                      //CPictureInfoTag* tag = pItem->GetPictureInfoTag();

                    }
                }
            }

            // look for date?
            if((*entry)->m_Description.date.GetLength()) {
                SYSTEMTIME time = {};
                sscanf((*entry)->m_Description.date, "%hu-%hu-%huT%hu:%hu:%hu",
                       &time.wYear, &time.wMonth, &time.wDay, &time.wHour, &time.wMinute, &time.wSecond);
                pItem->m_dateTime = time;
            }

            // if there is a thumbnail available set it here
            if((*entry)->m_ExtraInfo.album_art_uri.GetLength())
                pItem->SetThumbnailImage((const char*) (*entry)->m_ExtraInfo.album_art_uri);
            else if((*entry)->m_Description.icon_uri.GetLength())
                pItem->SetThumbnailImage((const char*) (*entry)->m_Description.icon_uri);

            PLT_ProtocolInfo fanart_mask("xbmc.org", "*", "fanart", "*");
            for(unsigned i = 0; i < (*entry)->m_Resources.GetItemCount(); ++i) {
                PLT_MediaItemResource& res = (*entry)->m_Resources[i];
                if(res.m_ProtocolInfo.Match(fanart_mask)) {
                    pItem->SetProperty("fanart_image", (const char*)res.m_Uri);
                    break;
                }
            }
            items.Add(pItem);

            ++entry;
        }

        NPT_String max_string = "";
        int        max_count  = 0;
        for(std::map<NPT_String, int>::iterator it = classes.begin(); it != classes.end(); it++)
        {
          if(it->second > max_count)
          {
            max_string = it->first;
            max_count  = it->second;
          }
        }
        items.SetContent(GetContentMapping(max_string));
    }

cleanup:
    return true;

failure:
    return false;
}
}
