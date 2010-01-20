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

#ifndef _PLT_FILE_MEDIA_SERVER_H_
#define _PLT_FILE_MEDIA_SERVER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltMediaServer.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define MAX_PATH_LENGTH 1024
#define ALBUMART_QUERY  "aa"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_MetadataHandler;

/*----------------------------------------------------------------------
|   PLT_FileMediaServer class
+---------------------------------------------------------------------*/
class PLT_FileMediaServer : public PLT_MediaServer
{
public:
    // class methods
    static NPT_String BuildSafeResourceUri(const NPT_HttpUrl& base_uri, 
                                           const char*        host, 
                                           const char*        file_path);

    // constructor
    PLT_FileMediaServer(const char*  path, 
                        const char*  friendly_name,
                        bool         show_ip = false,
                        const char*  uuid = NULL,
                        NPT_UInt16   port = 0,
                        bool         port_rebind = false);

    // overridable
    virtual NPT_Result AddMetadataHandler(PLT_MetadataHandler* handler);
    virtual NPT_Result ExtractResourcePath(const NPT_HttpUrl& url, NPT_String& file_path);
    virtual NPT_String BuildResourceUri(const NPT_HttpUrl& base_uri, const char* host, const char* file_path);

protected:
    virtual ~PLT_FileMediaServer();

    // overridable
    virtual NPT_Result ProcessFileRequest(NPT_HttpRequest&              request, 
                                          const NPT_HttpRequestContext& context,
                                          NPT_HttpResponse&             response);
    // PLT_DeviceHost methods
    virtual NPT_Result SetupDevice();
    virtual NPT_Result ProcessHttpGetRequest(NPT_HttpRequest&              request, 
                                             const NPT_HttpRequestContext& context,
                                             NPT_HttpResponse&             response);
    virtual NPT_Result ProcessGetDescription(NPT_HttpRequest&              request,
                                             const NPT_HttpRequestContext& context,
                                             NPT_HttpResponse&             response);
    // PLT_MediaServer methods
    virtual NPT_Result OnBrowseMetadata(PLT_ActionReference&          action, 
                                        const char*                   object_id, 
                                        const char*                   filter,
                                        NPT_UInt32                    starting_index,
                                        NPT_UInt32                    requested_count,
                                        const NPT_List<NPT_String>&   sort_criteria,
                                        const PLT_HttpRequestContext& context);
    virtual NPT_Result OnBrowseDirectChildren(PLT_ActionReference&          action, 
                                              const char*                   object_id, 
                                              const char*                   filter,
                                              NPT_UInt32                    starting_index,
                                              NPT_UInt32                    requested_count,
                                              const NPT_List<NPT_String>&   sort_criteria,
                                              const PLT_HttpRequestContext& context);
    virtual NPT_Result OnSearchContainer(PLT_ActionReference&          action, 
                                         const char*                   object_id, 
                                         const char*                   search_criteria,
                                         const char*                   filter,
                                         NPT_UInt32                    starting_index,
                                         NPT_UInt32                    requested_count,
                                         const NPT_List<NPT_String>&   sort_criteria, 
                                         const PLT_HttpRequestContext& context);
                                
    virtual NPT_Result ServeFile(NPT_HttpRequest&              request, 
                                 const NPT_HttpRequestContext& context,
                                 NPT_HttpResponse&             response,
                                 const NPT_String&             file_path);
    virtual NPT_Result OnAlbumArtRequest(NPT_HttpResponse& response, 
                                         NPT_String        file_path);
    virtual NPT_Result GetFilePath(const char* object_id, NPT_String& filepath);
    virtual bool       ProcessFile(const NPT_String&) { return true;}
    virtual PLT_MediaObject* BuildFromFilePath(const NPT_String&             filepath, 
                                               const PLT_HttpRequestContext& context,
                                               bool                          with_count = true,
                                               bool                          keep_extension_in_title = false);

public:
    NPT_UInt16 m_FileServerPort;

protected:
    friend class PLT_MediaItem;

    NPT_String                     m_Path;
    NPT_HttpUrl                    m_FileBaseUri;
    NPT_HttpUrl                    m_AlbumArtBaseUri;
    NPT_List<PLT_MetadataHandler*> m_MetadataHandlers;
    bool                           m_FilterUnknownOut;
};

#endif /* _PLT_FILE_MEDIA_SERVER_H_ */
