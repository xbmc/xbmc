/*****************************************************************
|
|   Platinum - AV Media Server Device
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

#ifndef _PLT_MEDIA_SERVER_H_
#define _PLT_MEDIA_SERVER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptDirectory.h"
#include "PltDeviceHost.h"
#include "PltMediaItem.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define MAX_PATH_LENGTH 1024

/* BrowseFlags */
enum BrowseFlags {
    BROWSEMETADATA,
    BROWSEDIRECTCHILDREN
};

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_FileHttpServerHandler;
class PLT_MetadataHandler;

extern const char* BrowseFlagsStr[];

/*----------------------------------------------------------------------
|   PLT_MediaServer class
+---------------------------------------------------------------------*/
class PLT_MediaServer : public PLT_DeviceHost
{
public:
    PLT_MediaServer(const char*  path, 
                    const char*  friendly_name,
                    bool  show_ip = false,
                    const char*  udn = NULL,
                    unsigned int port = 0);

    // PLT_DeviceHost methods
    virtual NPT_Result Start(PLT_TaskManager* task_manager);
    virtual NPT_Result Stop();
    virtual NPT_Result OnAction(PLT_Action* action, NPT_SocketInfo* info = NULL);

    NPT_Result AddMetadataHandler(PLT_MetadataHandler* handler);

    // overridable methods
    virtual NPT_Result ProcessFileRequest(NPT_HttpRequest* request, NPT_SocketInfo info, NPT_HttpResponse* response);

protected:
    virtual ~PLT_MediaServer();

    static NPT_Result GetBrowseFlag(const char* str, BrowseFlags& flag);
    
    bool       ProceedWithEntry(const NPT_String filepath, NPT_DirectoryEntryType& type);
    NPT_Result GetEntryCount(const char* path, NPT_Cardinal& count); 
    NPT_Result GetFilePath(const char* object_id, NPT_String& filepath);


private:
    // ConnectionManager
    NPT_Result OnGetCurrentConnectionIDs(PLT_Action* action);
    NPT_Result OnGetProtocolInfo(PLT_Action* action);
    NPT_Result OnGetCurrentConnectionInfo(PLT_Action* action);

    // ContentDirectory
    NPT_Result OnGetSortCapabilities(PLT_Action* action);
    NPT_Result OnGetSearchCapabilities(PLT_Action* action);
    NPT_Result OnGetSystemUpdateID(PLT_Action* action);
    NPT_Result OnBrowse(PLT_Action* action);
    NPT_Result OnBrowseMetadata(PLT_Action* action, const char* filepath);
    NPT_Result OnBrowseDirectChildren(PLT_Action* action, const char* filepath);

    NPT_Result OnAlbumArtRequest(NPT_String filepath, NPT_HttpResponse* response);

    NPT_Result BuildFromFilePath(PLT_MediaItem& item, const NPT_String filepath);

protected:
    friend class PLT_MediaItem;

    bool          m_ShowIP;
    NPT_String    m_Path;
    NPT_String    m_DirDelimiter;
    NPT_String    m_FileBaseUri;
    NPT_String    m_AlbumArtBaseUri;

    PLT_FileHttpServerHandler*  m_FileServerHandler;
    PLT_HttpServer*             m_FileServer;

    NPT_List<PLT_MetadataHandler*> m_MetadataHandlers;
};

/*----------------------------------------------------------------------
|   PLT_FileHttpServerHandler
+---------------------------------------------------------------------*/
class PLT_FileHttpServerHandler : public PLT_HttpServerListener
{
public:
    PLT_FileHttpServerHandler(PLT_MediaServer* mediaServer) : m_MediaServer(mediaServer) {}
    virtual ~PLT_FileHttpServerHandler() {}

    // PLT_HttpServerListener methods
    NPT_Result ProcessHttpRequest(NPT_HttpRequest* request, NPT_SocketInfo info, NPT_HttpResponse*& response) {
        response = new NPT_HttpResponse(200, "OK", "HTTP/1.1");
        response->GetHeaders().SetHeader("Server", "Platinum");

        return m_MediaServer->ProcessFileRequest(request, info, response);
    }

private:
    PLT_MediaServer* m_MediaServer;
};

#endif /* _PLT_MEDIA_SERVER_H_ */
