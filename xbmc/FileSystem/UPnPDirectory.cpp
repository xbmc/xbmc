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
#include "../util.h"
#include "directorycache.h"

#include "UPnPDirectory.h"
#include "../lib/libUPnP/Platinum.h"
#include "../lib/libUPnP/PltMediaServer.h"
#include "../lib/libUPnP/PltMediaBrowser.h"
#include "../lib/libUPnP/PltSyncMediaBrowser.h"

DIRECTORY::CUPnP* DIRECTORY::CUPnP::upnp = NULL;

using namespace DIRECTORY;

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
|   CUPnP::CUPnP
+---------------------------------------------------------------------*/
CUPnP::CUPnP() :
    m_CtrlPointHolder(new CCtrlPointReferenceHolder())
{
    //PLT_SetLogLevel(PLT_LOG_LEVEL_4);
    m_UPnP = new PLT_UPnP(1900, false);
    m_CtrlPointHolder->m_CtrlPoint = new PLT_CtrlPoint();
    m_UPnP->AddCtrlPoint(m_CtrlPointHolder->m_CtrlPoint);
    m_UPnP->Start();

    m_MediaBrowser = new PLT_SyncMediaBrowser(m_CtrlPointHolder->m_CtrlPoint);

    // Issue a search request on the broadcast address instead of the upnp multicast address 239.255.255.250
    // since the xbox does not support multicast. UPnP devices will still respond to us
    // Repeat every 6 seconds
    m_CtrlPointHolder->m_CtrlPoint->Discover(NPT_HttpUrl("255.255.255.255", 1900, "*"), "upnp:rootdevice", 1, 6000);
}

/*----------------------------------------------------------------------
|   CUPnP::~CUPnP
+---------------------------------------------------------------------*/
CUPnP::~CUPnP()
{
    m_UPnP->Stop();
    delete m_UPnP;
    delete m_MediaBrowser;
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
CUPnP::ReleaseInstance()
{
    if (upnp) {
        // since it takes a while to clean up
        // starts a detached thread to do this
        CUPnPCleaner* cleaner = new CUPnPCleaner(upnp);
        cleaner->Start();
        upnp = NULL;
    }
}

/*----------------------------------------------------------------------
|   CUPnP::IgnoreUUID
+---------------------------------------------------------------------*/
void
CUPnP::IgnoreUUID(const char* uuid)
{
    m_CtrlPointHolder->m_CtrlPoint->SetUUIDToIgnore(uuid);
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
    CUPnP* upnp = CUPnP::GetInstance();
                     
    CFileItemList vecCacheItems;
    g_directoryCache.ClearDirectory(strPath);

    // We accept upnp://devuuid[/path[/file]]]]
    // make sure we have a slash to look for at the end
    CStdString strRoot = strPath;
    if (!CUtil::HasSlashAtEnd(strRoot)) strRoot += "/";

    NPT_String path = strPath.c_str();

    if (path.Left(7).Compare("upnp://", true) != 0) {
        return false;
    } else if (path.Compare("upnp://", true) == 0) {
        // root -> get list of devices 
        const NPT_Lock<PLT_DeviceMap>& devices = upnp->m_MediaBrowser->GetMediaServers();
        const NPT_List<PLT_DeviceMapEntry*>& entries = devices.GetEntries();
        NPT_List<PLT_DeviceMapEntry*>::Iterator entry = entries.GetFirstItem();
        while (entry) {
            PLT_DeviceDataReference device = (*entry)->GetValue();
            NPT_String name   = device->GetFriendlyName();
            NPT_String uuid = (*entry)->GetKey();

            CFileItem *pItem = new CFileItem((const char*)name);
            pItem->m_strPath = (const char*) path + uuid;
            pItem->m_bIsFolder = true;
            //pItem->m_bIsShareOrDrive = true;
            //pItem->m_iDriveType = SHARE_TYPE_REMOTE;

            if (!CUtil::HasSlashAtEnd(pItem->m_strPath)) pItem->m_strPath += '/';

            vecCacheItems.Add(pItem);
            items.Add(new CFileItem(*pItem));

            ++entry;
        }
    } else {
        // look for nextslash 
        int next_slash = path.Find('/', 7);
        if (next_slash == -1) 
            return false;

        NPT_String uuid = path.SubString(7, next_slash-7);
        NPT_String object_id = path.SubString(next_slash+1, path.GetLength()-next_slash-2);

        // look for device 
        PLT_DeviceDataReference* device;
        const NPT_Lock<PLT_DeviceMap>& devices = upnp->m_MediaBrowser->GetMediaServers();
        if (NPT_FAILED(devices.Get(uuid, device)) || device == NULL) 
            return false;

        // issue a browse request with object id, if id is empty use root id = 0 
        NPT_String root_id = object_id.IsEmpty()?"0":object_id;

        // just a guess as to what types of files we want */
        bool video = true;
        bool audio = true;
        bool image = true;
        if( !m_strFileMask.IsEmpty() ) {
          video = m_strFileMask.Find(".wmv") >= 0;
          audio = m_strFileMask.Find(".wma") >= 0;
          image = m_strFileMask.Find(".jpg") >= 0;
        }

        // special case for Windows Media Connect
        // Since we know it is WMC, we can target which folder we want based on directory mask
        if (root_id == "0" 
         && ( (*device)->GetFriendlyName().Find("Windows Media Connect", 0, true) >= 0
           || (*device)->m_ModelName == "Windows Media Player Sharing")) {

            // look for a specific type to differentiate which folder we want
            if (audio && !video && !image) {
                // music
                root_id = "1";
            } else if (!audio && video && !image) {
                // video
                root_id = "2";
            } else if (!audio && !video && image) {
                // pictures
                root_id = "3";
            }
        }

        // if error, the list could be partial and that's ok
        // we still want to process it
        PLT_MediaObjectListReference list;
        upnp->m_MediaBrowser->Browse(*device, root_id, list);
        PLT_MediaObjectList::Iterator entry = list->GetFirstItem();
        while (entry) {

            if( (!video && (*entry)->m_ObjectClass.type.CompareN("object.item.videoitem", 21,true) == 0)
             || (!audio && (*entry)->m_ObjectClass.type.CompareN("object.item.audioitem", 21,true) == 0)
             || (!image && (*entry)->m_ObjectClass.type.CompareN("object.item.imageitem", 21,true) == 0) )
            {
                ++entry;
                continue;
            }

            CFileItem *pItem = new CFileItem((const char*)(*entry)->m_Title);
            pItem->m_bIsFolder = (*entry)->IsContainer();

            // if it's a container, format a string as upnp://host/uuid/object_id/ 
            if (pItem->m_bIsFolder) {
                pItem->m_strPath = (const char*) NPT_String("upnp://") + uuid + "/" + (*entry)->m_ObjectID;
                if (!CUtil::HasSlashAtEnd(pItem->m_strPath)) pItem->m_strPath += '/';

            } else {
                if ((*entry)->m_Resources.GetItemCount()) {
                    // if http protocol, override url with upnp so that it triggers the use of PAPLAYER instead of MPLAYER
                    // somehow MPLAYER tries to http stream the wma even though the server doesn't support it.
                    //if ((*entry)->m_Resources[0].m_Uri.Left(4).Compare("http", true) == 0) {
                    //    pItem->m_strPath = (const char*) NPT_String("upnp") + (*entry)->m_Resources[0].m_Uri.SubString(4);
                    //} else {
                        pItem->m_strPath = (const char*) (*entry)->m_Resources[0].m_Uri;
                    //}

                    if ((*entry)->m_Resources[0].m_Size > 0) {
                        pItem->m_dwSize  = (*entry)->m_Resources[0].m_Size;
                    }
                    pItem->m_musicInfoTag.SetDuration((*entry)->m_Resources[0].m_Duration);
                    pItem->m_musicInfoTag.SetGenre((const char*) (*entry)->m_Affiliation.genre);
                    pItem->m_musicInfoTag.SetAlbum((const char*) (*entry)->m_Affiliation.album);
                    
                    // some servers (like WMC) use upnp:artist instead of dc:creator
                    if ((*entry)->m_Creator.GetLength() == 0) {
                        pItem->m_musicInfoTag.SetArtist((const char*) (*entry)->m_People.artist);
                    } else {
                        pItem->m_musicInfoTag.SetArtist((const char*) (*entry)->m_Creator);
                    }
                    pItem->m_musicInfoTag.SetTitle((const char*) (*entry)->m_Title);                    
                    pItem->m_musicInfoTag.SetLoaded();

                    // look for content type in protocol info
                    if ((*entry)->m_Resources[0].m_ProtocolInfo.GetLength()) {
                        char proto[1024];
                        char dummy1[1024];
                        char ct[1204];
                        char dummy2[1024];
                        int fields = sscanf((*entry)->m_Resources[0].m_ProtocolInfo, "%[^:]:%[^:]:%[^:]:%[^:]", proto, dummy1, ct, dummy2);
                        if (fields == 4) {
                            pItem->SetContentType(ct);
                        }
                    }

                    //TODO, current thumbnail and icon of CFileItem is expected to be a local
                    //      image file in the normal thumbnail directories, we have no way to
                    //      specify an external filename on the filelayer
                    //if((*entry)->m_ExtraInfo.album_art_uri.GetLength())
                    //  pItem->SetThumbnailImage((const char*) (*entry)->m_ExtraInfo.album_art_uri);
                    //else
                    //  pItem->SetThumbnailImage((const char*) (*entry)->m_Description.icon_uri);

                    if( (*entry)->m_Description.date.GetLength() )
                    {
                      SYSTEMTIME time = {};
                      int count = sscanf((*entry)->m_Description.date, "%hu-%hu-%huT%hu:%hu:%hu",
                                          &time.wYear, &time.wMonth, &time.wDay, &time.wHour, &time.wMinute, &time.wSecond);
                      pItem->m_dateTime = time;
                    }
                    
                }
            }

            pItem->SetLabelPreformated(true);
            vecCacheItems.Add(pItem);
            items.Add(new CFileItem(*pItem));

            ++entry;
        }
    }

    if (m_cacheDirectory)
        g_directoryCache.SetDirectory(strPath, vecCacheItems);

    return true;
}

