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


#include "stdafx.h"
#include "Util.h"

#include "xbox/Network.h"
#include "UPnP.h"
#include "FileSystem/UPnPVirtualPathDirectory.h"
#include "Platinum.h"
#include "PltFileMediaServer.h"
#include "PltMediaServer.h"
#include "PltMediaBrowser.h"
#include "PltSyncMediaBrowser.h"
#include "PltDidl.h"
#include "NptNetwork.h"

/*----------------------------------------------------------------------
|   static
+---------------------------------------------------------------------*/
CUPnP* CUPnP::upnp = NULL;
// change to false for XBMC_PC if you want real UPnP functionality
// otherwise keep to true for xbmc as it doesn't support multicast
// don't change unless you know what you're doing!
bool CUPnP::broadcast = true; 

#ifdef HAS_XBOX_NETWORK
#include <xtl.h>
#include <winsockx.h>
#include "NptXboxNetwork.h"

/*----------------------------------------------------------------------
|   static initializer
+---------------------------------------------------------------------*/
NPT_WinsockSystem::NPT_WinsockSystem() 
{
}

NPT_WinsockSystem::~NPT_WinsockSystem() 
{
}

NPT_WinsockSystem NPT_WinsockSystem::Initializer;

/*----------------------------------------------------------------------
|       NPT_NetworkInterface::GetNetworkInterfaces
+---------------------------------------------------------------------*/
NPT_Result
NPT_NetworkInterface::GetNetworkInterfaces(NPT_List<NPT_NetworkInterface*>& interfaces)
{
    XNADDR xna;
    DWORD  state;
    do {
        state = XNetGetTitleXnAddr(&xna);
        Sleep(100);
    } while (state == XNET_GET_XNADDR_PENDING);

    if (state & XNET_GET_XNADDR_STATIC || state & XNET_GET_XNADDR_DHCP) {
        NPT_IpAddress primary_address;
        primary_address.ResolveName(g_network.m_networkinfo.ip);

        NPT_IpAddress netmask;
        netmask.ResolveName(g_network.m_networkinfo.subnet);

        NPT_IpAddress broadcast_address;        
        broadcast_address.ResolveName("255.255.255.255");

//        {
//            // broadcast address is incorrect
//            unsigned char addr[4];
//            for(int i=0; i<4; i++) {
//                addr[i] = (primary_address.AsBytes()[i] & netmask.AsBytes()[i]) | 
//                    ~netmask.AsBytes()[i];
//            }
//            broadcast_address.Set(addr);
//        }


        NPT_Flags     flags = NPT_NETWORK_INTERFACE_FLAG_BROADCAST | NPT_NETWORK_INTERFACE_FLAG_MULTICAST;

        NPT_MacAddress mac;
        if (state & XNET_GET_XNADDR_ETHERNET) {
            mac.SetAddress(NPT_MacAddress::TYPE_ETHERNET, xna.abEnet, 6);
        }

        // create an interface object
        char iface_name[5];
        iface_name[0] = 'i';
        iface_name[1] = 'f';
        iface_name[2] = '0';
        iface_name[3] = '0';
        iface_name[4] = '\0';
        NPT_NetworkInterface* iface = new NPT_NetworkInterface(iface_name, mac, flags);

        // set the interface address
        NPT_NetworkInterfaceAddress iface_address(
            primary_address,
            broadcast_address,
            NPT_IpAddress::Any,
            netmask);
        iface->AddAddress(iface_address);  

        // add the interface to the list
        interfaces.Add(iface);  
    }

    return NPT_SUCCESS;
}
#endif

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
    PLT_MediaObject* BuildObject(
        CFileItem*      item,
        NPT_String&     file_path,
        bool            with_count = false,
        NPT_SocketInfo* info = NULL);

    PLT_MediaObject* Build(
        CFileItem*      item, 
        bool            with_count = false, 
        NPT_SocketInfo* info = NULL,
        const char*     parent_id = NULL);

    static NPT_String GetParentFolder(NPT_String file_path) {       
        int index = file_path.ReverseFind("\\");
        if (index == -1) return "";

        return file_path.Left(index);
    }
};

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::BuildObject
+---------------------------------------------------------------------*/
PLT_MediaObject*
CUPnPServer::BuildObject(CFileItem*      item,
                         NPT_String&     file_path,
                         bool            with_count /* = false */,
                         NPT_SocketInfo* info /* = NULL */)
{
    PLT_MediaItemResource resource;
    PLT_MediaObject*      object = NULL;

    if (!item->m_bIsFolder) {
        object = new PLT_MediaItem();

        /* Set the title using the filename for now */
        object->m_Title = item->GetLabel();
        if (object->m_Title.GetLength() == 0) goto failure;

        /* we need a valid extension to retrieve the mimetype for the protocol info */
        CStdString ext = CUtil::GetExtension((const char*)file_path);

        /* Set the protocol Info from the extension */
        resource.m_ProtocolInfo = PLT_MediaItem::GetProtInfoFromExt(ext);
        if (resource.m_ProtocolInfo.GetLength() == 0)  goto failure;

        /* Set the resource file size */
        resource.m_Size = (NPT_Integer)item->m_dwSize;

        // get list of ip addresses
        NPT_List<NPT_String> ips;
        NPT_CHECK_LABEL(PLT_UPnPMessageHelper::GetIPAddresses(ips), failure);

        // if we're passed an interface where we received the request from
        // move the ip to the top
        if (info && info->local_address.GetIpAddress().ToString() != "0.0.0.0") {
            ips.Remove(info->local_address.GetIpAddress().ToString());
            ips.Insert(ips.GetFirstItem(), info->local_address.GetIpAddress().ToString());
        }

        // iterate through list and build list of resources
        NPT_List<NPT_String>::Iterator ip = ips.GetFirstItem();
        while (ip) {
            NPT_HttpUrl uri = m_FileBaseUri;
            NPT_HttpUrlQuery query;
            query.AddField("path", file_path);
            uri.SetHost(*ip);
            uri.SetQuery(query.ToString());
            resource.m_Uri = uri.ToString();

            object->m_ObjectClass.type = PLT_MediaItem::GetUPnPClassFromExt(ext);
            object->m_Resources.Add(resource);

            ++ip;
        }
    } else {
        object = new PLT_MediaContainer;

        /* Assign a title for this container */
        object->m_Title = item->GetLabel();
        if (object->m_Title.GetLength() == 0) goto failure;
       
        /* Get the number of children for this container */
        if (with_count) {
            NPT_Cardinal count = 0;
            NPT_CHECK_LABEL(GetEntryCount(file_path, count), failure);
            ((PLT_MediaContainer*)object)->m_ChildrenCount = count;
        }

        object->m_ObjectClass.type = "object.container";
    }

    return object;

failure:
    delete object;
    return NULL;
}

/*----------------------------------------------------------------------
|   CUPnPServer::Build
+---------------------------------------------------------------------*/
PLT_MediaObject* 
CUPnPServer::Build(CFileItem*        item, 
                   bool              with_count /* = true */, 
                   NPT_SocketInfo*   info /* = NULL */,
                   const char*       parent_id /* = NULL */)
{
    PLT_MediaObject* object = NULL;
    NPT_String       path = item->m_strPath.c_str();
    NPT_String       share_name;
    NPT_String       file_path;

    //HACK: temporary disabling count as it thrashes HDD
    with_count = false;

    bool ret = CUPnPVirtualPathDirectory::SplitPath(path, share_name, file_path);
    if (!ret && path != "0") goto failure;

    if (file_path.GetLength()) {
        // make sure the path starts with something that is shared given the share
        if (!CUPnPVirtualPathDirectory::FindSharePath(share_name, file_path, true)) goto failure;
        
        // this is not a virtual directory
        object = BuildObject(item, file_path, with_count, info);
        if (!object) goto failure;

        // prepend back the share name in front
        object->m_ObjectID = share_name + "/" + file_path;

        // override object id & change the class if it's an item
        // and it's not been set previously
        if (object->m_ObjectID.StartsWith("virtualpath://upnpmusic")) {
            if (object->m_ObjectClass.type == "object.item") {
                object->m_ObjectClass.type = "object.item.audioitem";
            }
        } else if (object->m_ObjectID.StartsWith("virtualpath://upnpvideo")) {
            if (object->m_ObjectClass.type == "object.item") {
                object->m_ObjectClass.type = "object.item.videoitem";
            }
        } else if (object->m_ObjectID.StartsWith("virtualpath://upnppictures")) {
            if (object->m_ObjectClass.type == "object.item") {
                object->m_ObjectClass.type = "object.item.imageitem";
            }
        }

        if (parent_id) {
            object->m_ParentID = parent_id;
        } else {
            // populate parentid manually
            if (CUPnPVirtualPathDirectory::FindSharePath(share_name, file_path)) {
                // found the file_path as one of the path of the share
                // this means the parent id is the share
                object->m_ParentID = share_name;
            } else {
                // we didn't find the path, find the parent path
                NPT_String parent_path = GetParentFolder(file_path);
                if (parent_path.IsEmpty()) goto failure;

                // try again with parent
                if (CUPnPVirtualPathDirectory::FindSharePath(share_name, parent_path)) {
                    // found the file_path parent folder as one of the path of the share
                    // this means the parent id is the share
                    object->m_ParentID = share_name;
                } else {
                    object->m_ParentID = share_name + "/" + parent_path;
                }
            }
        }
    } else {
        object = new PLT_MediaContainer;
        object->m_Title = item->GetLabel();
        object->m_ObjectClass.type = "object.container";
        object->m_ObjectID = path;

        if (path == "0") {
            // root
            object->m_ParentID = "-1";
            // root has 3 children
            if (with_count) ((PLT_MediaContainer*)object)->m_ChildrenCount = 3;
        } else if (share_name.GetLength() == 0) {
            // no share_name means it's virtualpath://X where X=music, video or pictures
            object->m_ParentID = "0";
            if (with_count) {
                ((PLT_MediaContainer*)object)->m_ChildrenCount = 0;

                // look up number of shares
                VECSHARES *shares = NULL;
                if (path == "virtualpath://upnpmusic") {
                    shares = g_settings.GetSharesFromType("upnpmusic");
                } else if (path == "virtualpath://upnpvideo") {
                    shares = g_settings.GetSharesFromType("upnpvideo");
                } else if (path == "virtualpath://upnppictures") {
                    shares = g_settings.GetSharesFromType("upnppictures");
                }

                // use only shares that would some path with local files
                if (shares) {
                    CUPnPVirtualPathDirectory dir;
                    for (unsigned int i = 0; i < shares->size(); i++) {
                        // Does this share contains any local paths?
                        CShare &share = shares->at(i);
                        vector<CStdString> paths;

                        // reconstruct share name as it could have been replaced by
                        // a path if there was just one entry
                        NPT_String share_name = path + "/";
                        share_name += share.strName;
                        if (dir.GetMatchingShare((const char*)share_name, share, paths) && paths.size()) {
                            ((PLT_MediaContainer*)object)->m_ChildrenCount++;
                        }
                    }
                }
            }
        } else {
            CStdString mask;
            // this is a share name
            if (share_name.StartsWith("virtualpath://upnpmusic")) {
                object->m_ParentID = "virtualpath://upnpmusic";
                mask = g_stSettings.m_musicExtensions;
            } else if (share_name.StartsWith("virtualpath://upnpvideo")) {
                object->m_ParentID = "virtualpath://upnpvideo";
                mask = g_stSettings.m_videoExtensions;
            } else if (share_name.StartsWith("virtualpath://upnppictures")) {
                object->m_ParentID = "virtualpath://upnppictures";
                mask = g_stSettings.m_pictureExtensions;
            } else {
                // weird!
                goto failure;
            }

            if (with_count) {
                ((PLT_MediaContainer*)object)->m_ChildrenCount = 0;

                // get all the paths for a given share
                CShare share;
                CUPnPVirtualPathDirectory dir;
                vector<CStdString> paths;
                if (!dir.GetMatchingShare((const char*)share_name, share, paths)) goto failure;
                for (unsigned int i=0; i<paths.size(); i++) {
                    // FIXME: this is not efficient, we only need the number of items given a mask
                    // and not the list of items

                    // retrieve all the files for a given path
                   CFileItemList items;
                   if (CDirectory::GetDirectory(paths[i], items, mask)) {
                       // update childcount
                       ((PLT_MediaContainer*)object)->m_ChildrenCount += items.Size();
                   }
                }
            }
        }
    }

    return object;

failure:
    delete object;
    return NULL;
}

/*----------------------------------------------------------------------
|   CUPnPServer::OnBrowseMetadata
+---------------------------------------------------------------------*/
NPT_Result
CUPnPServer::OnBrowseMetadata(PLT_ActionReference& action, 
                              const char*          object_id, 
                              NPT_SocketInfo*      info /* = NULL */)
{
    NPT_String didl;
    NPT_Reference<PLT_MediaObject> object;
    NPT_String id = object_id;
    CShare share;
    CUPnPVirtualPathDirectory dir;
    vector<CStdString> paths;

    CFileItem* item = NULL;

    if (id == "0") {
        item = new CFileItem("0", true);
        item->SetLabel("Root");
        object = Build(item, true, info);
    } else if (id == "virtualpath://upnpmusic") {
        item = new CFileItem((const char*)id, true);
        item->SetLabel("Music");
        object = Build(item, true, info);
    } else if (id == "virtualpath://upnpvideo") {
        item = new CFileItem((const char*)id, true);
        item->SetLabel("Video");
        object = Build(item, true, info);
    } else if (id == "virtualpath://upnppictures") {
        item = new CFileItem((const char*)id, true);
        item->SetLabel("Pictures");
        object = Build(item, true, info);
    } else if (dir.GetMatchingShare((const char*)id, share, paths)) {
        item = new CFileItem((const char*)id, true);
        item->SetLabel(share.strName);
        object = Build(item, true, info);
    } else {
        NPT_String share_name, file_path;
        if (!CUPnPVirtualPathDirectory::SplitPath(id, share_name, file_path)) 
            return NPT_FAILURE;

        NPT_String parent_path = GetParentFolder(file_path);
        if (parent_path.IsEmpty()) return NPT_FAILURE;

        NPT_DirectoryEntryInfo entry_info;
        NPT_CHECK(NPT_DirectoryEntry::GetInfo(file_path, entry_info));

        item = new CFileItem((const char*)id, (entry_info.type==NPT_DIRECTORY_TYPE)?true:false);
        item->SetLabel((const char*)file_path.SubString(parent_path.GetLength()+1));

        // get file size
        if (entry_info.type == NPT_FILE_TYPE) {
            item->m_dwSize = entry_info.size;
        }

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
NPT_Result
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
        return NPT_SUCCESS;
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
        item = Build(items[i], true, info, object_id);
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
    m_ServerHolder(new CDeviceHostReferenceHolder()),
    m_CtrlPointHolder(new CCtrlPointReferenceHolder())
{
    //PLT_SetLogLevel(PLT_LOG_LEVEL_4);

    // initialize upnp in broadcast listening mode for xbmc
    m_UPnP = new PLT_UPnP(1900, !broadcast);

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
|   CUPnP::StartClient
+---------------------------------------------------------------------*/
void
CUPnP::StartClient()
{
    if (!m_CtrlPointHolder->m_CtrlPoint.IsNull()) return;

    // create controlpoint, pass NULL to avoid sending a multicast search
    m_CtrlPointHolder->m_CtrlPoint = new PLT_CtrlPoint(broadcast?NULL:"upnp:rootdevice");

    // ignore our own server
    if (!m_ServerHolder->m_Device.IsNull()) {
        m_CtrlPointHolder->m_CtrlPoint->IgnoreUUID(m_ServerHolder->m_Device->GetUUID());
    }

    // start it
    m_UPnP->AddCtrlPoint(m_CtrlPointHolder->m_CtrlPoint);

    // start browser
    m_MediaBrowser = new PLT_SyncMediaBrowser(m_CtrlPointHolder->m_CtrlPoint, true);
    
    // Issue a search request on the broadcast address instead of the upnp multicast address 239.255.255.250
    // since the xbox does not support multicast. UPnP devices will still receive our request and respond to us
    // since they're listening on port 1900 in multicast
    // Repeat every 6 seconds
    if (broadcast) {
        m_CtrlPointHolder->m_CtrlPoint->Discover(NPT_HttpUrl("255.255.255.255", 1900, "*"), "upnp:rootdevice", 1, 6000);
    }
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
|   CUPnP::StartServer
+---------------------------------------------------------------------*/
void
CUPnP::StartServer()
{
    if (!m_ServerHolder->m_Device.IsNull()) return;

    // load upnpserver.xml so that g_settings.m_vecUPnPMusicShares, etc.. are loaded
    g_settings.LoadUPnPXml("q:\\system\\upnpserver.xml");

    // create the server with the friendlyname and UUID from upnpserver.xml if found
    m_ServerHolder->m_Device = new CUPnPServer(
        g_settings.m_UPnPServerFriendlyName.length()?g_settings.m_UPnPServerFriendlyName.c_str():"Xbox Media Center",
        g_settings.m_UPnPUUID.length()?g_settings.m_UPnPUUID.c_str():NULL);

    // trying to set optional upnp values for XP UPnP UI Icons to detect us
    // but it doesn't work anyways as it requires multicast for XP to detect us
    NPT_String ip = g_network.m_networkinfo.ip;
#ifndef HAS_XBOX_NETWORK
    NPT_List<NPT_String> list;
    if (NPT_SUCCEEDED(PLT_UPnPMessageHelper::GetIPAddresses(list))) {
        ip = *(list.GetFirstItem());
    }
#endif
    m_ServerHolder->m_Device->m_PresentationURL = NPT_HttpUrl(ip, atoi(g_guiSettings.GetString("servers.webserverport")), "/").ToString();
    m_ServerHolder->m_Device->m_ModelName = "Xbox Media Center";
    m_ServerHolder->m_Device->m_ModelDescription = "Xbox Media Center";
    m_ServerHolder->m_Device->m_ModelURL = "http://www.xboxmediacenter.com/";
    m_ServerHolder->m_Device->m_ModelNumber = "2.0";
    m_ServerHolder->m_Device->m_ModelName = "XBMC";
    m_ServerHolder->m_Device->m_Manufacturer = "Xbox Team";
    m_ServerHolder->m_Device->m_ManufacturerURL = "http://www.xboxmediacenter.com/";

    // since the xbox doesn't support multicast
    // we use broadcast but we advertise more often
    m_ServerHolder->m_Device->SetBroadcast(broadcast);

    // tell controller to ignore ourselves from list of upnp servers
    if (!m_CtrlPointHolder->m_CtrlPoint.IsNull()) {
        m_CtrlPointHolder->m_CtrlPoint->IgnoreUUID(m_ServerHolder->m_Device->GetUUID());
    }

    // start server
    m_UPnP->AddDevice(m_ServerHolder->m_Device);

    // save UUID
    g_settings.m_UPnPUUID = m_ServerHolder->m_Device->GetUUID();
    g_settings.SaveUPnPXml("q:\\system\\upnpserver.xml");
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
