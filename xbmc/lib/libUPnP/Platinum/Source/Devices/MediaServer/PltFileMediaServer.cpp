/*****************************************************************
|
|   Platinum - File Media Server
|
| Copyright (c) 2004-2008, Plutinosoft, LLC.
| All rights reserved.
| http://www.plutinosoft.com
|
| This program is free software; you can redistribute it and/or
| modify it under the terms of the GNU General Public License
| as published by the Free Software Foundation; either version 2
| of the License, or (at your option) any later version.
|
| OEMs, ISVs, VARs and other distributors that combine and 
| distribute commercially licensed software with Platinum software
| and do not wish to distribute the source code for the commercially
| licensed software under version 2, or (at your option) any later
| version, of the GNU General Public License (the "GPL") must enter
| into a commercial license agreement with Plutinosoft, LLC.
| 
| This program is distributed in the hope that it will be useful,
| but WITHOUT ANY WARRANTY; without even the implied warranty of
| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
| GNU General Public License for more details.
|
| You should have received a copy of the GNU General Public License
| along with this program; see the file LICENSE.txt. If not, write to
| the Free Software Foundation, Inc., 
| 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
| http://www.gnu.org/licenses/gpl-2.0.html
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
|   PLT_FileMediaServer::PLT_FileMediaServer
+---------------------------------------------------------------------*/
PLT_FileMediaServer::PLT_FileMediaServer(const char*  path, 
                                         const char*  friendly_name, 
                                         bool         show_ip     /* = false */, 
                                         const char*  uuid        /* = NULL */, 
                                         NPT_UInt16   port        /* = 0 */,
                                         bool         port_rebind /* = false */) :	
    PLT_MediaServer(friendly_name, 
                    show_ip,
                    uuid, 
                    port,
                    port_rebind),
    m_FilterUnknownOut(false)
{
    /* set up the server root file path */
    m_Path  = path;
    m_Path.TrimRight("/\\");
}

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::~PLT_FileMediaServer
+---------------------------------------------------------------------*/
PLT_FileMediaServer::~PLT_FileMediaServer()
{
}

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::AddMetadataHandler
+---------------------------------------------------------------------*/
NPT_Result 
PLT_FileMediaServer::AddMetadataHandler(PLT_MetadataHandler* handler) 
{
    /* TODO: make sure we don't have a metadata handler 
       already registered for the same extension */

    m_MetadataHandlers.Add(handler);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::SetupDevice
+---------------------------------------------------------------------*/
NPT_Result
PLT_FileMediaServer::SetupDevice()
{
    /* set the base paths for content and album art
       we use "0.0.0.0" for the ip as a placeholder 
       to be replaced by the local interface ip a 
       request is received on 
    */
    m_FileBaseUri     = NPT_HttpUrl("0.0.0.0", GetPort(), "/");
    m_AlbumArtBaseUri = NPT_HttpUrl("0.0.0.0", GetPort(), "/", ALBUMART_QUERY);

    return PLT_MediaServer::SetupDevice();
}

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::ProcessHttpGetRequest
+---------------------------------------------------------------------*/
NPT_Result 
PLT_FileMediaServer::ProcessHttpGetRequest(NPT_HttpRequest&              request, 
                                           const NPT_HttpRequestContext& context,
                                           NPT_HttpResponse&             response)
{
    /* Try to handle file request first */
    NPT_Result res = ProcessFileRequest(request, context, response);

    /* file not found, delegate to host */
    if (response.GetStatusCode() == 404) {
        response.SetStatus(200, "OK");
        return PLT_MediaServer::ProcessHttpGetRequest(request, context, response);
    }

    return res;
}

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::ProcessGetDescription
+---------------------------------------------------------------------*/
NPT_Result 
PLT_FileMediaServer::ProcessGetDescription(NPT_HttpRequest&              request,
                                           const NPT_HttpRequestContext& context,
                                           NPT_HttpResponse&             response)
{
    NPT_String m_OldModelName   = m_ModelName;
    NPT_String m_OldModelNumber = m_ModelNumber;

    /* change some things based on User-Agent header */
    NPT_HttpHeader* user_agent = request.GetHeaders().GetHeader(NPT_HTTP_HEADER_USER_AGENT);
    if (user_agent && user_agent->GetValue().Find("Sonos", 0, true)>=0) {
        /* Force "Rhapsody" so that Sonos doesn't reject us */
        m_ModelName   = "Rhapsody";
        m_ModelNumber = "3.0";
    }

    NPT_Result res = PLT_MediaServer::ProcessGetDescription(request, context, response);

    /* reset back to old values now */
    m_ModelName   = m_OldModelName;
    m_ModelNumber = m_OldModelNumber;

    return res;
}

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::ProcessFileRequest
+---------------------------------------------------------------------*/
NPT_Result 
PLT_FileMediaServer::ProcessFileRequest(NPT_HttpRequest&              request, 
                                        const NPT_HttpRequestContext& context,
                                        NPT_HttpResponse&             response)
{
    NPT_HttpUrlQuery query(request.GetUrl().GetQuery());

    NPT_LOG_FINE("PLT_FileMediaServer::ProcessFileRequest Received Request:");
    PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINE, &request);

    response.GetHeaders().SetHeader("Accept-Ranges", "bytes");

    if (request.GetMethod().Compare("GET") && request.GetMethod().Compare("HEAD")) {
        response.SetStatus(500, "Internal Server Error");
        return NPT_SUCCESS;
    }

    /* Extract file path from url */
    NPT_String file_path;
    NPT_CHECK_LABEL_WARNING(ExtractResourcePath(request.GetUrl(), file_path),  
                            failure);

    /* Serve album art or file */
    if (query.GetField(ALBUMART_QUERY)) {
        NPT_CHECK_WARNING(OnAlbumArtRequest(response, NPT_FilePath::Create(m_Path, file_path)));
        return NPT_SUCCESS;
    } else {
        NPT_CHECK_WARNING(ServeFile(request, context, response, NPT_FilePath::Create(m_Path, file_path)));
        return NPT_SUCCESS;
    }

failure:
    response.SetStatus(404, "File Not Found");
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::ServeFile
+---------------------------------------------------------------------*/
NPT_Result 
PLT_FileMediaServer::ServeFile(NPT_HttpRequest&              request, 
                               const NPT_HttpRequestContext& context,
                               NPT_HttpResponse&             response,
                               const NPT_String&             file_path)
{
    NPT_COMPILER_UNUSED(context);

    NPT_Position start, end;
    PLT_HttpHelper::GetRange(request, start, end);

    NPT_CHECK_WARNING(PLT_FileServer::ServeFile(response,
                                                file_path, 
                                                start, 
                                                end, 
                                                !request.GetMethod().Compare("HEAD")));

    /* Update content type header according to file and context */
    NPT_HttpEntity* entity = response.GetEntity();
    PLT_HttpRequestContext tmp_context(request, context);
    if (entity) entity->SetContentType(PLT_MediaItem::GetMimeType(file_path, &tmp_context));
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::OnAlbumArtRequest
+---------------------------------------------------------------------*/
NPT_Result 
PLT_FileMediaServer::OnAlbumArtRequest(NPT_HttpResponse& response,
                                       NPT_String        file_path)
{
    PLT_MetadataHandler* metadataHandler = NULL;
    char*      caData;
    int        caDataLen;

    /* prevent hackers from accessing files outside of our root */
    if ((file_path.Find("/..") >= 0) || (file_path.Find("\\..") >= 0) || 
        !NPT_File::Exists(file_path)) {
        goto filenotfound;
    }

    /* load the metadata handler and read the cover art */
    NPT_ContainerFind(m_MetadataHandlers, 
                      PLT_MetadataHandlerFinder(NPT_FilePath::FileExtension(file_path)), 
                      metadataHandler);
    if (metadataHandler && 
        NPT_SUCCEEDED(metadataHandler->Load(file_path)) &&
        NPT_SUCCEEDED(metadataHandler->GetCoverArtData(caData, caDataLen))) {

        PLT_HttpHelper::SetContentType(response, "application/octet-stream");
        PLT_HttpHelper::SetBody(response, caData, caDataLen);
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
PLT_FileMediaServer::OnBrowseMetadata(PLT_ActionReference&          action, 
                                      const char*                   object_id, 
                                      const char*                   filter,
                                      NPT_UInt32                    starting_index,
                                      NPT_UInt32                    requested_count,
                                      const NPT_List<NPT_String>&   sort_criteria,
                                      const PLT_HttpRequestContext& context)
{
    NPT_COMPILER_UNUSED(sort_criteria);
    NPT_COMPILER_UNUSED(requested_count);
    NPT_COMPILER_UNUSED(starting_index);
    
    NPT_String didl;
    PLT_MediaObjectReference item;

    /* locate the file from the object ID */
    NPT_String filepath;
    if (NPT_FAILED(GetFilePath(object_id, filepath))) {
        /* error */
        NPT_LOG_WARNING("PLT_FileMediaServer::OnBrowse - ObjectID not found.");
        action->SetError(701, "No Such Object.");
        return NPT_FAILURE;
    }

    /* build the object didl */
    item = BuildFromFilePath(filepath, context, true);
    if (item.IsNull()) return NPT_FAILURE;
    NPT_String tmp;    
    NPT_CHECK_SEVERE(PLT_Didl::ToDidl(*item.AsPointer(), filter, tmp));

    /* add didl header and footer */
    didl = didl_header + tmp + didl_footer;

    NPT_CHECK_SEVERE(action->SetArgumentValue("Result", didl));
    NPT_CHECK_SEVERE(action->SetArgumentValue("NumberReturned", "1"));
    NPT_CHECK_SEVERE(action->SetArgumentValue("TotalMatches", "1"));

    // update ID may be wrong here, it should be the one of the container?
    // TODO: We need to keep track of the overall updateID of the CDS
    NPT_CHECK_SEVERE(action->SetArgumentValue("UpdateId", "1"));

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::OnBrowseDirectChildren
+---------------------------------------------------------------------*/
NPT_Result
PLT_FileMediaServer::OnBrowseDirectChildren(PLT_ActionReference&          action, 
                                            const char*                   object_id, 
                                            const char*                   filter,
                                            NPT_UInt32                    starting_index,
                                            NPT_UInt32                    requested_count,
                                            const NPT_List<NPT_String>&   sort_criteria,
                                            const PLT_HttpRequestContext& context)
{
    NPT_COMPILER_UNUSED(sort_criteria);
    
    /* locate the file from the object ID */
    NPT_String dir;
    if (NPT_FAILED(GetFilePath(object_id, dir))) {
        /* error */
        NPT_LOG_WARNING("ObjectID not found.");
        action->SetError(710, "No Such Container.");
        return NPT_FAILURE;
    }

    /* retrieve the item type */
    NPT_FileInfo info;
    NPT_Result res = NPT_File::GetInfo(dir, &info);
    if (NPT_FAILED(res)) {
        /* Object does not exist */
        NPT_LOG_WARNING_1("BROWSEDIRECTCHILDREN failed for item %s", dir.GetChars());
        action->SetError(701, "No such Object");
        return NPT_FAILURE;
    }

    if (info.m_Type != NPT_FileInfo::FILE_TYPE_DIRECTORY) {
        /* error */
        NPT_LOG_WARNING("BROWSEDIRECTCHILDREN not allowed on an item.");
        action->SetError(710, "No such container");
        return NPT_FAILURE;
    }

    NPT_List<NPT_String> entries;
    res = NPT_File::ListDir(dir, entries, 0, 0);
    if (NPT_FAILED(res)) {
        NPT_LOG_WARNING_1("PLT_FileMediaServer::OnBrowseDirectChildren - failed to open dir %s", (const char*) dir);
        return res;
    }

    unsigned long cur_index = 0;
    unsigned long num_returned = 0;
    unsigned long total_matches = 0;
    NPT_String didl = didl_header;

    PLT_MediaObjectReference item;
    for (NPT_List<NPT_String>::Iterator it = entries.GetFirstItem();
         it;
         ++it) {
        NPT_String filepath = NPT_FilePath::Create(dir, *it);
        
        // verify we want to process this file first
        if (!ProcessFile(filepath)) continue;
        
        /* build item object from file path */
        item = BuildFromFilePath(
            filepath, 
            context,
            true);

        /* generate didl if within range requested */
        if (!item.IsNull()) {
            if ((cur_index >= starting_index) && 
                ((num_returned < requested_count) || (requested_count == 0))) {
                NPT_String tmp;
                NPT_CHECK_SEVERE(PLT_Didl::ToDidl(*item.AsPointer(), filter, tmp));

                didl += tmp;
                num_returned++;
            }
            cur_index++;
            total_matches++;        
        }
    };

    didl += didl_footer;

    NPT_CHECK_SEVERE(action->SetArgumentValue("Result", didl));
    NPT_CHECK_SEVERE(action->SetArgumentValue("NumberReturned", NPT_String::FromInteger(num_returned)));
    NPT_CHECK_SEVERE(action->SetArgumentValue("TotalMatches", NPT_String::FromInteger(total_matches))); // 0 means we don't know how many we have but most browsers don't like that!!
    NPT_CHECK_SEVERE(action->SetArgumentValue("UpdateId", "1"));

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::OnSearchContainer
+---------------------------------------------------------------------*/
NPT_Result
PLT_FileMediaServer::OnSearchContainer(PLT_ActionReference&          action, 
                                       const char*                   object_id, 
                                       const char*                   search_criteria,
                                       const char*                   /* filter */,
                                       NPT_UInt32                    /* starting_index */,
                                       NPT_UInt32                    /* requested_count */,
                                       const NPT_List<NPT_String>&   /* sort_criteria */,
                                       const PLT_HttpRequestContext& /* context */)
{
    /* parse search criteria */

    /* TODO: HACK TO PASS DLNA */
    if (search_criteria && NPT_StringsEqual(search_criteria, "Unknownfieldname")) {
        /* error */
        NPT_LOG_WARNING_1("Unsupported or invalid search criteria %s", search_criteria);
        action->SetError(708, "Unsupported or invalid search criteria");
        return NPT_FAILURE;
    }
    
    /* locate the file from the object ID */
    NPT_String dir;
    if (NPT_FAILED(GetFilePath(object_id, dir))) {
        /* error */
        NPT_LOG_WARNING("ObjectID not found.");
        action->SetError(710, "No Such Container.");
        return NPT_FAILURE;
    }
        
    /* retrieve the item type */
    NPT_FileInfo info;
    NPT_Result res = NPT_File::GetInfo(dir, &info);
    if (NPT_FAILED(res) || (info.m_Type != NPT_FileInfo::FILE_TYPE_DIRECTORY)) {
        /* error */
        NPT_LOG_WARNING("No such container");
        action->SetError(710, "No such container");
        return NPT_FAILURE;
    }
    
    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::GetFilePath
+---------------------------------------------------------------------*/
NPT_Result
PLT_FileMediaServer::GetFilePath(const char* object_id, 
                                 NPT_String& filepath) 
{
    if (!object_id) return NPT_ERROR_INVALID_PARAMETERS;

    filepath = m_Path;

    if (NPT_StringLength(object_id) > 2 || object_id[0]!='0') {
        filepath += (const char*)object_id + (object_id[0]=='0'?1:0);
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::BuildSafeResourceUri
+---------------------------------------------------------------------*/
NPT_String
PLT_FileMediaServer::BuildSafeResourceUri(const NPT_HttpUrl& base_uri, 
                                          const char*        host, 
                                          const char*        file_path)
{
    NPT_String result;
    NPT_HttpUrl uri = base_uri;

    if (host) uri.SetHost(host);

    NPT_String uri_path = uri.GetPath();
    if (!uri_path.EndsWith("/")) uri_path += "/";
    // WMP hack: just prefix with %25/ so we know if something url decodes again
    uri_path += "%25/" + NPT_Uri::PercentEncode(file_path, " !\"<>\\^`{|}?#[]:/", true);
    uri.SetPath(uri_path);

    // 360 hack: force inclusion of port in case it's 80
    return uri.ToStringWithDefaultPort(0);
}

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::ExtractResourcePath
+---------------------------------------------------------------------*/
NPT_Result
PLT_FileMediaServer::ExtractResourcePath(const NPT_HttpUrl& url, NPT_String& file_path)
{
    // Extract uri path from url
    NPT_String uri_path = url.GetPath();
    if (uri_path.StartsWith(m_FileBaseUri.GetPath(), true)) {
        file_path = uri_path.SubString(m_FileBaseUri.GetPath().GetLength());
    } else if (uri_path.StartsWith(m_AlbumArtBaseUri.GetPath(), true)) {
        file_path = uri_path.SubString(m_AlbumArtBaseUri.GetPath().GetLength());
    } else
        return NPT_FAILURE;

    // Detect if server is url decoding paths invalidly
    if(file_path.Left(4) == "%25/") {
        file_path = NPT_Uri::PercentDecode(file_path);
        file_path.Erase(0, 2);
    } else if(file_path.Left(2) == "%/") {
        file_path.Erase(0, 2);
        NPT_LOG_FINE("Client is urldecoding our resource paths");
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::BuildResourceUri
+---------------------------------------------------------------------*/
NPT_String
PLT_FileMediaServer::BuildResourceUri(const NPT_HttpUrl& base_uri, 
                                      const char*        host, 
                                      const char*        file_path)
{
    return BuildSafeResourceUri(base_uri, host, file_path);
}

/*----------------------------------------------------------------------
|   PLT_FileMediaServer::BuildFromFilePath
+---------------------------------------------------------------------*/
PLT_MediaObject*
PLT_FileMediaServer::BuildFromFilePath(const NPT_String&             filepath, 
                                       const PLT_HttpRequestContext& context,
                                       bool                          with_count /* = true */,
                                       bool                          keep_extension_in_title /* = false */)
{
    NPT_String            root = m_Path;
    PLT_MediaItemResource resource;
    PLT_MediaObject*      object = NULL;
    
    NPT_LOG_FINEST_1("Building didl for file '%s'", (const char*)filepath);

    /* retrieve the entry type (directory or file) */
    NPT_FileInfo info; 
    NPT_CHECK_LABEL_FATAL(NPT_File::GetInfo(filepath, &info), failure);

    if (info.m_Type == NPT_FileInfo::FILE_TYPE_REGULAR) {
        object = new PLT_MediaItem();

        /* Set the title using the filename for now */
        object->m_Title = NPT_FilePath::BaseName(filepath, keep_extension_in_title);
        if (object->m_Title.GetLength() == 0) goto failure;

        /* make sure we return something with a valid mimetype */
        if (m_FilterUnknownOut && 
            NPT_StringsEqual(PLT_MediaItem::GetMimeType(filepath, &context), 
                             "application/octet-stream")) {
            goto failure;
        }
        
        /* Set the protocol Info from the extension */
        resource.m_ProtocolInfo = PLT_ProtocolInfo(PLT_MediaItem::GetProtocolInfo(filepath, true, &context));
        if (!resource.m_ProtocolInfo.IsValid())  goto failure;

        /* Set the resource file size */
        resource.m_Size = info.m_Size;
 
        /* format the resource URI */
        NPT_String url = filepath.SubString(root.GetLength()+1);

        // get list of ip addresses
        NPT_List<NPT_IpAddress> ips;
        NPT_CHECK_LABEL_SEVERE(PLT_UPnPMessageHelper::GetIPAddresses(ips), failure);

        /* if we're passed an interface where we received the request from
           move the ip to the top so that it is used for the first resource */
        if (context.GetLocalAddress().GetIpAddress().ToString() != "0.0.0.0") {
            ips.Remove(context.GetLocalAddress().GetIpAddress());
            ips.Insert(ips.GetFirstItem(), context.GetLocalAddress().GetIpAddress());
        }
        NPT_List<NPT_IpAddress>::Iterator ip = ips.GetFirstItem();
        
        /* Look to see if a metadata handler exists for this extension */
        PLT_MetadataHandler* handler = NULL;
        NPT_Result res = NPT_ContainerFind(
            m_MetadataHandlers, 
            PLT_MetadataHandlerFinder(NPT_FilePath::FileExtension(filepath)), 
            handler);
        if (NPT_SUCCEEDED(res) && handler) {
            /* if it failed loading data, reset the metadata handler so we don't use it */
            if (NPT_SUCCEEDED(handler->Load(filepath))) {
                /* replace the title with the one from the Metadata */
                NPT_String newTitle;
                if (handler->GetTitle(newTitle) != NULL) {
                    object->m_Title = newTitle;
                }

                /* assign description */
                handler->GetDescription(object->m_Description.long_description);

                /* assign album art uri if we haven't yet */
                if (object->m_ExtraInfo.album_art_uri.GetLength() == 0) {
                    object->m_ExtraInfo.album_art_uri = 
                        NPT_Uri::PercentEncode(BuildResourceUri(m_AlbumArtBaseUri, ip->ToString(), url), 
                                               NPT_Uri::UnsafeCharsToEncode);
                }

                /* duration */
                handler->GetDuration(resource.m_Duration);

                /* protection */
                handler->GetProtection(resource.m_Protection);
            }
        }
        
        object->m_ObjectClass.type = PLT_MediaItem::GetUPnPClass(filepath, &context);
        
        /* add as many resources as we have interfaces s*/
        while (ip) {
            resource.m_Uri = BuildResourceUri(m_FileBaseUri, ip->ToString(), url);
            object->m_Resources.Add(resource);
            ++ip;
        }
    } else {
        object = new PLT_MediaContainer;

        /* Assign a title for this container */
        if (filepath.Compare(root, true) == 0) {
            object->m_Title = "Root";
        } else {
            object->m_Title = NPT_FilePath::BaseName(filepath, keep_extension_in_title);
            if (object->m_Title.GetLength() == 0) goto failure;
        }

        /* Get the number of children for this container */
        NPT_Cardinal count = 0;
        if (with_count && NPT_SUCCEEDED(NPT_File::GetCount(filepath, count))) {
            ((PLT_MediaContainer*)object)->m_ChildrenCount = count;
        }

        object->m_ObjectClass.type = "object.container.storageFolder";
    }

    /* is it the root? */
    if (filepath.Compare(root, true) == 0) {
        object->m_ParentID = "-1";
        object->m_ObjectID = "0";
    } else {
        NPT_String directory = NPT_FilePath::DirName(filepath);
        /* is the parent path the root? */
        if (directory.GetLength() == root.GetLength()) {
            object->m_ParentID = "0";
        } else {
            object->m_ParentID = "0" + filepath.SubString(root.GetLength(), directory.GetLength() - root.GetLength());
        }
        object->m_ObjectID = "0" + filepath.SubString(root.GetLength());
    }

    return object;

failure:
    delete object;
    return NULL;
}
