/*
* UPnP Support for XBox Media Center
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

#include "../stdafx.h"
#include "UPnPDirectory.h"
#include "../util.h"
#include "directorycache.h"

CUPnP g_UPnP;

CUPnP::CUPnP() : 
    m_Initted(false), 
    m_UPnP(NULL),
    m_MediaBrowser(NULL)
{
    PLT_SetLogLevel(PLT_LOG_LEVEL_4);
}

CUPnP::~CUPnP()
{
    delete m_MediaBrowser;
    delete m_UPnP;
}

void CUPnP::Init()
{
    if (m_Initted) return;

    m_UPnP = new PLT_UPnP(1900, false);
    m_CtrlPoint = new PLT_CtrlPoint();
    m_UPnP->AddCtrlPoint(m_CtrlPoint);
    m_UPnP->Start();
    m_MediaBrowser = new PLT_SyncMediaBrowser(m_CtrlPoint);

    m_Initted = true;
}

CUPnPDirectory::CUPnPDirectory(void)
{
} 

CUPnPDirectory::~CUPnPDirectory(void)
{
}

bool CUPnPDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
    bool was_initted = g_UPnP.IsInitted();
    if (!was_initted) g_UPnP.Init();
                     
    // We accept m_UPnP://server[:port[/devuuid[/path[/file]]]]
    CFileItemList vecCacheItems;
    g_directoryCache.ClearDirectory(strPath);

    CStdString strRoot = strPath;
    if (!CUtil::HasSlashAtEnd(strRoot)) strRoot += "/";

    // we need an url to do proper escaping 
    NPT_HttpUrl url(strRoot);
    NPT_String path = url.GetPath();

    if (path.IsEmpty()) {
        return false;
    } else if (path == "/") {
        // root ?         
        
        // always issue to 1900 just in case ssdp proxy is not running
        // If WMC is the only one running, it will respond to it
        if (url.GetPort() != 1900) {
            g_UPnP.m_CtrlPoint->Discover(NPT_HttpUrl(url.GetHost(), 1900, "*"), "upnp:rootdevice", 1);
        }

        // issue a search at the address/port specified 
        g_UPnP.m_CtrlPoint->Discover(NPT_HttpUrl(url.GetHost(), url.GetPort()?url.GetPort():1901, "*"), "upnp:rootdevice", 1);

        // wait a bit the first time to let devices respond
        // it's not guaranteed that it is enough time so users will have to go 
        // try again (going back up then again) if the device has not showed up
        // yet. If xbmc supports a way to refresh a view asynchronously, it would
        // be better...
        if (!was_initted) NPT_System::Sleep(NPT_TimeInterval(2, 0));

        // get list of devices 
        const NPT_Lock<PLT_DeviceMap>& devices = g_UPnP.m_MediaBrowser->GetMediaServers();
        const NPT_List<PLT_DeviceMapEntry*>& entries = devices.GetEntries();
        NPT_List<PLT_DeviceMapEntry*>::Iterator entry = entries.GetFirstItem();
        while (entry) {
            PLT_DeviceDataReference device = (*entry)->GetValue();
            NPT_String name   = device->GetFriendlyName();
            NPT_String uuid = (*entry)->GetKey();

            CFileItem *pItem = new CFileItem((const char*)name);
            pItem->m_strPath = (const char*) url.AsString() + uuid;
            pItem->m_bIsFolder = true;
            pItem->m_bIsShareOrDrive = true;
            pItem->m_iDriveType = SHARE_TYPE_REMOTE;

            if (!CUtil::HasSlashAtEnd(pItem->m_strPath)) pItem->m_strPath += '/';

            vecCacheItems.Add(pItem);
            items.Add(new CFileItem(*pItem));

            ++entry;
        }
    } else {
        // look for nextslash 
        int next_slash = path.Find('/', 1);
        if (next_slash == -1) 
            return false;

        NPT_String uuid = path.SubString(1, next_slash-1);
        NPT_String object_id = path.SubString(next_slash+1, path.GetLength()-next_slash-2);

        // look for device 
        PLT_DeviceDataReference* device;
        const NPT_Lock<PLT_DeviceMap>& devices = g_UPnP.m_MediaBrowser->GetMediaServers();
        if (NPT_FAILED(devices.Get(uuid, device)) || device == NULL) 
            return false;

        // issue a browse request with object id, if id is empty use root id = 0 
        NPT_String root_id = object_id.IsEmpty()?"0":object_id;

        // special case for Windows Media Connect
        // Since we know it is WMC, we can target which folder we want based on directory mask
        if (root_id == "0" && (*device)->GetFriendlyName().Find("Windows Media Connect", 0, true) >= 0) {
            // look for a specific type to differentiate which folder we want
            if (m_strFileMask.Find(".wma") >= 0) {
                // music
                root_id = "1";
            } else if (m_strFileMask.Find(".wmv") >= 0) {
                // video
                root_id = "2";
            } else if (m_strFileMask.Find(".jpg") >= 0) {
                // pictures
                root_id = "3";
            }
        }

        PLT_MediaItemListReference list;
        if (NPT_FAILED(g_UPnP.m_MediaBrowser->Browse(*device, root_id, list)))
            return false;

        NPT_List<PLT_MediaItem*>::Iterator entry = list->GetFirstItem();
        while (entry) {
            CFileItem *pItem = new CFileItem((const char*)(*entry)->m_Title);
            pItem->m_bIsFolder = (*entry)->IsContainer();
            pItem->m_bIsShareOrDrive = true;
            pItem->m_iDriveType = SHARE_TYPE_REMOTE;

            // if it's a container, format a string as upnp://host/uuid/object_id/ 
            if (pItem->m_bIsFolder) {
                pItem->m_strPath = (const char*) NPT_String("upnp://") + url.GetHost() + ":1900/" + uuid + "/" + (*entry)->m_ObjectID;
                if (!CUtil::HasSlashAtEnd(pItem->m_strPath)) pItem->m_strPath += '/';
            } else {
                if ((*entry)->m_Resources.GetItemCount()) {
                    // if http protocol, override url with upnp so that it triggers the use of PAPLAYER instead of MPLAYER
                    // somehow MPLAYER tries to http stream the wma even though the server doesn't support it.
                    if ((*entry)->m_Resources[0].m_Uri.Left(4).Compare("http", true) == 0) {
                        pItem->m_strPath = (const char*) NPT_String("upnp") + (*entry)->m_Resources[0].m_Uri.SubString(4);
                    } else {
                        pItem->m_strPath = (const char*) (*entry)->m_Resources[0].m_Uri;
                    }

                    if ((*entry)->m_Resources[0].m_Size > 0) {
                        pItem->m_dwSize  = (*entry)->m_Resources[0].m_Size;
                    }
                    pItem->m_musicInfoTag.SetDuration((*entry)->m_Resources[0].m_Duration);
                    pItem->m_musicInfoTag.SetGenre((const char*) (*entry)->m_Genre);
                    pItem->m_musicInfoTag.SetAlbum((const char*) (*entry)->m_Album);
                    
                    // some servers (like WMC) use upnp:artist instead of dc:creator
                    if ((*entry)->m_Creator.GetLength() == 0) {
                        pItem->m_musicInfoTag.SetArtist((const char*) (*entry)->m_Artist);
                    } else {
                        pItem->m_musicInfoTag.SetArtist((const char*) (*entry)->m_Creator);
                    }
                    pItem->m_musicInfoTag.SetTitle((const char*) (*entry)->m_Title);

                    // look for content type in protocol info
                    if ((*entry)->m_Resources[0].m_ProtocolInfo.GetLength()) {
                        char proto[1024];
                        char dummy1[1024];
                        char ct[1204];
                        char dummy2[1024];
                        int fields = sscanf((*entry)->m_Resources[0].m_ProtocolInfo, "%s:%s:%s:%s", proto, dummy1, ct, dummy2);
                        if (fields == 4) {
                            pItem->SetContentType(ct);
                        }
                    }
                    
                    pItem->SetLabelPreformated(true);
                    pItem->m_musicInfoTag.SetLoaded();

                    //TODO: figure out howto set the album art
                }
            }

            vecCacheItems.Add(pItem);
            items.Add(new CFileItem(*pItem));

            ++entry;
        }
    }

    if (m_cacheDirectory)
        g_directoryCache.SetDirectory(strPath, vecCacheItems);

    return true;
}

