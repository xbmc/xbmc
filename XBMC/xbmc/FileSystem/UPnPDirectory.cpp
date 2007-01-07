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

#include "VirtualPathDirectory.h"
#include "UPnPDirectory.h"
#include "../lib/libUPnP/Platinum.h"
#include "../lib/libUPnP/PltFileMediaServer.h"
#include "../lib/libUPnP/PltMediaServer.h"
#include "../lib/libUPnP/PltMediaBrowser.h"
#include "../lib/libUPnP/PltSyncMediaBrowser.h"
#include "../lib/libUPnP/PltDidl.h"

CUPnP* CUPnP::upnp = NULL;

/*----------------------------------------------------------------------
|   CUPnPVirtualPathDirectory class
+---------------------------------------------------------------------*/
class CUPnPVirtualPathDirectory : public CVirtualPathDirectory 
{
public:

    static bool FindSharePath(const char* share_name, const char* path, bool begin = false) {
        CFileItemList items;
        CUPnPVirtualPathDirectory dir;
        if (!dir.GetDirectory((const char*)share_name, items)) {
            return false;
        }

        for (int i = 0; i < items.Size(); i++) {
            if (begin) {
                if (NPT_StringsEqualN(path, items[i]->m_strPath.c_str(), NPT_StringLength(items[i]->m_strPath.c_str()))) {
                    return true;
                }
            } else if (NPT_StringsEqual(path, items[i]->m_strPath.c_str())) {
                return true;
            }
        }

        return false;
    }

    static bool SplitPath(const char* object_id, NPT_String& share_name, NPT_String& path) {
        int index = 0;
        NPT_String id = object_id;

        // reset output params first
        share_name = "";
        path = "";

        if (id.StartsWith("virtualpath://music")) {
            index = 19;
        } else if (id.StartsWith("virtualpath://video")) {
            index = 19;
        } else if (id.StartsWith("virtualpath://pictures")) {
            index = 22;
        } else {
            return false;
        }

        // nothing to split
        if (id.GetLength() <= (NPT_Cardinal)index) {
            return true;
        }

        // invalid id!
        if (id[index] != '/') {
            return false;
        }

        // look for share
        index = id.Find('/', index+1);
        share_name = id.SubString(0, (index==-1)?id.GetLength():index);

        if (index >= 0) {
            path = id.SubString(index+1);
        }

        return true;
    }

    bool GetDirectory(const CStdString& strPath, CFileItemList &items) {
        NPT_String path = strPath.c_str();
        CShare     share;
        CFileItem* item;

        if (path == "0") {
            // music
            item = new CFileItem("virtualpath://music", true);
            item->SetLabel("Music");
            items.Add(item);

            // video
            item = new CFileItem("virtualpath://video", true);
            item->SetLabel("Video");
            items.Add(item);

            // pictures
            item = new CFileItem("virtualpath://pictures", true);
            item->SetLabel("Pictures");
            items.Add(item);

            return true;
        } else if (path == "virtualpath://music" || 
                   path == "virtualpath://video" || 
                   path == "virtualpath://pictures") {
            // look for all shares given a container
            VECSHARES *shares = NULL;
            if (path == "virtualpath://music") {
                shares = g_settings.GetSharesFromType("music");
            } else if (path == "virtualpath://video") {
                shares = g_settings.GetSharesFromType("video");
            } else if (path == "virtualpath://pictures") {
                shares = g_settings.GetSharesFromType("pictures");
            }
            if (shares) {
                for (unsigned int i = 0; i < shares->size(); i++) {
                    // Does this share contains any local paths?
                    CShare &share = shares->at(i);
                    if (GetMatchingShare(share.strPath, share) && share.vecPaths.size()) {
                        item = new CFileItem(share.strPath, true);
                        item->SetLabel(share.strName);
                        items.Add(item);
                    }
                }
            }
            
            return true;
        } else if (!GetMatchingShare((const char*)path, share)) {
            // split to remove share name from path
            NPT_String share_name;
            NPT_String file_path;
            bool bret = SplitPath(path, share_name, file_path);
            if (!bret || share_name.GetLength() == 0 || file_path.GetLength() == 0) {
                return false;
            }

            // make sure the file_path is the beginning of a share paths
            if (!FindSharePath(share_name, file_path, true)) return false;

            // use the share name to figure out what extensions to use
            if (share_name.StartsWith("virtualpath://music")) {
                return CDirectory::GetDirectory(
                    (const char*)file_path, 
                    items, 
                    g_stSettings.m_musicExtensions);
            } else if (share_name.StartsWith("virtualpath://video")) {
                return CDirectory::GetDirectory(
                    (const char*)file_path, 
                    items, 
                    g_stSettings.m_videoExtensions);
            } else if (share_name.StartsWith("virtualpath://pictures")) {
                return CDirectory::GetDirectory(
                    (const char*)file_path, 
                    items, 
                    g_stSettings.m_pictureExtensions);
            }

            // weird!
            return false;
        }

        // use the path to figure out what extensions to use
        if (path.StartsWith("virtualpath://music")) {
            SetMask(g_stSettings.m_musicExtensions);
        } else if (path.StartsWith("virtualpath://video")) {
            SetMask(g_stSettings.m_videoExtensions);
        } else if (path.StartsWith("virtualpath://pictures")) {
            SetMask(g_stSettings.m_pictureExtensions);
        }

        // dont allow prompting, this is a background task
        // although I don't think it matters here
        SetAllowPrompting(false);

        // it's a virtual path, get all items
        return CVirtualPathDirectory::GetDirectory(strPath, items);
    }

    bool GetMatchingShare(const CStdString &strPath, CShare& share) {
        if (!CVirtualPathDirectory::GetMatchingShare(strPath, share))
            return false;

        // filter out non local shares
        vector<CStdString> paths;
        for (unsigned int i = 0; i < share.vecPaths.size(); i++) {
            if (!CUtil::IsRemote(share.vecPaths[i])) {
                paths.push_back(share.vecPaths[i]);
            }
        }
        share.vecPaths = paths;
        return true;
    }
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
|   CUPnP::CUPnP
+---------------------------------------------------------------------*/
class CUPnPServer : public PLT_FileMediaServer
{
public:
    CUPnPServer(const char* friendly_name, const char* uuid = NULL) : 
      PLT_FileMediaServer("", friendly_name, true, uuid) {
          // hack: override path to make sure it's empty
          // urls will contain full paths to local files
          m_Path = "";
          m_DirDelimiter = NPT_WIN32_DIR_DELIMITER_STR;
      }

    virtual NPT_Result OnBrowseMetadata(
        PLT_ActionReference& action, 
        const char*          object_id, 
        NPT_SocketInfo*      info = NULL);
    virtual NPT_Result OnBrowseDirectChildren(
        PLT_ActionReference& action, 
        const char*          object_id, 
        NPT_SocketInfo*      info = NULL);

private:
    PLT_MediaObject* Build(
        CFileItem*      item, 
        bool            with_count = true, 
        NPT_SocketInfo* info = NULL);
};

/*----------------------------------------------------------------------
|   CUPnPServer::Build
+---------------------------------------------------------------------*/
PLT_MediaObject* 
CUPnPServer::Build(CFileItem*        item, 
                   bool              with_count /* = true */, 
                   NPT_SocketInfo*   info /* = NULL */)
{
    PLT_MediaObject* object = NULL;
    NPT_String       path = item->m_strPath;
    NPT_String       share_name;
    NPT_String       file_path;

    bool ret = CUPnPVirtualPathDirectory::SplitPath(path, share_name, file_path);
    if (!ret && path != "0") return NULL;

    if (file_path.GetLength()) {
        // this is not a virtual directory
        object = BuildFromFilePath(file_path, with_count, info, true);
        if (!object) return NULL;

        // prepend back the share name in front
        object->m_ObjectID = share_name + "/" + file_path;

        // override object id & change the class if it's an item
        // and it's not been set previously
        if (object->m_ObjectID.StartsWith("virtualpath://music")) {
            if (object->m_ObjectClass.type == "object.item") {
                object->m_ObjectClass.type = "object.item.audioitem";
            }
        } else if (object->m_ObjectID.StartsWith("virtualpath://video")) {
            if (object->m_ObjectClass.type == "object.item") {
                object->m_ObjectClass.type = "object.item.videoitem";
            }
        } else if (object->m_ObjectID.StartsWith("virtualpath://pictures")) {
            if (object->m_ObjectClass.type == "object.item") {
                object->m_ObjectClass.type = "object.item.imageitem";
            }
        }

        // populate parentid
        if (CUPnPVirtualPathDirectory::FindSharePath(share_name, file_path)) {
            // found the file_path as one of the path of the share
            // this means the parent id is the share
            object->m_ParentID = share_name;
        } else {
            // we didn't find the path, so it means the parent id is
            // the parent folder of the file_path
            int index = file_path.ReverseFind("\\");
            if (index == -1) {
                // weird!
                delete object;
                return NULL;
            }
            object->m_ParentID = share_name + "/" + file_path.Left(index);
        }
    } else {
        object = new PLT_MediaContainer;
        object->m_Title = item->GetLabel();
        object->m_ObjectClass.type = "object.container";
        object->m_ObjectID = path;

        if (path == "0") {
            // root
            object->m_ParentID = "-1";
        } else if (share_name.GetLength() == 0) {
            // no share_name means it's virtualpath://X where X=music, video or pictures
            object->m_ParentID = "0";
        } else if (share_name.StartsWith("virtualpath://music")) {
            object->m_ParentID = "virtualpath://music";
        } else if (share_name.StartsWith("virtualpath://video")) {
            object->m_ParentID = "virtualpath://video";
        } else if (share_name.StartsWith("virtualpath://pictures")) {
            object->m_ParentID = "virtualpath://pictures";
        } else {
            // weird!
            delete object;
            return NULL;
        }

        // how to get children count?
    }

    return object;
}

/*----------------------------------------------------------------------
|   CUPnPServer::OnBrowseMetadata
+---------------------------------------------------------------------*/
CUPnPServer::OnBrowseMetadata(PLT_ActionReference& action, 
                              const char*          object_id, 
                              NPT_SocketInfo*      info /* = NULL */)
{
    NPT_String didl;
    NPT_Reference<PLT_MediaObject> object;
    NPT_String id = object_id;
    CShare share;
    CUPnPVirtualPathDirectory dir;

    CFileItem* item = NULL;

    if (id == "0") {
        item = new CFileItem("0", true);
        item->SetLabel("Root");
        object = Build(item, true, info);
    } else if (id == "virtualpath://music") {
        item = new CFileItem((const char*)id, true);
        item->SetLabel("Music");
        object = Build(item, true, info);
    } else if (id == "virtualpath://video") {
        item = new CFileItem((const char*)id, true);
        item->SetLabel("Video");
        object = Build(item, true, info);
    } else if (id == "virtualpath://pictures") {
        item = new CFileItem((const char*)id, true);
        item->SetLabel("Pictures");
        object = Build(item, true, info);
    } else if (dir.GetMatchingShare((const char*)id, share)) {
        item = new CFileItem((const char*)id, true);
        item->SetLabel(share.strName);
        object = Build(item, true, info);
    } else {
        item = new CFileItem((const char*)id, true);
        item->SetLabel((const char*)id);
        object = Build(item, true, info);
        if (!object.IsNull()) object->m_ObjectID = id;
    }

    delete item;
    if (object.IsNull()) return NPT_FAILURE;

    NPT_String filter;
    NPT_CHECK(action->GetArgumentValue("Filter", filter));

    NPT_String tmp;    
    NPT_CHECK(PLT_Didl::ToDidl(*object.AsPointer(), filter, tmp));

    /* add didl header and footer */
    didl = didl_header + tmp + didl_footer;

    NPT_CHECK(action->SetArgumentValue("Result", didl));
    NPT_CHECK(action->SetArgumentValue("NumberReturned", "1"));
    NPT_CHECK(action->SetArgumentValue("TotalMatches", "1"));

    // update ID may be wrong here, it should be the one of the container?
    NPT_CHECK(action->SetArgumentValue("UpdateId", "1"));
    // TODO: We need to keep track of the overall updateID of the CDS

    return NPT_SUCCESS;
}
/*----------------------------------------------------------------------
|   CUPnPServer::OnBrowseDirectChildren
+---------------------------------------------------------------------*/
CUPnPServer::OnBrowseDirectChildren(PLT_ActionReference& action, 
                                    const char*          object_id, 
                                    NPT_SocketInfo*      info /* = NULL */)
{
    CUPnPVirtualPathDirectory dir;
    CFileItemList items;
    if (!dir.GetDirectory(object_id, items)) {
        /* error */
        PLT_Log(PLT_LOG_LEVEL_1, "CUPnPServer::OnBrowseDirectChildren - ObjectID not found.");
        action->SetError(701, "No Such Object.");
        return NPT_FAILURE;
    }

    NPT_String share_name;
    NPT_String file_path;
    CUPnPVirtualPathDirectory::SplitPath(object_id, share_name, file_path);

    NPT_String filter;
    NPT_String startingInd;
    NPT_String reqCount;

    NPT_CHECK(action->GetArgumentValue("Filter", filter));
    NPT_CHECK(action->GetArgumentValue("StartingIndex", startingInd));
    NPT_CHECK(action->GetArgumentValue("RequestedCount", reqCount));   

    unsigned long start_index, req_count;
    if (NPT_FAILED(startingInd.ToInteger(start_index)) ||
        NPT_FAILED(reqCount.ToInteger(req_count))) {
        return NPT_FAILURE;
    }
        
    unsigned long cur_index = 0;
    unsigned long num_returned = 0;
    unsigned long total_matches = 0;
    //unsigned long update_id = 0;
    NPT_String didl = didl_header;
    PLT_MediaObjectReference item;
    for (int i=0; i < (int) items.Size(); ++i) {
        // if the item doesn't start by virtualpath://
        // we need to prepend the sharename
        if (!NPT_String(items[i]->m_strPath.c_str()).StartsWith("virtualpath://")) {
            if (share_name.GetLength() == 0) return NPT_FAILURE; // weird!
            items[i]->m_strPath = (const char*) NPT_String(share_name + "/" + items[i]->m_strPath.c_str());
        }
        item = Build(items[i], true, info);
        if (!item.IsNull()) {
            if ((cur_index >= start_index) && ((num_returned < req_count) || (req_count == 0))) {
                NPT_String tmp;
                NPT_CHECK(PLT_Didl::ToDidl(*item.AsPointer(), filter, tmp));

                didl += tmp;
                num_returned++;
            }
            cur_index++;
            total_matches++;        
        }
    }

    didl += didl_footer;

    NPT_CHECK(action->SetArgumentValue("Result", didl));
    NPT_CHECK(action->SetArgumentValue("NumberReturned", NPT_String::FromInteger(num_returned)));
    NPT_CHECK(action->SetArgumentValue("TotalMatches", NPT_String::FromInteger(total_matches)));
    NPT_CHECK(action->SetArgumentValue("UpdateId", "1"));
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CUPnP::CUPnP
+---------------------------------------------------------------------*/
CUPnP::CUPnP() :
    m_CtrlPointHolder(new CCtrlPointReferenceHolder())
{
    //PLT_SetLogLevel(PLT_LOG_LEVEL_4);

    // initialize upnp in non multicast listening mode
    m_UPnP = new PLT_UPnP(1900, false);
    m_CtrlPointHolder->m_CtrlPoint = new PLT_CtrlPoint();

    // create a server
    // TODO: specify a unique UUID we save somewhere
    //       also, let the user set the friendlyname
    PLT_DeviceHostReference server(new CUPnPServer("Xbox Media Center"));
    server->m_ModelName = "Xbox Media Center";

    // since the xbox doesn't support multicast
    // we use broadcast but we advertise more often
    server->SetBroadcast(true);

    // start server
    m_UPnP->AddDevice(server);

    // tell controller to ignore ourselves from list of upnp servers
    m_CtrlPointHolder->m_CtrlPoint->IgnoreUUID(server->GetUUID());

    // start controller
    m_UPnP->AddCtrlPoint(m_CtrlPointHolder->m_CtrlPoint);

    // start browser
    m_MediaBrowser = new PLT_SyncMediaBrowser(m_CtrlPointHolder->m_CtrlPoint);

    // start upnp monitoring
    m_UPnP->Start();

    // Issue a search request on the broadcast address instead of the upnp multicast address 239.255.255.250
    // since the xbox does not support multicast. UPnP devices will still receive our request and respond to us
    // since they're listening on port 1900 in multicast
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
    m_CtrlPointHolder->m_CtrlPoint->IgnoreUUID(uuid);
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

        // special case for Xbox Media Center
        if (root_id == "0" && ( (*device)->m_ModelName.Find("Xbox Media Center", 0, true) >= 0)) {

            // look for a specific type to differentiate which folder we want
            if (audio && !video && !image) {
                // music
                root_id = "virtualpath://music";
            } else if (!audio && video && !image) {
                // video
                root_id = "virtualpath://video";
            } else if (!audio && !video && image) {
                // pictures
                root_id = "virtualpath://pictures";
            }
        }

        // if error, the list could be partial and that's ok
        // we still want to process it
        PLT_MediaObjectListReference list;
        upnp->m_MediaBrowser->Browse(*device, root_id, list);
        if (list.IsNull()) return false;

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

