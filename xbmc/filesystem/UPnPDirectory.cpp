/*
 * UPnP Support for XBMC
 *  Copyright (c) 2006 c0diq (Sylvain Rebaud)
 *      Portions Copyright (c) by the authors of libPlatinum
 *      http://www.plutinosoft.com/blog/category/platinum/
 *
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "UPnPDirectory.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "network/upnp/UPnP.h"
#include "network/upnp/UPnPInternal.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <Platinum/Source/Devices/MediaServer/PltSyncMediaBrowser.h>
#include <Platinum/Source/Platinum/Platinum.h>

using namespace MUSIC_INFO;
using namespace XFILE;
using namespace UPNP;

namespace XFILE
{

static std::string GetContentMapping(NPT_String& objectClass)
{
    struct SClassMapping
    {
        const char* ObjectClass;
        const char* Content;
    };
    static const SClassMapping mapping[] = {
          { "object.item.videoItem.videoBroadcast"                  , "episodes"      }
        , { "object.item.videoItem.musicVideoClip"                  , "musicvideos"  }
        , { "object.item.videoItem"                                 , "movies"       }
        , { "object.item.audioItem.musicTrack"                      , "songs"        }
        , { "object.item.audioItem"                                 , "songs"        }
        , { "object.item.imageItem.photo"                           , "photos"       }
        , { "object.item.imageItem"                                 , "photos"       }
        , { "object.container.album.videoAlbum.videoBroadcastShow"  , "tvshows"      }
        , { "object.container.album.videoAlbum.videoBroadcastSeason", "seasons"      }
        , { "object.container.album.musicAlbum"                     , "albums"       }
        , { "object.container.album.photoAlbum"                     , "photos"       }
        , { "object.container.album"                                , "albums"       }
        , { "object.container.person"                               , "artists"      }
        , { NULL                                                    , NULL           }
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
    watchdog += 5.0;

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
        NPT_System::Sleep(NPT_TimeInterval((double)1));
    }

    return !device.IsNull();
}

/*----------------------------------------------------------------------
|   CUPnPDirectory::GetFriendlyName
+---------------------------------------------------------------------*/
std::string CUPnPDirectory::GetFriendlyName(const CURL& url)
{
    NPT_String path = url.Get().c_str();
    if (!path.EndsWith("/")) path += "/";

    if (path.Left(7).Compare("upnp://", true) != 0) {
      return {};
    } else if (path.Compare("upnp://", true) == 0) {
        return "UPnP Media Servers (Auto-Discover)";
    }

    // look for nextslash
    int next_slash = path.Find('/', 7);
    if (next_slash == -1)
      return {};

    NPT_String uuid = path.SubString(7, next_slash-7);
    NPT_String object_id = path.SubString(next_slash+1, path.GetLength()-next_slash-2);

    // look for device
    PLT_DeviceDataReference device;
    if(!FindDeviceWait(CUPnP::GetInstance(), uuid, device))
      return {};

    return device->GetFriendlyName().GetChars();
}

/*----------------------------------------------------------------------
|   CUPnPDirectory::GetDirectory
+---------------------------------------------------------------------*/
bool CUPnPDirectory::GetResource(const CURL& path, CFileItem &item)
{
    if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_SERVICES_UPNP))
      return false;

    if(!path.IsProtocol("upnp"))
      return false;

    CUPnP* upnp = CUPnP::GetInstance();
    if(!upnp)
        return false;

    const std::string& uuid = path.GetHostName();
    std::string object = path.GetFileName();
    StringUtils::TrimRight(object, "/");
    object = CURL::Decode(object);

    PLT_DeviceDataReference device;
    if(!FindDeviceWait(upnp, uuid.c_str(), device)) {
      CLog::Log(LOGERROR, "CUPnPDirectory::GetResource - unable to find uuid {}", uuid);
      return false;
    }

    PLT_MediaObjectListReference list;
    if (NPT_FAILED(upnp->m_MediaBrowser->BrowseSync(device, object.c_str(), list, true))) {
      CLog::Log(LOGERROR, "CUPnPDirectory::GetResource - unable to find object {}", object);
      return false;
    }

    if (list.IsNull() || !list->GetItemCount()) {
      CLog::Log(LOGERROR, "CUPnPDirectory::GetResource - no items returned for object {}", object);
      return false;
    }

    PLT_MediaObjectList::Iterator entry = list->GetFirstItem();
    if (entry == 0)
        return false;

  return UPNP::GetResource(*entry, item);
}


/*----------------------------------------------------------------------
|   CUPnPDirectory::GetDirectory
+---------------------------------------------------------------------*/
bool
CUPnPDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
    if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_SERVICES_UPNP))
      return false;

    CUPnP* upnp = CUPnP::GetInstance();

    /* upnp should never be cached, it has internal cache */
    items.SetCacheToDisc(CFileItemList::CACHE_NEVER);

    // We accept upnp://devuuid/[item_id/]
    NPT_String path = url.Get().c_str();
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
            pItem->SetPath(std::string((const char*) "upnp://" + uuid + "/"));
            pItem->m_bIsFolder = true;
            pItem->SetArt("thumb", (const char*)(*device)->GetIconUrl("image/png"));

            items.Add(pItem);

            ++device;
        }
    } else {
        if (!path.EndsWith("/")) path += "/";

        // look for nextslash
        int next_slash = path.Find('/', 7);

        NPT_String uuid = (next_slash==-1)?path.SubString(7):path.SubString(7, next_slash-7);
        NPT_String object_id = (next_slash == -1) ? NPT_String("") : path.SubString(next_slash + 1);
        object_id.TrimRight("/");
        if (object_id.GetLength()) {
            object_id = CURL::Decode((char*)object_id).c_str();
        }

        // try to find the device with wait on startup
        PLT_DeviceDataReference device;
        if (!FindDeviceWait(upnp, uuid, device))
            goto failure;

        // issue a browse request with object_id
        // if object_id is empty use "0" for root
        object_id = object_id.IsEmpty() ? NPT_String("0") : object_id;

        // remember a count of object classes
        std::map<NPT_String, int> classes;

        // just a guess as to what types of files we want
        bool video = true;
        bool audio = true;
        bool image = true;
        StringUtils::TrimLeft(m_strFileMask, "/");
        if (!m_strFileMask.empty()) {
            video = m_strFileMask.find(".wmv") != std::string::npos;
            audio = m_strFileMask.find(".wma") != std::string::npos;
            image = m_strFileMask.find(".jpg") != std::string::npos;
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

            // keep count of classes
            classes[(*entry)->m_ObjectClass.type]++;
            CFileItemPtr pItem = BuildObject(*entry, UPnPClient);
            if(!pItem) {
                ++entry;
                continue;
            }

            std::string id;
            if ((*entry)->m_ReferenceID.IsEmpty())
                id = (const char*) (*entry)->m_ObjectID;
            else
                id = (const char*) (*entry)->m_ReferenceID;

            id = CURL::Encode(id);
            URIUtils::AddSlashAtEnd(id);
            pItem->SetPath(std::string((const char*) "upnp://" + uuid + "/" + id.c_str()));

            items.Add(pItem);

            ++entry;
        }

        NPT_String max_string = "";
        int        max_count  = 0;
        for (auto& it : classes)
        {
          if (it.second > max_count)
          {
            max_string = it.first;
            max_count = it.second;
          }
        }
        std::string content = GetContentMapping(max_string);
        items.SetContent(content);
        if (content == "unknown")
        {
          items.AddSortMethod(SortByNone, 571, LABEL_MASKS("%L", "%I", "%L", ""));
          items.AddSortMethod(SortByLabel, SortAttributeIgnoreFolders, 551, LABEL_MASKS("%L", "%I", "%L", ""));
          items.AddSortMethod(SortBySize, 553, LABEL_MASKS("%L", "%I", "%L", "%I"));
          items.AddSortMethod(SortByDate, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));
        }
    }

cleanup:
    return true;

failure:
    return false;
}
}

bool CUPnPDirectory::Resolve(CFileItem& item) const
{
  return GetResource(item.GetURL(), item);
}
