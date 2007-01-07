/*****************************************************************
|
|   Platinum - File Media Server
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

#ifndef _PLT_FILE_MEDIA_SERVER_H_
#define _PLT_FILE_MEDIA_SERVER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
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
                        unsigned int port = 0);

    // PLT_DeviceHost methods
    virtual NPT_Result Start(PLT_TaskManager* task_manager);
    virtual NPT_Result Stop();

    NPT_Result AddMetadataHandler(PLT_MetadataHandler* handler);

protected:
    virtual ~PLT_FileMediaServer();
    
protected:
    NPT_Result OnBrowseMetadata(PLT_ActionReference& action, const char* object_id, NPT_SocketInfo* info = NULL);
    NPT_Result OnBrowseDirectChildren(PLT_ActionReference& action, const char* object_id, NPT_SocketInfo* info = NULL);
    NPT_Result ProcessFileRequest(NPT_HttpRequest* request, NPT_SocketInfo info, NPT_HttpResponse*& response);

private:
    NPT_Result OnAlbumArtRequest(NPT_String filepath, NPT_HttpResponse* response);
    PLT_MediaObject* BuildFromFilePath(const NPT_String& filepath, bool with_count = true, NPT_SocketInfo* info = NULL);
    bool       ProceedWithEntry(const NPT_String filepath, NPT_DirectoryEntryType& type);
    NPT_Result GetEntryCount(const char* path, NPT_Cardinal& count); 
    NPT_Result GetFilePath(const char* object_id, NPT_String& filepath);

protected:
    friend class PLT_MediaItem;

    NPT_String    m_Path;
    NPT_String    m_DirDelimiter;
    NPT_String    m_FileBaseUri;
    NPT_String    m_AlbumArtBaseUri;

    NPT_List<PLT_MetadataHandler*> m_MetadataHandlers;
};

#endif /* _PLT_FILE_MEDIA_SERVER_H_ */
