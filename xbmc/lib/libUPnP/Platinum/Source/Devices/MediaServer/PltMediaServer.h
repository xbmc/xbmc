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
#include "Neptune.h"
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
extern const char* BrowseFlagsStr[];
class PLT_HttpFileServerHandler;

/*----------------------------------------------------------------------
|   PLT_MediaServer class
+---------------------------------------------------------------------*/
class PLT_MediaServer : public PLT_DeviceHost
{
public:
    // PLT_DeviceHost methods
    virtual NPT_Result Start(PLT_TaskManager* task_manager, PLT_DeviceHostReference& self);
    virtual NPT_Result Stop();
    virtual NPT_Result OnAction(PLT_ActionReference& action, NPT_SocketInfo* info = NULL);

protected:
    PLT_MediaServer(const char*  friendly_name,
                    bool         show_ip = false,
                    const char*  uuid = NULL,
                    unsigned int port = 0,
                    unsigned int fileserver_port = 0);
    virtual ~PLT_MediaServer();

    // class methods
    static NPT_Result GetBrowseFlag(const char* str, BrowseFlags& flag);

    // ConnectionManager
    virtual NPT_Result OnGetCurrentConnectionIDs(PLT_ActionReference& action, NPT_SocketInfo* info = NULL);
    virtual NPT_Result OnGetProtocolInfo(PLT_ActionReference& action, NPT_SocketInfo* info = NULL);
    virtual NPT_Result OnGetCurrentConnectionInfo(PLT_ActionReference& action, NPT_SocketInfo* info = NULL);

    // ContentDirectory
    virtual NPT_Result OnGetSortCapabilities(PLT_ActionReference& action, NPT_SocketInfo* info = NULL);
    virtual NPT_Result OnGetSearchCapabilities(PLT_ActionReference& action, NPT_SocketInfo* info = NULL);
    virtual NPT_Result OnGetSystemUpdateID(PLT_ActionReference& action, NPT_SocketInfo* info = NULL);
    virtual NPT_Result OnBrowse(PLT_ActionReference& action, NPT_SocketInfo* info = NULL);
    virtual NPT_Result OnSearch(PLT_ActionReference& action, NPT_SocketInfo* info = NULL);

    // X_MS_MediaReceiverRegistrar
    virtual NPT_Result OnIsAuthorized(PLT_ActionReference& action, NPT_SocketInfo* info = NULL);
    virtual NPT_Result OnIsValidated(PLT_ActionReference& action, NPT_SocketInfo* info = NULL);
    virtual NPT_Result OnRegisterDevice(PLT_ActionReference& action, NPT_SocketInfo* info = NULL);

    // overridable methods
    virtual NPT_Result OnBrowseMetadata(PLT_ActionReference& action, const char* object_id, NPT_SocketInfo* info = NULL);
    virtual NPT_Result OnBrowseDirectChildren(PLT_ActionReference& action, const char* object_id, NPT_SocketInfo* info = NULL);
    virtual NPT_Result OnSearch(PLT_ActionReference& action, const NPT_String& object_id, const NPT_String& searchCriteria, NPT_SocketInfo* info = NULL);


    PLT_HttpServer* m_FileServer;
};

#endif /* _PLT_MEDIA_SERVER_H_ */
