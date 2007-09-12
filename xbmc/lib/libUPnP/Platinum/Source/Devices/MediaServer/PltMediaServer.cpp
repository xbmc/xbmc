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
#include "PltMediaServer.h"
#include "PltMediaItem.h"
#include "PltService.h"
#include "PltTaskManager.h"
#include "PltHttpServer.h"
#include "PltDidl.h"
#include "PltMetadataHandler.h"

NPT_SET_LOCAL_LOGGER("platinum.media.server")

/*----------------------------------------------------------------------
|   forward references
+---------------------------------------------------------------------*/
extern NPT_UInt8 MS_ConnectionManagerSCPD[];
extern NPT_UInt8 MS_ContentDirectorySCPD[];
extern NPT_UInt8 MS_ContentDirectorywSearchSCPD[];
extern NPT_UInt8 X_MS_MediaReceiverRegistrarSCPD[2575];

const char* BrowseFlagsStr[] = {
    "BrowseMetadata",
    "BrowseDirectChildren"
};

/*----------------------------------------------------------------------
|   PLT_MediaServer::PLT_MediaServer
+---------------------------------------------------------------------*/
PLT_MediaServer::PLT_MediaServer(const char*  friendly_name, 
                                 bool         show_ip, 
                                 const char*  uuid, 
                                 unsigned int port,
                                 unsigned int fileserver_port) :	
    PLT_DeviceHost("/", uuid, "urn:schemas-upnp-org:device:MediaServer:1", friendly_name, show_ip, port)
{
    PLT_Service* service = new PLT_Service(
        this,
        "urn:schemas-upnp-org:service:ContentDirectory:1", 
        "urn:upnp-org:serviceId:CDS_1-0");
    if (NPT_SUCCEEDED(service->SetSCPDXML((const char*) MS_ContentDirectorywSearchSCPD))) {
        service->InitURLs("ContentDirectory", m_UUID);
        AddService(service);
        service->SetStateVariable("ContainerUpdateIDs", "0", false);
        service->SetStateVariable("SystemUpdateID", "0", false);
        service->SetStateVariable("SearchCapability", "upnp:class", false);
        service->SetStateVariable("SortCapability", "", false);
    }

    service = new PLT_Service(
        this,
        "urn:schemas-upnp-org:service:ConnectionManager:1", 
        "urn:upnp-org:serviceId:CMGR_1-0");
    if (NPT_SUCCEEDED(service->SetSCPDXML((const char*) MS_ConnectionManagerSCPD))) {
        service->InitURLs("ConnectionManager", m_UUID);
        AddService(service);
        service->SetStateVariable("CurrentConnectionIDs", "0", false);
        service->SetStateVariable("SinkProtocolInfo", "", false);
        service->SetStateVariable("SourceProtocolInfo", "http-get:*:*:*", false);
    }

    service = new PLT_Service(
        this,
        "urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:1", 
        "urn:microsoft.com:serviceId:X_MS_MediaReceiverRegistrar");
    if (NPT_SUCCEEDED(service->SetSCPDXML((const char*) X_MS_MediaReceiverRegistrarSCPD))) {
        service->InitURLs("X_MS_MediaReceiverRegistrar", m_UUID);
        AddService(service);
    }

    m_FileServer = new PLT_HttpServer(fileserver_port);
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::~PLT_MediaServer
+---------------------------------------------------------------------*/
PLT_MediaServer::~PLT_MediaServer()
{
    delete m_FileServer;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::Start
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::Start(PLT_TaskManager* task_manager, PLT_DeviceHostReference& self)
{
    // start our file server
    NPT_CHECK_SEVERE(m_FileServer->Start());

    return PLT_DeviceHost::Start(task_manager, self);
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::Stop
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::Stop()
{
    // stop our file server
    m_FileServer->Stop();

    return PLT_DeviceHost::Stop();
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnAction
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::OnAction(PLT_ActionReference& action, NPT_SocketInfo* info /* = NULL */)
{
    /* parse the action name */
    NPT_String name = action->GetActionDesc()->GetName();

    // ContentDirectory
    if (name.Compare("Browse", true) == 0) {
        return OnBrowse(action, info);
    }
    if (name.Compare("Search", true) == 0) {
        return OnSearch(action, info);
    }
    if (name.Compare("GetSystemUpdateID", true) == 0) {
        return OnGetSystemUpdateID(action, info);
    }
    if (name.Compare("GetSortCapabilities", true) == 0) {
        return OnGetSortCapabilities(action, info);
    }  
    if (name.Compare("GetSearchCapabilities", true) == 0) {
        return OnGetSearchCapabilities(action, info);
    }  

    // ConnectionMananger
    if (name.Compare("GetCurrentConnectionIDs", true) == 0) {
        return OnGetCurrentConnectionIDs(action, info);
    }
    if (name.Compare("GetProtocolInfo", true) == 0) {
        return OnGetProtocolInfo(action, info);
    }    
    if (name.Compare("GetCurrentConnectionInfo", true) == 0) {
        return OnGetCurrentConnectionInfo(action, info);
    }

    // X_MS_MediaReceiverRegistrar
    if (name.Compare("IsAuthorized", true) == 0) {
        return OnIsAuthorized(action, info);
    }
    if (name.Compare("IsValidated", true) == 0) {
        return OnIsValidated(action, info);
    }    
    if (name.Compare("GetCurrentConnectionInfo", true) == 0) {
        return OnRegisterDevice(action, info);
    }

    action->SetError(401,"No Such Action.");
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnGetCurrentConnectionIDs
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::OnGetCurrentConnectionIDs(PLT_ActionReference& action, NPT_SocketInfo* info /* = NULL */)
{
    NPT_COMPILER_UNUSED(info);

    if (NPT_FAILED(action->SetArgumentOutFromStateVariable("ConnectionIDs"))) {
        return NPT_FAILURE;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnGetProtocolInfo
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::OnGetProtocolInfo(PLT_ActionReference& action, NPT_SocketInfo* info /* = NULL */)
{
    NPT_COMPILER_UNUSED(info);

    if (NPT_FAILED(action->SetArgumentOutFromStateVariable("Source"))) {
        return NPT_FAILURE;
    }
    if (NPT_FAILED(action->SetArgumentOutFromStateVariable("Sink"))) {
        return NPT_FAILURE;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnGetCurrentConnectionInfo
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::OnGetCurrentConnectionInfo(PLT_ActionReference& action, NPT_SocketInfo* info /* = NULL */)
{
    NPT_COMPILER_UNUSED(info);

    if (NPT_FAILED(action->VerifyArgumentValue("ConnectionID", "0"))) {
        action->SetError(706,"No Such Connection.");
        return NPT_FAILURE;
    }

    if (NPT_FAILED(action->SetArgumentValue("RcsID", "-1"))){
        return NPT_FAILURE;
    }
    if (NPT_FAILED(action->SetArgumentValue("AVTransportID", "-1"))) {
        return NPT_FAILURE;
    }
    if (NPT_FAILED(action->SetArgumentValue("ProtocolInfo", "http-get:*:*:*"))) {
        return NPT_FAILURE;
    }
    if (NPT_FAILED(action->SetArgumentValue("PeerConnectionManager", "/"))) {
        return NPT_FAILURE;
    }
    if (NPT_FAILED(action->SetArgumentValue("PeerConnectionID", "-1"))) {
        return NPT_FAILURE;
    }
    if (NPT_FAILED(action->SetArgumentValue("Direction", "Output"))) {
        return NPT_FAILURE;
    }
    if (NPT_FAILED(action->SetArgumentValue("Status", "Unknown"))) {
        return NPT_FAILURE;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnGetSystemUpdateID
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::OnGetSystemUpdateID(PLT_ActionReference& action, NPT_SocketInfo* info /* = NULL */)
{
    NPT_COMPILER_UNUSED(info);

    if (NPT_FAILED(action->SetArgumentOutFromStateVariable("Id"))) {
        return NPT_FAILURE;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnGetSortCapabilities
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::OnGetSortCapabilities(PLT_ActionReference& action, NPT_SocketInfo* info /* = NULL */)
{
    NPT_COMPILER_UNUSED(info);
    NPT_CHECK(action->SetArgumentOutFromStateVariable("SortCaps"));
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnGetSearchCapabilities
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::OnGetSearchCapabilities(PLT_ActionReference& action, NPT_SocketInfo* info /* = NULL */)
{
    NPT_COMPILER_UNUSED(info);
    NPT_CHECK(action->SetArgumentOutFromStateVariable("SearchCaps"));
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::GetBrowseFlag
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::GetBrowseFlag(const char* str, BrowseFlags& flag) 
{
    if (NPT_String::Compare(str, BrowseFlagsStr[0], true) == 0) {
        flag = BROWSEMETADATA;
        return NPT_SUCCESS;
    }
    if (NPT_String::Compare(str, BrowseFlagsStr[1], true) == 0) {
        flag = BROWSEDIRECTCHILDREN;
        return NPT_SUCCESS;
    }
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnBrowse
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::OnBrowse(PLT_ActionReference& action, NPT_SocketInfo* info /* = NULL */)
{
    NPT_COMPILER_UNUSED(info);

    NPT_Result    res;
    NPT_String    object_id;
    NPT_String    browseFlagValue;

    if (NPT_FAILED(action->GetArgumentValue("ObjectId", object_id)) || 
        NPT_FAILED(action->GetArgumentValue("BrowseFlag",  browseFlagValue))) {
        NPT_LOG_WARNING("PLT_FileMediaServer::OnBrowse - invalid arguments.");
        return NPT_SUCCESS;
    }

    /* we don't support sorting yet */
//    PLT_Argument* sortCritArg = action->GetArgument("SortCriteria");
//     if (sortCritArg != NULL)  {
//         NPT_String sortCrit = sortCritArg->GetValue();
//         if (sortCrit.GetLength() > 0) {
//             /* error */
//             PLT_Log(PLT_LOG_LEVEL_1, "PLT_FileMediaServer::OnBrowse - SortCriteria not allowed.");
//             action->SetError(709, "MediaServer does not support sorting.");
//             return NPT_FAILURE;
//         }
//     }

    /* extract browseFlag */
    BrowseFlags browseFlag;
    if (NPT_FAILED(GetBrowseFlag(browseFlagValue, browseFlag))) {
        /* error */
        NPT_LOG_WARNING("PLT_FileMediaServer::OnBrowse - BrowseFlag value not allowed.");
        action->SetError(402,"Invalid BrowseFlag arg.");
        return NPT_SUCCESS;
    }

    NPT_LOG_FINE_2("PLT_FileMediaServer::On%s - id = %s", (const char*)browseFlagValue, (const char*)object_id);

    /* Invoke the browse function */
    if (browseFlag == BROWSEMETADATA) {
        res = OnBrowseMetadata(action, object_id, info);
    } else {
        res = OnBrowseDirectChildren(action, object_id, info);
    }

    if (NPT_FAILED(res) && (action->GetErrorCode() == 0)) {
        action->SetError(800, "Internal error");
    }

    return res;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnSearch
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::OnSearch(PLT_ActionReference& action, NPT_SocketInfo* info /* = NULL */)
{
    NPT_COMPILER_UNUSED(info);

    NPT_Result    res;
    NPT_String    container_id;
    NPT_String    searchCriteria;

    NPT_CHECK_FATAL(action->GetArgumentValue("SearchCriteria", searchCriteria));
    NPT_CHECK_FATAL(action->GetArgumentValue("ContainerId", container_id));


    NPT_LOG_FINE_1("PLT_FileMediaServer::OnSearch - id = %s", (const char*)container_id);
    
    if(searchCriteria == "")
        res = OnBrowseDirectChildren(action, container_id, info);
    else
        res = OnSearch(action, container_id, searchCriteria, info);

    if (NPT_FAILED(res) && (action->GetErrorCode() == 0)) {
        action->SetError(800, "Internal error");
    }

    return res;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnIsAuthorized
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::OnIsAuthorized(PLT_ActionReference& action, NPT_SocketInfo* info /* = NULL*/)
{
    if (NPT_FAILED(action->SetArgumentValue("Result", "1"))){
        return NPT_FAILURE;
    }
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnIsValidated
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::OnIsValidated(PLT_ActionReference& action, NPT_SocketInfo* info /* = NULL*/)
{
    if (NPT_FAILED(action->SetArgumentValue("Result", "1"))){
        return NPT_FAILURE;
    }
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnRegisterDevice
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::OnRegisterDevice(PLT_ActionReference& action, NPT_SocketInfo* info /* = NULL*/)
{
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnBrowseMetadata
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaServer::OnBrowseMetadata(PLT_ActionReference& action, 
                                  const char* object_id, 
                                  NPT_SocketInfo* info /* = NULL */)
{ 
    NPT_COMPILER_UNUSED(action);
    NPT_COMPILER_UNUSED(object_id);
    NPT_COMPILER_UNUSED(info);

    return NPT_ERROR_NOT_IMPLEMENTED; 
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnBrowseDirectChildren
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaServer::OnBrowseDirectChildren(PLT_ActionReference& action, 
                                        const char*          object_id, 
                                        NPT_SocketInfo*      info /* = NULL */) 
{ 
    NPT_COMPILER_UNUSED(action);
    NPT_COMPILER_UNUSED(object_id);
    NPT_COMPILER_UNUSED(info);

    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnSearch
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::OnSearch(PLT_ActionReference& action, 
                      const NPT_String& object_id, 
                      const NPT_String& searchCriteria,
                      NPT_SocketInfo* info /*= NULL*/)
{
    return NPT_ERROR_NOT_IMPLEMENTED;
}
