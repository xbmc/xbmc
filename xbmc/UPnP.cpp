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
#include "Application.h"

#include "xbox/Network.h"
#include "UPnP.h"
#include "FileSystem/UPnPVirtualPathDirectory.h"
#include "FileSystem/MusicDatabaseDirectory.h"
#include "FileSystem/VideoDatabaseDirectory.h"
#include "MusicDatabase.h"
#include "VideoDatabase.h"
#include "FileSystem/VideoDatabaseDirectory/DirectoryNode.h"
#include "FileSystem/VideoDatabaseDirectory/QueryParams.h"
#include "Platinum.h"
#include "PltFileMediaServer.h"
#include "PltMediaServer.h"
#include "PltMediaBrowser.h"
#include "../MediaRenderer/PltMediaRenderer.h"
#include "PltSyncMediaBrowser.h"
#include "PltDidl.h"
#include "NptNetwork.h"
#include "NptConsole.h"

NPT_SET_LOCAL_LOGGER("xbmc.upnp")

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
    if (!g_network.IsAvailable(true))
        return NPT_ERROR_NETWORK_DOWN;

    NPT_IpAddress primary_address;
    primary_address.ResolveName(g_network.m_networkinfo.ip);

    NPT_IpAddress netmask;
    netmask.ResolveName(g_network.m_networkinfo.subnet);

    NPT_IpAddress broadcast_address;        
    broadcast_address.ResolveName("255.255.255.255");

    NPT_Flags     flags = NPT_NETWORK_INTERFACE_FLAG_BROADCAST | NPT_NETWORK_INTERFACE_FLAG_MULTICAST;

    NPT_MacAddress mac;
    //mac.SetAddress(NPT_MacAddress::TYPE_ETHERNET, g_network.m_networkinfo.mac, 6);

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

    return NPT_SUCCESS;
}
#endif


/*----------------------------------------------------------------------
|   NPT_Console::Output and NPT_GetEnvironment
+---------------------------------------------------------------------*/

void NPT_Console::Output(const char* message)
{
    CLog::Log(LOGDEBUG, message);
}

NPT_Result NPT_GetEnvironment(const char* name, NPT_String& value)
{
    return NPT_FAILURE;
}

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
    virtual NPT_Result OnSearch(
        PLT_ActionReference& action, 
        const NPT_String& object_id, 
        const NPT_String& searchCriteria,
        NPT_SocketInfo* info = NULL);

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

    // some usefull buffers
    CStdStringArray strings;
    CStdString buffer;

    // get list of ip addresses
    NPT_List<NPT_String> ips;
    NPT_CHECK_LABEL(PLT_UPnPMessageHelper::GetIPAddresses(ips), failure);

    // if we're passed an interface where we received the request from
    // move the ip to the top
    if (info && info->local_address.GetIpAddress().ToString() != "0.0.0.0") {
        ips.Remove(info->local_address.GetIpAddress().ToString());
        ips.Insert(ips.GetFirstItem(), info->local_address.GetIpAddress().ToString());
    }

    if (!item->m_bIsFolder) {
        object = new PLT_MediaItem();

        object->m_ObjectID = item->m_strPath;

        /* Setup object type */
        if( item->IsMusicDb() ) {
            object->m_ObjectClass.type = "object.item.audioitem";
          
            if( item->HasMusicInfoTag() ) {
                CMusicInfoTag *tag = item->GetMusicInfoTag();
                if( !tag->GetURL().IsEmpty() )
                  file_path = tag->GetURL();

                StringUtils::SplitString(tag->GetGenre(), " / ", strings);
                for(CStdStringArray::iterator it = strings.begin(); it != strings.end(); it++) {
                    object->m_Affiliation.genre_extended.Add((*it).c_str());
                }

                object->m_Affiliation.album = tag->GetAlbum();
                object->m_People.artist = tag->GetArtist();
                object->m_Creator = tag->GetArtist();
                object->m_MiscInfo.original_track_number = tag->GetTrackNumber();
                resource.m_Duration = tag->GetDuration();                
            }

        } else if( item->IsVideoDb() ) {
            object->m_ObjectClass.type = "object.item.videoitem";

            if( item->HasVideoInfoTag() ) {
                CVideoInfoTag *tag = item->GetVideoInfoTag();
                if( !tag->m_strFileNameAndPath.IsEmpty() )
                  file_path = tag->m_strFileNameAndPath;

                StringUtils::SplitString(tag->m_strGenre, " / ", strings);                
                for(CStdStringArray::iterator it = strings.begin(); it != strings.end(); it++) {
                    object->m_Affiliation.genre_extended.Add((*it).c_str());
                }

                for(CVideoInfoTag::iCast it = tag->m_cast.begin();it != tag->m_cast.end();it++) {
                    object->m_People.actor += it->first + ",";
                    object->m_People.actor_role += it->second + ",";
                }
                object->m_People.actor.TrimRight(",");
                object->m_People.actor_role.TrimRight(",");

                object->m_People.director = tag->m_strDirector;
                object->m_Description.description = tag->m_strTagLine;
                object->m_Description.long_description = tag->m_strPlot;
                //TODO - this is wrong, imdb gives it as minute string ie, "116 min"
                //resource.m_Duration = StringUtils::TimeStringToSeconds(tag->m_strRuntime.c_str());
            }

        } else if( item->IsAudio() ) {
            object->m_ObjectClass.type = "object.item.audioitem";
        } else if( item->IsVideo() ) {
            object->m_ObjectClass.type = "object.item.videoitem";
        } else if( item->IsPicture() ) {
            object->m_ObjectClass.type = "object.item.imageitem";
        }

        /* we need a valid extension to retrieve the mimetype for the protocol info */
        CStdString ext = CUtil::GetExtension((const char*)file_path);

        /* if we still miss a object class, try set it now from extension */
        if( object->m_ObjectClass.type == "object.item" || object->m_ObjectClass.type == "" ) {
            object->m_ObjectClass.type = PLT_MediaItem::GetUPnPClassFromExt(ext);
        }

        /* Set the protocol Info from the extension */
        resource.m_ProtocolInfo = PLT_MediaItem::GetProtInfoFromExt(ext);
        if (resource.m_ProtocolInfo.GetLength() == 0)  goto failure;

        /* Set the resource file size */
        resource.m_Size = (NPT_Integer)item->m_dwSize;

        // if the item is remote, add a direct link to the item
        if( CUtil::IsRemote ( (const char*)file_path ) ) {
            resource.m_Uri = file_path;
            object->m_Resources.Add(resource);
        }

        // iterate through ip addresses and build list of resources
        // throught http file server
        NPT_List<NPT_String>::Iterator ip = ips.GetFirstItem();
        while (ip) {
            NPT_HttpUrl uri = m_FileBaseUri;
            NPT_HttpUrlQuery query;
            query.AddField("path", file_path);
            uri.SetHost(*ip);
            uri.SetQuery(query.ToString());
            resource.m_Uri = uri.ToString();
            
            object->m_Resources.Add(resource);

            ++ip;
        }        
    } else {
        PLT_MediaContainer* container = new PLT_MediaContainer;
        object = container;

        /* Assign a title and id for this container */
        container->m_ObjectID = item->m_strPath;
        container->m_ObjectClass.type = "object.container";
        container->m_ChildrenCount = -1;

        if(item->IsMusicDb()) {
        } else if(item->IsVideoDb()) {
        }

        /* Get the number of children for this container */
        if (with_count) {
            if( object->m_ObjectID.StartsWith("virtualpath://") ) {
                NPT_Cardinal count = 0;
                NPT_CHECK_LABEL(GetEntryCount(file_path, count), failure);
                container->m_ChildrenCount = count;
            } else {
                /* this should be a standard path */
                // TODO - get file count of this directory
            }
        }
        
    }
    
    // set a title for the object
    if( !item->GetLabel().IsEmpty() /* item->IsLabelPreformated() */ ) {
        object->m_Title = item->GetLabel();
    } else {
        object->m_Title = CUtil::GetTitleFromPath(item->m_strPath, item->m_bIsFolder);
    }

    // set a thumbnail if we have one
    if( item->HasThumbnail() ) {
        NPT_HttpUrl uri = m_FileBaseUri;
        NPT_HttpUrlQuery query;
        query.AddField("path", item->GetThumbnailImage() );
        uri.SetHost(*ips.GetFirstItem());
        uri.SetQuery(query.ToString());
        object->m_ExtraInfo.album_art_uri = uri.ToString();
    }

    return object;

failure:
    if(object)
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

    if(!CUPnPVirtualPathDirectory::SplitPath(path, share_name, file_path))
    {
      file_path = item->m_strPath;
      share_name = "";

      if (path.StartsWith("musicdb://")) {
          CStdString label;
          if( path == "musicdb://" ) {              
              item->SetLabel("Music Library");
              item->SetLabelPreformated(true);
          } else {

              if( !item->HasMusicInfoTag() || !item->GetMusicInfoTag()->Loaded() )
                  item->LoadMusicTag();

              if( !item->HasThumbnail() )
                  item->SetCachedMusicThumb();

              if( item->GetLabel().IsEmpty() ) {
                  /* if no label try to grab it from node type */
                  if( CMusicDatabaseDirectory::GetLabel((const char*)path, label) ) {
                      item->SetLabel(label);
                      item->SetLabelPreformated(true);
                  }
              }
          }
      } else if (file_path.StartsWith("videodb://")) {
          CStdString label;
          if( path == "videodb://" ) {
              item->SetLabel("Video Library");
              item->SetLabelPreformated(true);
          } else {


              if( !item->HasVideoInfoTag() ) {
                  DIRECTORY::VIDEODATABASEDIRECTORY::CQueryParams params;
                  DIRECTORY::VIDEODATABASEDIRECTORY::CDirectoryNode::GetDatabaseInfo((const char*)path, params);

                  CVideoDatabase db;
                  if( !db.Open() )
                      return NULL;

                  if( params.GetMovieId() >= 0 )
                      db.GetMovieInfo((const char*)path, *item->GetVideoInfoTag(), params.GetMovieId());
                  else if( params.GetEpisodeId() >= 0 )
                      db.GetEpisodeInfo((const char*)path, *item->GetVideoInfoTag(), params.GetEpisodeId());
                  else if( params.GetTvShowId() >= 0 )
                      db.GetTvShowInfo((const char*)path, *item->GetVideoInfoTag(), params.GetTvShowId());
              }

              // try to grab title from tag
              if( item->HasVideoInfoTag() && !item->GetVideoInfoTag()->m_strTitle.IsEmpty() ) {
                  item->SetLabel( item->GetVideoInfoTag()->m_strTitle );
                  item->SetLabelPreformated(true);
              }

              // try to grab it from the folder
              if( item->GetLabel().IsEmpty() ) {
                  if( CVideoDatabaseDirectory::GetLabel((const char*)path, label) ) {
                      item->SetLabel(label);
                      item->SetLabelPreformated(true);
                  }
              }

              if( !item->HasThumbnail() )
                  item->SetCachedVideoThumb();
          }
          
      }

      //not a virtual path directory, new system
      object = BuildObject(item, file_path, with_count, info);
      if(!object)
        return NULL;

      if(parent_id)
        object->m_ParentID = parent_id;

      return object;
    }

    path.TrimRight("/");
    if (file_path.GetLength()) {
        // make sure the path starts with something that is shared given the share
        if (!CUPnPVirtualPathDirectory::FindSharePath(share_name, file_path, true)) goto failure;
        
        // this is not a virtual directory
        object = BuildObject(item, file_path, with_count, info);
        if (!object) goto failure;

        // override object id & change the class if it's an item
        // and it's not been set previously
        if (object->m_ObjectClass.type == "object.item") {
            if (share_name == "virtualpath://upnpmusic")
                object->m_ObjectClass.type = "object.item.audioitem";
            else if (share_name == "virtualpath://upnpvideo")
                object->m_ObjectClass.type = "object.item.videoitem";
            else if (share_name == "virtualpath://upnppictures")
                object->m_ObjectClass.type = "object.item.imageitem";
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

        // old style, needs virtual path prefix
        if( !object->m_ObjectID.StartsWith("virtualpath://") )
            object->m_ObjectID = share_name + "/" + object->m_ObjectID;

    } else {
        object = new PLT_MediaContainer;
        object->m_Title = item->GetLabel();
        object->m_ObjectClass.type = "object.container";
        object->m_ObjectID = path;

        if (path == "virtualpath://upnproot") {
            // root
            object->m_ObjectID = "0";
            object->m_ParentID = "-1";
            // root has 5 children
            if (with_count) ((PLT_MediaContainer*)object)->m_ChildrenCount = 5;
        } else if (share_name.GetLength() == 0) {
            // no share_name means it's virtualpath://X where X=music, video or pictures
            object->m_ParentID = "0";
            if (with_count || true) { // we can always count these, it's quick
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
    if(object)
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
        id = "virtualpath://upnproot/";
    }

    if (id.StartsWith("virtualpath://")) {

        id.TrimRight("/");
        if (id == "virtualpath://upnproot") {
            id += "/";
            item = new CFileItem((const char*)id, true);
            item->SetLabel("Root");
            item->SetLabelPreformated(true);
            object = Build(item, true, info);
        } else if (id == "virtualpath://upnpmusic") {
            id += "/";
            item = new CFileItem((const char*)id, true);
            item->SetLabel("Music Files");
            item->SetLabelPreformated(true);
            object = Build(item, true, info);
        } else if (id == "virtualpath://upnpvideo") {
            id += "/";
            item = new CFileItem((const char*)id, true);
            item->SetLabel("Video Files");
            item->SetLabelPreformated(true);
            object = Build(item, true, info);
        } else if (id == "virtualpath://upnppictures") {
            id += "/";
            item = new CFileItem((const char*)id, true);
            item->SetLabel("Picture Files");
            item->SetLabelPreformated(true);
            object = Build(item, true, info);
        } else if (dir.GetMatchingShare((const char*)id, share, paths)) {
            id += "/";
            item = new CFileItem((const char*)id, true);
            item->SetLabel(share.strName);
            item->SetLabelPreformated(true);
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
            item->SetLabelPreformated(true);

            // get file size
            if (entry_info.type == NPT_FILE_TYPE) {
                item->m_dwSize = entry_info.size;
            }

            object = Build(item, true, info);
            if (!object.IsNull()) object->m_ObjectID = id;
        }
    } else {

        if( CDirectory::Exists((const char*)id) ) {
            item = new CFileItem((const char*)id, true);
        } else {
            item = new CFileItem((const char*)id, false);            
        }
        CStdString parent;
        if(!CUtil::GetParentPath((const char*)id, parent))
          parent = "0";

        object = Build(item, true, info, parent.c_str());
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
    NPT_String id = object_id;    
    CFileItemList items;

    if (id == "0") {
        id = "virtualpath://upnproot/";
    }

    if (id.StartsWith("virtualpath://")) {
        CUPnPVirtualPathDirectory dir;
        if (!dir.GetDirectory((const char*)id, items)) {
            /* error */
            NPT_LOG_FINE("CUPnPServer::OnBrowseDirectChildren - ObjectID not found.")
            action->SetError(701, "No Such Object.");
            return NPT_SUCCESS;
        }
    } else {
        items.m_strPath = id;
        if (!items.Load()) {
            // cache anything that takes more than a second to retreive
            DWORD time = GetTickCount() + 1000;

            if (!CDirectory::GetDirectory((const char*)id, items)) {
                /* error */
                NPT_LOG_FINE("CUPnPServer::OnBrowseDirectChildren - ObjectID not found.")
                action->SetError(701, "No Such Object.");
                return NPT_SUCCESS;
            }
            if(items.GetCacheToDisc() || time < GetTickCount())
              items.Save();
        }
    }

    NPT_String filter;
    NPT_String startingInd;
    NPT_String reqCount;

    NPT_CHECK_SEVERE(action->GetArgumentValue("Filter", filter));
    NPT_CHECK_SEVERE(action->GetArgumentValue("StartingIndex", startingInd));
    NPT_CHECK_SEVERE(action->GetArgumentValue("RequestedCount", reqCount));   

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
        item = Build(items[i], true, info, object_id);
        if (!item.IsNull()) {
            if ((cur_index >= start_index) && ((num_returned < req_count) || (req_count == 0))) {
                NPT_String tmp;
                NPT_CHECK(PLT_Didl::ToDidl(*item.AsPointer(), filter, tmp));

                // Neptunes string growing is dead slow for small additions
                if(didl.GetCapacity() < tmp.GetLength() + didl.GetLength()) {
                    didl.Reserve(didl.GetCapacity()*4);
                }

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

NPT_Result
CUPnPServer::OnSearch(PLT_ActionReference& action, 
                      const NPT_String& object_id, 
                      const NPT_String& searchCriteria,
                      NPT_SocketInfo* info /*= NULL*/)

{
  // uggly hack to get windows media player to show stuff
  if(searchCriteria.Find("""object.item.audioItem""") >= 0)
      return OnBrowseDirectChildren(action, "musicdb://4", info);
  else if(searchCriteria.Find("""object.item.videoItem""") >= 0)
      return OnBrowseDirectChildren(action, "videodb://1/2", info);

  return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   CUPnP::CMediaRenderer
+---------------------------------------------------------------------*/
class CUPnPRenderer : 
    public PLT_MediaRenderer
{
public:
    CUPnPRenderer(const char*          friendly_name,
                  bool                 show_ip = false,
                  const char*          uuid = NULL,
                  unsigned int         port = 0) :
        PLT_MediaRenderer(NULL, friendly_name, show_ip, uuid, port)
    {
    }

    void UpdateState()
    {
        PLT_Service* avt;
        if(NPT_FAILED(FindServiceByType("urn:schemas-upnp-org:service:AVTransport:1", avt)))
          return;

        bool publish = true;
        CStdString buffer;

        StringUtils::SecondsToTimeString((long)g_application.GetTime(), buffer, TIME_FORMAT_HH_MM_SS);
        avt->SetStateVariable("RelativeTimePosition", buffer.c_str(), publish);

        StringUtils::SecondsToTimeString((long)g_application.GetTotalTime(), buffer, TIME_FORMAT_HH_MM_SS);
        avt->SetStateVariable("CurrentTrackDuration", buffer.c_str(), publish);

        // TODO - these states don't generate events, LastChange state needs to be fixed
        if (g_application.IsPlaying()) {
            avt->SetStateVariable("TransportState", "PLAYING", publish);
            avt->SetStateVariable("TransportStatus", "OK", publish);
            avt->SetStateVariable("TransportPlaySpeed", "1", publish);
            avt->SetStateVariable("NumberOfTracks", "1", publish);
            avt->SetStateVariable("CurrentTrack", "1", publish);
        } else {
            avt->SetStateVariable("TransportState", "STOPPED", publish);
            avt->SetStateVariable("TransportStatus", "OK", publish);
            avt->SetStateVariable("TransportPlaySpeed", "1", publish);
            avt->SetStateVariable("NumberOfTracks", "0", publish);
            avt->SetStateVariable("CurrentTrack", "0", publish);
        }
        avt->NotifyChanged();
    }

    // AVTransport
    virtual NPT_Result OnNext(PLT_ActionReference& action)
    {
        g_applicationMessenger.PlayListPlayerNext();
        return NPT_SUCCESS;
    }
    virtual NPT_Result OnPause(PLT_ActionReference& action)
    {
        if(!g_application.IsPaused())
          g_applicationMessenger.MediaPause();
        return NPT_SUCCESS;
    }
    virtual NPT_Result OnPlay(PLT_ActionReference& action)
    {
        if(g_application.IsPaused())
            g_applicationMessenger.MediaPause();
        return NPT_SUCCESS;
    }
    virtual NPT_Result OnPrevious(PLT_ActionReference& action)
    {
        g_applicationMessenger.PlayListPlayerPrevious();
        return NPT_SUCCESS;
    }
    virtual NPT_Result OnStop(PLT_ActionReference& action)
    {
        g_applicationMessenger.MediaStop();
        return NPT_SUCCESS;
    }
    virtual NPT_Result OnSetAVTransportURI(PLT_ActionReference& action)
    {
        NPT_String uri, meta;
        PLT_Service* service;

        NPT_CHECK_SEVERE(action->GetArgumentValue("CurrentURI",uri));
        NPT_CHECK_SEVERE(action->GetArgumentValue("CurrentURIMetaData",meta));

        NPT_CHECK_SEVERE(FindServiceByType("urn:schemas-upnp-org:service:AVTransport:1", service));

        service->SetStateVariable("TransportState", "TRANSITIONING", false);
        service->SetStateVariable("TransportStatus", "OK", false);
        service->SetStateVariable("TransportPlaySpeed", "1", false);

        service->SetStateVariable("AVTransportURI", uri, false);
        service->SetStateVariable("AVTransportURIMetaData", meta, false);
        NPT_CHECK_SEVERE(action->SetArgumentsOutFromStateVariable());

        g_applicationMessenger.MediaPlay((const char*)uri);
        
        return NPT_SUCCESS;
    }

};

/*----------------------------------------------------------------------
|   CCtrlPointReferenceHolder class
+---------------------------------------------------------------------*/
class CRendererReferenceHolder
{
public:
    PLT_DeviceHostReference m_Device;
};


/*----------------------------------------------------------------------
|   CUPnP::CUPnP
+---------------------------------------------------------------------*/
CUPnP::CUPnP() :
    m_ServerHolder(new CDeviceHostReferenceHolder()),
    m_CtrlPointHolder(new CCtrlPointReferenceHolder()),
    m_RendererHolder(new CRendererReferenceHolder())
{
//#ifdef HAS_XBOX_HARDWARE
//    broadcast = true;
//#else
//    broadcast = false;
//#endif
    // xbox can't receive multicast, but it can send it
    broadcast = false;
    
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

#ifdef _XBOX
    // Issue a search request on the every 6 seconds, both on broadcast and multicast
    // xbox can't receive multicast, but it can send it so upnp clients know we are here
    m_CtrlPointHolder->m_CtrlPoint->Discover(NPT_HttpUrl("255.255.255.255", 1900, "*"), "upnp:rootdevice", 1, 6000);
    m_CtrlPointHolder->m_CtrlPoint->Discover(NPT_HttpUrl("239.255.255.250", 1900, "*"), "upnp:rootdevice", 1, 6000);
#endif
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
    CStdString filename;
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), "upnpserver.xml", filename);
    g_settings.LoadUPnPXml(filename);

    // create the server with the friendlyname and UUID from upnpserver.xml if found
    m_ServerHolder->m_Device = new CUPnPServer("XBMC - Media Server",
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
    m_ServerHolder->m_Device->m_ModelDescription = "Xbox Media Center - Media Server";
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

void CUPnP::StartRenderer()
{
    if (!m_RendererHolder->m_Device.IsNull()) return;

    CStdString filename;
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), "upnpserver.xml", filename);
    g_settings.LoadUPnPXml(filename);

    NPT_String ip = g_network.m_networkinfo.ip;
#ifndef HAS_XBOX_NETWORK
    NPT_List<NPT_String> list;
    if (NPT_SUCCEEDED(PLT_UPnPMessageHelper::GetIPAddresses(list))) {
        ip = *(list.GetFirstItem());
    }
#endif

    m_RendererHolder->m_Device = new CUPnPRenderer("XBMC - Media Renderer", true, 
          (g_settings.m_UPnPUUIDRenderer.length() ? g_settings.m_UPnPUUIDRenderer.c_str() : NULL) );

    m_RendererHolder->m_Device->m_PresentationURL = NPT_HttpUrl(ip, atoi(g_guiSettings.GetString("servers.webserverport")), "/").ToString();
    m_RendererHolder->m_Device->m_ModelName = "Xbox Media Center";
    m_RendererHolder->m_Device->m_ModelDescription = "Xbox Media Center - Media Renderer";
    m_RendererHolder->m_Device->m_ModelURL = "http://www.xboxmediacenter.com/";
    m_RendererHolder->m_Device->m_ModelNumber = "2.0";
    m_RendererHolder->m_Device->m_Manufacturer = "Xbox Team";
    m_RendererHolder->m_Device->m_ManufacturerURL = "http://www.xboxmediacenter.com/";

    m_RendererHolder->m_Device->SetBroadcast(broadcast);

    m_UPnP->AddDevice(m_RendererHolder->m_Device);

    // save UUID
    g_settings.m_UPnPUUIDRenderer = m_RendererHolder->m_Device->GetUUID();
    g_settings.SaveUPnPXml(filename);
}

void CUPnP::StopRenderer()
{
    if (m_RendererHolder->m_Device.IsNull()) return;

    m_UPnP->RemoveDevice(m_RendererHolder->m_Device);
    m_RendererHolder->m_Device = NULL;
}

void CUPnP::UpdateState()
{
  if (!m_RendererHolder->m_Device.IsNull())
      ((CUPnPRenderer*)m_RendererHolder->m_Device.AsPointer())->UpdateState();  
}

