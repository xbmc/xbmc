/*****************************************************************
|
|   Platinum - AV Media Server Device
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltUPnP.h"
#include "PltFileMediaServer.h"
#include "PltMediaItem.h"
#include "PltService.h"
#include "PltTaskManager.h"
#include "PltHttpServer.h"
#include "PltDidl.h"
#include "PltMetadataHandler.h"
#include "PltVersion.h"

NPT_SET_LOCAL_LOGGER("platinum.media.server.file")

/*----------------------------------------------------------------------
|   PLT_HttpFileRequestHandler
+---------------------------------------------------------------------*/
class PLT_HttpFileRequestHandler : public NPT_HttpRequestHandler
{
public:
    PLT_HttpFileRequestHandler(PLT_FileMediaServer* file_server) : m_FileServer(file_server) {}
    virtual ~PLT_HttpFileRequestHandler() {}

    // NPT_HttpRequestHandler methods
    NPT_Result SetupResponse(NPT_HttpRequest&  request, 
                             NPT_HttpResponse& response, 
                             NPT_SocketInfo&   client_info) {
        return m_FileServer->ProcessFileRequest(request, response, client_info);
    }

private:
    PLT_FileMediaServer* m_FileServer;
};

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::PLT_FileMediaServer
+---------------------------------------------------------------------*/
PLT_FileMediaServer::PLT_FileMediaServer(const char*  path, 
                                         const char*  friendly_name, 
                                         bool         show_ip, 
                                         const char*  uuid, 
                                         unsigned int port,
                                         unsigned int file_serverport) :	
    PLT_MediaServer(friendly_name, show_ip, uuid, port, file_serverport)
{
    /* set up the server root path */
    m_Path  = path;
    if (m_Path.Find(NPT_WIN32_DIR_DELIMITER_STR) != -1) {
        m_DirDelimiter = NPT_WIN32_DIR_DELIMITER_STR;
    } else if (m_Path.Find(NPT_UNIX_DIR_DELIMITER_STR) != -1) {
        m_DirDelimiter = NPT_UNIX_DIR_DELIMITER_STR;
    } else {
        m_DirDelimiter = NPT_UNIX_DIR_DELIMITER_STR;
    }

    if (!m_Path.EndsWith(m_DirDelimiter)) {
        m_Path += m_DirDelimiter;
    }

    m_FileServerHandler = new PLT_HttpFileRequestHandler(this);
    m_FileServer->AddRequestHandler(m_FileServerHandler, "/", true);
}

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::~PLT_FileMediaServer
+---------------------------------------------------------------------*/
PLT_FileMediaServer::~PLT_FileMediaServer()
{
    delete m_FileServerHandler;
}

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::AddMetadataHandler
+---------------------------------------------------------------------*/
NPT_Result 
PLT_FileMediaServer::AddMetadataHandler(PLT_MetadataHandler* handler) 
{
    // make sure we don't have a metadatahandler registered for the same extension
//    PLT_MetadataHandler* prev_handler;
//    NPT_Result ret = NPT_ContainerFind(m_MetadataHandlers, PLT_MetadataHandlerFinder(handler->GetExtension()), prev_handler);
//    if (NPT_SUCCEEDED(ret)) {
//        return NPT_ERROR_INVALID_PARAMETERS;
//    }

    m_MetadataHandlers.Add(handler);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::Start
+---------------------------------------------------------------------*/
NPT_Result
PLT_FileMediaServer::Start(PLT_TaskManager* task_manager)
{
    NPT_CHECK_SEVERE(PLT_MediaServer::Start(task_manager));

    // FIXME: hack for now: find the first valid non local ip address
    // to use in item resources. TODO: we should advertise all ips as
    // multiple resources instead.
    NPT_List<NPT_String> ips;
    PLT_UPnPMessageHelper::GetIPAddresses(ips);
    if (ips.GetItemCount() == 0) return NPT_ERROR_INTERNAL;

    // set the base paths for content and album arts
    m_FileBaseUri     = NPT_HttpUrl(*ips.GetFirstItem(), m_FileServer->GetPort(), "/content");
    m_AlbumArtBaseUri = NPT_HttpUrl(*ips.GetFirstItem(), m_FileServer->GetPort(), "/albumart");

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::ProcessFileRequest
+---------------------------------------------------------------------*/
NPT_Result 
PLT_FileMediaServer::ProcessFileRequest(NPT_HttpRequest&  request, 
                                        NPT_HttpResponse& response, 
                                        NPT_SocketInfo&   client_info)
{
    NPT_COMPILER_UNUSED(client_info);

    NPT_LOG_FINE("PLT_FileMediaServer::ProcessFileRequest Received Request:");
    PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINE, &request);

    response.GetHeaders().SetHeader("Accept-Ranges", "bytes");

    if (request.GetMethod().Compare("GET") && request.GetMethod().Compare("HEAD")) {
        response.SetStatus(500, "Internal Server Error");
        return NPT_SUCCESS;
    }

    // File requested
    NPT_String path = m_FileBaseUri.GetPath();
    NPT_String strUri = NPT_Uri::Decode(request.GetUrl().GetPath());

    NPT_HttpUrlQuery query(request.GetUrl().GetQuery());
    NPT_String file_path = query.GetField("path");
    if (file_path.GetLength() == 0) goto failure;

    if (path.Compare(strUri.Left(path.GetLength()), true) == 0) {
        NPT_Integer start, end;
        PLT_HttpHelper::GetRange(&request, start, end);

        return PLT_FileServer::ServeFile(m_Path + file_path, &response, start, end, !request.GetMethod().Compare("HEAD"));
    } 

    // Album Art requested
    path = m_AlbumArtBaseUri.GetPath();
    if (path.Compare(strUri.Left(path.GetLength()), true) == 0) {
        return OnAlbumArtRequest(m_Path + file_path, response);
    } 

failure:
    response.SetStatus(404, "File Not Found");
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::OnAlbumArtRequest
+---------------------------------------------------------------------*/
NPT_Result 
PLT_FileMediaServer::OnAlbumArtRequest(NPT_String filepath, NPT_HttpResponse& response)
{
    NPT_Size total_len;
    NPT_File file(filepath);
    NPT_InputStreamReference stream;

    if (NPT_FAILED(file.Open(NPT_FILE_OPEN_MODE_READ)) || NPT_FAILED(file.GetInputStream(stream)) || 
        NPT_FAILED(stream->GetSize(total_len)) || (total_len == 0)) {
        goto filenotfound;
    } else {
        const char* extension = PLT_MediaItem::GetExtFromFilePath(filepath, m_DirDelimiter);
        if (extension == NULL) {
            goto filenotfound;
        }

        PLT_MetadataHandler* metadataHandler = NULL;
        char* caData;
        int   caDataLen;
        NPT_Result ret = NPT_ContainerFind(m_MetadataHandlers, PLT_MetadataHandlerFinder(extension), metadataHandler);
        if (NPT_FAILED(ret) || metadataHandler == NULL) {
            goto filenotfound;
        }
        // load the metadatahandler and read the cover art
        if (NPT_FAILED(metadataHandler->Load(*stream)) || NPT_FAILED(metadataHandler->GetCoverArtData(caData, caDataLen))) {
            goto filenotfound;
        }
        PLT_HttpHelper::SetContentType(&response, "application/octet-stream");
        PLT_HttpHelper::SetBody(&response, caData, caDataLen);
        delete caData;
        return NPT_SUCCESS;
    }

filenotfound:
    response.SetStatus(404, "File Not Found");
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::OnBrowseMetadata
+---------------------------------------------------------------------*/
NPT_Result
PLT_FileMediaServer::OnBrowseMetadata(PLT_ActionReference& action, 
                                      const char*          object_id, 
                                      NPT_SocketInfo*      info /* = NULL */)
{
    NPT_String didl;

    /* locate the file from the object ID */
    NPT_String filepath;
    if (NPT_FAILED(GetFilePath(object_id, filepath))) {
        /* error */
        NPT_LOG_WARNING("PLT_FileMediaServer::OnBrowse - ObjectID not found.");
        action->SetError(701, "No Such Object.");
        return NPT_FAILURE;
    }

    NPT_Reference<PLT_MediaObject> item(BuildFromFilePath(filepath, true, info));
    if (item.IsNull()) return NPT_FAILURE;

    NPT_String filter;
    NPT_CHECK_SEVERE(action->GetArgumentValue("Filter", filter));

    NPT_String tmp;    
    NPT_CHECK_SEVERE(PLT_Didl::ToDidl(*item.AsPointer(), filter, tmp));

    /* add didl header and footer */
    didl = didl_header + tmp + didl_footer;

    NPT_CHECK_SEVERE(action->SetArgumentValue("Result", didl));
    NPT_CHECK_SEVERE(action->SetArgumentValue("NumberReturned", "1"));
    NPT_CHECK_SEVERE(action->SetArgumentValue("TotalMatches", "1"));

    // update ID may be wrong here, it should be the one of the container?
    NPT_CHECK_SEVERE(action->SetArgumentValue("UpdateId", "1"));
    // TODO: We need to keep track of the overall updateID of the CDS

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::OnBrowseDirectChildren
+---------------------------------------------------------------------*/
NPT_Result
PLT_FileMediaServer::OnBrowseDirectChildren(PLT_ActionReference& action, 
                                            const char*          object_id, 
                                            NPT_SocketInfo*      info /* = NULL */)
{
    /* locate the file from the object ID */
    NPT_String dir;
    if (NPT_FAILED(GetFilePath(object_id, dir))) {
        /* error */
        NPT_LOG_WARNING("PLT_FileMediaServer::OnBrowse - ObjectID not found.");
        action->SetError(701, "No Such Object.");
        return NPT_FAILURE;
    }

    /* retrieve the item type */
    NPT_DirectoryEntryInfo entry_info;
    NPT_Result res = NPT_DirectoryEntry::GetInfo(dir, entry_info);
    if (NPT_FAILED(res)) {
        /* Object does not exist */
        action->SetError(800, "Can't retrieve info " + dir);
        return NPT_FAILURE;
    }

    if (entry_info.type != NPT_DIRECTORY_TYPE) {
        /* error */
        NPT_LOG_WARNING("PLT_FileMediaServer::OnBrowse - BROWSEDIRECTCHILDREN not allowed on an item.");
        action->SetError(710, "item is not a container.");
        return NPT_FAILURE;
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

    NPT_String path = dir;
    if (!path.EndsWith(m_DirDelimiter)) {
        path += m_DirDelimiter;
    }

    /* start iterating through the directory */
    NPT_Directory directory(path);
    NPT_String    entryName;
    res = directory.GetNextEntry(entryName);
    if (NPT_FAILED(res)) {
        NPT_LOG_WARNING_1("PLT_FileMediaServer::OnBrowseDirectChildren - failed to open dir %s", (const char*) path);
        return res;
    }

    unsigned long cur_index = 0;
    unsigned long num_returned = 0;
    unsigned long total_matches = 0;
    //unsigned long update_id = 0;
    NPT_String didl = didl_header;
    PLT_MediaObjectReference item;
    do {
        item = BuildFromFilePath(path + entryName, true, info);
        if (!item.IsNull()) {
            if ((cur_index >= start_index) && ((num_returned < req_count) || (req_count == 0))) {
                NPT_String tmp;
                NPT_CHECK_SEVERE(PLT_Didl::ToDidl(*item.AsPointer(), filter, tmp));

                didl += tmp;
                num_returned++;
            }
            cur_index++;
            total_matches++;        
        }
        res = directory.GetNextEntry(entryName);
    } while (NPT_SUCCEEDED(res));

    didl += didl_footer;

    NPT_CHECK_SEVERE(action->SetArgumentValue("Result", didl));

    NPT_CHECK_SEVERE(action->SetArgumentValue("NumberReturned", NPT_String::FromInteger(num_returned)));

    NPT_CHECK_SEVERE(action->SetArgumentValue("TotalMatches", NPT_String::FromInteger(total_matches)));

    NPT_CHECK_SEVERE(action->SetArgumentValue("UpdateId", "1"));

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::GetFilePath
+---------------------------------------------------------------------*/
NPT_Result
PLT_FileMediaServer::GetFilePath(const char* object_id, NPT_String& filepath) 
{
    if (!object_id) return NPT_ERROR_INVALID_PARAMETERS;

    filepath = m_Path;

    if (NPT_StringLength(object_id) > 2 || object_id[0]!='0') {
        filepath += (const char*)object_id + (object_id[0]=='0'?2:0);
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::ProceedWithEntry
+---------------------------------------------------------------------*/
bool
PLT_FileMediaServer::ProceedWithEntry(const NPT_String filepath, NPT_DirectoryEntryInfo& info)
{
    /* make sure this is a valid entry */
    if (filepath.EndsWith(m_DirDelimiter + ".") || filepath.EndsWith(m_DirDelimiter + "..")) {
        return false;
    }

    /* retrieve the entry type (directory or file) */
    if (NPT_FAILED(NPT_DirectoryEntry::GetInfo(filepath, info))) {
        return false;
    }

    /* we could add restrictions here */
    return true;
}

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::BuildFromFilePath
+---------------------------------------------------------------------*/
PLT_MediaObject*
PLT_FileMediaServer::BuildFromFilePath(const NPT_String& filepath, 
                                       bool              with_count /* = true */,
                                       NPT_SocketInfo*   info /* = NULL */,
                                       bool              keep_extension_in_title /* = false */)
{
    NPT_String            delimiter = m_DirDelimiter;
    NPT_String            root = m_Path;
    PLT_MediaItemResource resource;
    PLT_MediaObject*      object = NULL;
    int                   dir_delim_index;

    /* make sure this is a valid entry */
    /* and retrieve the entry type (directory or file) */
    NPT_DirectoryEntryInfo entry_info; 
    if (!ProceedWithEntry(filepath, entry_info)) goto failure;

    /* find the last directory delimiter */
    dir_delim_index = filepath.ReverseFind(delimiter);
    if (dir_delim_index < 0) goto failure;

    if (entry_info.type == NPT_FILE_TYPE) {
        object = new PLT_MediaItem();

        /* we need a valid extension to retrieve the mimetype for the protocol info */
        int ext_index = filepath.ReverseFind('.');
        if (ext_index <= 0 || ext_index < dir_delim_index) {
            ext_index = filepath.GetLength();
        }

        /* Set the title using the filename for now */
        object->m_Title = filepath.SubString(dir_delim_index+1, 
            keep_extension_in_title?filepath.GetLength():ext_index - dir_delim_index -1);
        if (object->m_Title.GetLength() == 0) goto failure;

        /* Set the protocol Info from the extension */
        const char* ext = ((const char*)filepath) + ext_index;
        resource.m_ProtocolInfo = PLT_MediaItem::GetProtInfoFromExt(ext);
        if (resource.m_ProtocolInfo.GetLength() == 0)  goto failure;

        /* Set the resource file size */
        resource.m_Size = entry_info.size;
 
        /* format the resource URI */
        NPT_String url = filepath.SubString(root.GetLength());

        // get list of ip addresses
        NPT_List<NPT_String> ips;
        NPT_CHECK_LABEL_SEVERE(PLT_UPnPMessageHelper::GetIPAddresses(ips), failure);

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
            query.AddField("path", url);
            uri.SetHost(*ip);
            uri.SetQuery(query.ToString());
            //uri.SetPath(uri.GetPath() + url);

            /* prepend the base URI and url encode it */ 
            //resource.m_Uri = NPT_Uri::Encode(uri.ToString(), NPT_Uri::UnsafeCharsToEncode);
            resource.m_Uri = uri.ToString();

            /* Look to see if a metadatahandler exists for this extension */
            PLT_MetadataHandler* handler = NULL;
            NPT_Result res = NPT_ContainerFind(m_MetadataHandlers, PLT_MetadataHandlerFinder(ext), handler);
            if (NPT_SUCCEEDED(res) && handler) {
                /* if it failed loading data, reset the metadatahandler so we don't use it */
                if (NPT_SUCCEEDED(handler->LoadFile(filepath))) {
                    /* replace the title with the one from the Metadata */
                    NPT_String newTitle;
                    if (handler->GetTitle(newTitle) != NULL) {
                        object->m_Title = newTitle;
                    }

                    /* assign description */
                    handler->GetDescription(object->m_Description.long_description);

                    /* assign album art uri if we haven't yet */
                    /* prepend the album art base URI and url encode it */ 
                    if (object->m_ExtraInfo.album_art_uri.GetLength() == 0) {
                        NPT_HttpUrl uri = m_AlbumArtBaseUri;
                        NPT_HttpUrlQuery query;
                        query.AddField("path", url);
                        uri.SetHost(*ip);
                        uri.SetQuery(query.ToString());
                        //uri.SetPath(uri.GetPath() + url);

                        object->m_ExtraInfo.album_art_uri = NPT_Uri::Encode(uri.ToString(), NPT_Uri::UnsafeCharsToEncode);
                    }

                    /* duration */
                    handler->GetDuration((NPT_UInt32&)resource.m_Duration);

                    /* protection */
                    handler->GetProtection(resource.m_Protection);
                }
            }

            object->m_ObjectClass.type = PLT_MediaItem::GetUPnPClassFromExt(ext);
            object->m_Resources.Add(resource);

            ++ip;
        }
    } else {
        object = new PLT_MediaContainer;

        /* Assign a title for this container */
        if (filepath.Compare(root, true) == 0) {
            object->m_Title = "Root";
        } else {
            object->m_Title = filepath.SubString(dir_delim_index+1, filepath.GetLength() - dir_delim_index -1);
            if (object->m_Title.GetLength() == 0) goto failure;
        }

        /* Get the number of children for this container */
        if (with_count) {
            NPT_Cardinal count = 0;
            NPT_CHECK_LABEL_SEVERE(GetEntryCount(filepath, count), failure);
            ((PLT_MediaContainer*)object)->m_ChildrenCount = count;
        }

        object->m_ObjectClass.type = "object.container";
    }

    /* is it the root? */
    if (filepath.Compare(root, true) == 0) {
        object->m_ParentID = "-1";
        object->m_ObjectID = "0";
    } else {
        /* is the parent path the root? */
        if (dir_delim_index == (int)root.GetLength() - 1) {
            object->m_ParentID = "0";
        } else {
            object->m_ParentID = "0" + delimiter + filepath.SubString(root.GetLength(), dir_delim_index - root.GetLength());
        }
        object->m_ObjectID = "0" + delimiter + filepath.SubString(root.GetLength());
    }

    return object;

failure:
    delete object;
    return NULL;
}

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::GetEntryCount
+---------------------------------------------------------------------*/
NPT_Result
PLT_FileMediaServer::GetEntryCount(const char* path, NPT_Cardinal& count) 
{
    NPT_String dir_path = path;

    // reset output params
    count = 0;

    // ensure path ends with a delimiter
    if (!dir_path.EndsWith(m_DirDelimiter)) {
        dir_path += m_DirDelimiter;
    }

    NPT_Directory directory(dir_path);
    NPT_String entryName;
    NPT_Result res = directory.GetNextEntry(entryName);
    if (NPT_FAILED(res)) {
        NPT_LOG_WARNING_1("PLT_FileMediaServer::OnBrowseDirectChildren - failed to open dir %s", (const char*) path);
        return res;
    }

    do {
        /* Check if the item would be ok to add to a didl */
//         NPT_Reference<PLT_MediaObject> item(BuildFromFilePath(dir_path + entryName, false));
//         if (!item.IsNull()) {
//             count++;
//         }
        NPT_DirectoryEntryInfo info; 
        if (ProceedWithEntry(dir_path + entryName, info)) count++;
        res = directory.GetNextEntry(entryName);
    } while (NPT_SUCCEEDED(res));

    return NPT_SUCCESS;
}
