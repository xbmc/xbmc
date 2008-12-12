/*****************************************************************
|
|   Platinum - File Media Server
|
|   Copyright (c) 2004-2008, Plutinosoft, LLC.
|   Author: Sylvain Rebaud (sylvain@plutinosoft.com)
|
 ****************************************************************/

#ifndef _PLT_FILE_MEDIA_SERVER_H_
#define _PLT_FILE_MEDIA_SERVER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "NptDirectory.h"
#include "PltMediaServer.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define MAX_PATH_LENGTH 1024

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_MetadataHandler;

/*----------------------------------------------------------------------
|   PLT_MediaServer class
+---------------------------------------------------------------------*/
class PLT_FileMediaServer : public PLT_MediaServer
{
public:
    PLT_FileMediaServer(const char*  path, 
                        const char*  friendly_name,
                        bool         show_ip = false,
                        const char*  uuid = NULL,
                        NPT_UInt16   port = 0);

    NPT_Result AddMetadataHandler(PLT_MetadataHandler* handler);
    static NPT_String BuildResourceUri(const NPT_HttpUrl& base_uri, const char* host, const char* file_path);

protected:
    virtual ~PLT_FileMediaServer();

    // overridable
    virtual NPT_Result ProcessFileRequest(NPT_HttpRequest&              request, 
                                          const NPT_HttpRequestContext& context,
                                          NPT_HttpResponse&             response);
    // PLT_DeviceHost methods
    virtual NPT_Result SetupDevice();
    virtual NPT_Result ProcessHttpRequest(NPT_HttpRequest&              request, 
                                          const NPT_HttpRequestContext& context,
                                          NPT_HttpResponse&             response);
    virtual NPT_Result ProcessGetDescription(NPT_HttpRequest&              request,
                                             const NPT_HttpRequestContext& context,
                                             NPT_HttpResponse&             response);
    // PLT_MediaServer methods
    virtual NPT_Result OnBrowseMetadata(PLT_ActionReference&          action, 
                                        const char*                   object_id, 
                                        const NPT_HttpRequestContext& context);
    virtual NPT_Result OnBrowseDirectChildren(PLT_ActionReference&          action, 
                                              const char*                   object_id, 
                                              const NPT_HttpRequestContext& context);
    virtual NPT_Result ServeFile(NPT_HttpRequest&              request, 
                                 const NPT_HttpRequestContext& context,
                                 NPT_HttpResponse&             response,
                                 NPT_String                    uri_path,
                                 NPT_String                    file_path);
    virtual NPT_Result OnAlbumArtRequest(NPT_HttpResponse& response, 
                                         NPT_String        file_path);
    virtual bool       ProceedWithEntry(const NPT_String        filepath, 
                                        NPT_DirectoryEntryInfo& info);
    virtual NPT_Result GetEntryCount(const char* path, NPT_Cardinal& count); 
    virtual NPT_Result GetFilePath(const char* object_id, NPT_String& filepath);

    virtual PLT_MediaObject* BuildFromFilePath(const NPT_String&        filepath, 
                                               bool                     with_count = true, 
                                               const NPT_SocketAddress* req_local_address = NULL, 
                                               bool                     keep_extension_in_title = false);

public:
    NPT_UInt16                     m_FileServerPort;

protected:
    friend class PLT_MediaItem;

    NPT_String                     m_Path;
    NPT_String                     m_DirDelimiter;
    NPT_HttpUrl                    m_FileBaseUri;
    NPT_HttpUrl                    m_AlbumArtBaseUri;
    NPT_List<PLT_MetadataHandler*> m_MetadataHandlers;
};

#endif /* _PLT_FILE_MEDIA_SERVER_H_ */
