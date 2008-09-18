/*****************************************************************
|
|   Platinum - File Media Server
|
|   Copyright (c) 2004-2008 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
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
                        NPT_UInt16   port = 0,
                        NPT_UInt16   fileserver_port = 0);

    NPT_Result AddMetadataHandler(PLT_MetadataHandler* handler);

    // overridable
    virtual NPT_Result ProcessFileRequest(NPT_HttpRequest&              request, 
                                          const NPT_HttpRequestContext& context,
                                          NPT_HttpResponse&             response);
                                          
protected:
    virtual ~PLT_FileMediaServer();

    // PLT_DeviceHost methods
    virtual NPT_Result Start(PLT_SsdpListenTask* task);
    virtual NPT_Result Stop(PLT_SsdpListenTask* task);
    
    // PLT_MediaServer methods
    virtual NPT_Result OnBrowseMetadata(PLT_ActionReference&          action, 
                                        const char*                   object_id, 
                                        const NPT_HttpRequestContext& context);
    virtual NPT_Result OnBrowseDirectChildren(PLT_ActionReference&          action, 
                                              const char*                   object_id, 
                                              const NPT_HttpRequestContext& context);

    // protected methods
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
    PLT_HttpServer*                m_FileServer;
    NPT_HttpRequestHandler*        m_FileServerHandler;
    NPT_List<PLT_MetadataHandler*> m_MetadataHandlers;
};

#endif /* _PLT_FILE_MEDIA_SERVER_H_ */
