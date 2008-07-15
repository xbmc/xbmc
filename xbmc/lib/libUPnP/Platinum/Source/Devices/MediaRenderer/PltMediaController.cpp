/*****************************************************************
|
|   Platinum - AV Media Controller (Media Renderer Control Point)
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltMediaController.h"
#include "PltDidl.h"
#include "PltDeviceData.h"
#include "PltXmlHelper.h"

NPT_SET_LOCAL_LOGGER("platinum.media.renderer.controller")

/*----------------------------------------------------------------------
|   PLT_MediaController::PLT_MediaController
+---------------------------------------------------------------------*/
PLT_MediaController::PLT_MediaController(PLT_CtrlPointReference&      ctrl_point, 
                                         PLT_MediaControllerListener* listener) :
    m_CtrlPoint(ctrl_point),
    m_Listener(listener)
{
    m_CtrlPoint->AddListener(this);
}

/*----------------------------------------------------------------------
|   PLT_MediaController::~PLT_MediaController
+---------------------------------------------------------------------*/
PLT_MediaController::~PLT_MediaController()
{
    m_CtrlPoint->RemoveListener(this);
}

/*----------------------------------------------------------------------
|   PLT_MediaController::OnDeviceAdded
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaController::OnDeviceAdded(PLT_DeviceDataReference& device)
{
    PLT_DeviceDataReference data;
    NPT_String uuid = device->GetUUID();
    // is it a new device?
    if (NPT_SUCCEEDED(NPT_ContainerFind(m_MediaRenderers, PLT_DeviceDataFinder(uuid), data))) {
        NPT_LOG_FINE_1("Device (%s) is already in our list!", (const char*)uuid);
        return NPT_FAILURE;
    }

    NPT_LOG_FINE("Device Found:");
    device->ToLog(NPT_LOG_LEVEL_FINE);

    // verify the device implements the function we need
    PLT_Service* serviceAVT;
    PLT_Service* serviceCMR;
    NPT_String type;
    
    type = "urn:schemas-upnp-org:service:AVTransport:1";
    if (NPT_FAILED(device->FindServiceByType(type, serviceAVT))) {
        NPT_LOG_FINE_1("Service %s not found", (const char*)type);
        return NPT_FAILURE;
    }
    
    type = "urn:schemas-upnp-org:service:ConnectionManager:1";
    if (NPT_FAILED(device->FindServiceByType(type, serviceCMR))) {
        NPT_LOG_FINE_1("Service %s not found", (const char*)type);
        return NPT_FAILURE;
    }

    m_MediaRenderers.Add(device);
    if (m_Listener) {
        m_Listener->OnMRAddedRemoved(device, 1);
    }

    // subscribe to AVT eventing
    m_CtrlPoint->Subscribe(serviceAVT);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaController::OnDeviceRemoved
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaController::OnDeviceRemoved(PLT_DeviceDataReference& device)
{
    // only release if we have kept it around
    PLT_DeviceDataReference data;
    NPT_String uuid = device->GetUUID();
    // is it a new device?
    if (NPT_FAILED(NPT_ContainerFind(m_MediaRenderers, PLT_DeviceDataFinder(uuid), data))) {
        NPT_LOG_FINE_1("Device (%s) not found in our list!", (const char*)uuid);
        return NPT_FAILURE;
    }

    NPT_LOG_FINE("Device Removed:");
    device->ToLog(NPT_LOG_LEVEL_FINE);

    m_MediaRenderers.Remove(device);
    if (m_Listener) {
        m_Listener->OnMRAddedRemoved(device, 0);
    }
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaController::FindActionDesc
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaController::FindActionDesc(PLT_DeviceDataReference& device, 
                                    const char*              service_type,
                                    const char*              action_name,
                                    PLT_ActionDesc*&         action_desc)
{
    // look for the service
    PLT_Service* service;
    if (NPT_FAILED(device->FindServiceByType(service_type, service))) {
        NPT_LOG_FINE_1("Service %s not found", (const char*)service_type);
        return NPT_FAILURE;
    }

    action_desc = service->FindActionDesc(action_name);
    if (action_desc == NULL) {
        NPT_LOG_FINE_1("Action %s not found in service", action_name);
        return NPT_FAILURE;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaController::CreateAction
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaController::CreateAction(PLT_DeviceDataReference& device, 
                                  const char*              service_type,
                                  const char*              action_name,
                                  PLT_ActionReference&     action)
{
    PLT_ActionDesc* action_desc;
    NPT_CHECK_SEVERE(FindActionDesc(device, service_type, action_name, action_desc));
    action = new PLT_Action(action_desc);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaController::CallAVTransportAction
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaController::CallAVTransportAction(PLT_ActionReference& action,
                                           NPT_UInt32           instance_id,
                                           void*                userdata)
{
    // Set the object id
    NPT_CHECK_SEVERE(action->SetArgumentValue("InstanceID", 
        NPT_String::FromInteger(instance_id)));

    // set the arguments on the action, this will check the argument values
    return m_CtrlPoint->InvokeAction(action, userdata);
}

/*----------------------------------------------------------------------
|   PLT_MediaController::GetCurrentTransportActions
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaController::GetCurrentTransportActions(PLT_DeviceDataReference& device, 
                                                NPT_UInt32               instance_id,
                                                void*                    userdata)
{
    PLT_ActionReference action;
    NPT_CHECK_SEVERE(CreateAction(device, 
        "urn:schemas-upnp-org:service:AVTransport:1", 
        "GetCurrentTransportActions", 
        action));
    return CallAVTransportAction(action, instance_id, userdata);
}

/*----------------------------------------------------------------------
|   PLT_MediaController::GetDeviceCapabilities
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaController::GetDeviceCapabilities(PLT_DeviceDataReference& device, 
                                           NPT_UInt32               instance_id,
                                           void*                    userdata)
{
    PLT_ActionReference action;
    NPT_CHECK_SEVERE(CreateAction(device, 
        "urn:schemas-upnp-org:service:AVTransport:1", 
        "GetDeviceCapabilities", 
        action));
    return CallAVTransportAction(action, instance_id, userdata);
}

/*----------------------------------------------------------------------
|   PLT_MediaController::GetMediaInfo
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaController::GetMediaInfo(PLT_DeviceDataReference& device, 
                                  NPT_UInt32               instance_id,
                                  void*                    userdata)
{
    PLT_ActionReference action;
    NPT_CHECK_SEVERE(CreateAction(device, 
        "urn:schemas-upnp-org:service:AVTransport:1", 
        "GetMediaInfo", 
        action));
    return CallAVTransportAction(action, instance_id, userdata);
}

/*----------------------------------------------------------------------
|   PLT_MediaController::GetPositionInfo
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaController::GetPositionInfo(PLT_DeviceDataReference& device, 
                                     NPT_UInt32               instance_id,
                                     void*                    userdata)
{
    PLT_ActionReference action;
    NPT_CHECK_SEVERE(CreateAction(device, 
        "urn:schemas-upnp-org:service:AVTransport:1", 
        "GetPositionInfo", 
        action));
    return CallAVTransportAction(action, instance_id, userdata);
}

/*----------------------------------------------------------------------
|   PLT_MediaController::GetTransportInfo
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaController::GetTransportInfo(PLT_DeviceDataReference& device, 
                                      NPT_UInt32               instance_id,
                                      void*                    userdata)
{
    PLT_ActionReference action;
    NPT_CHECK_SEVERE(CreateAction(device, 
        "urn:schemas-upnp-org:service:AVTransport:1", 
        "GetTransportInfo", 
        action));
    return CallAVTransportAction(action, instance_id, userdata);
}

/*----------------------------------------------------------------------
|   PLT_MediaController::GetTransportSettings
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaController::GetTransportSettings(PLT_DeviceDataReference&  device, 
                                          NPT_UInt32                instance_id,
                                          void*                     userdata)
{
    PLT_ActionReference action;
    NPT_CHECK_SEVERE(CreateAction(device, 
        "urn:schemas-upnp-org:service:AVTransport:1", 
        "GetTransportSettings", 
        action));
    return CallAVTransportAction(action, instance_id, userdata);
}

/*----------------------------------------------------------------------
|   PLT_MediaController::Next
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaController::Next(PLT_DeviceDataReference& device, 
                          NPT_UInt32               instance_id,
                          void*                    userdata)
{
    PLT_ActionReference action;
    NPT_CHECK_SEVERE(CreateAction(device, 
        "urn:schemas-upnp-org:service:AVTransport:1", 
        "Next", 
        action));
    return CallAVTransportAction(action, instance_id, userdata);
}

/*----------------------------------------------------------------------
|   PLT_MediaController::Pause
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaController::Pause(PLT_DeviceDataReference& device, 
                           NPT_UInt32               instance_id,
                           void*                    userdata)
{
    PLT_ActionReference action;
    NPT_CHECK_SEVERE(CreateAction(device, 
        "urn:schemas-upnp-org:service:AVTransport:1", 
        "Pause", 
        action));
    return CallAVTransportAction(action, instance_id, userdata);
}

/*----------------------------------------------------------------------
|   PLT_MediaController::Play
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaController::Play(PLT_DeviceDataReference& device, 
                          NPT_UInt32               instance_id,
                          NPT_String               speed,
                          void*                    userdata)
{
    PLT_ActionReference action;
    NPT_CHECK_SEVERE(CreateAction(device, 
        "urn:schemas-upnp-org:service:AVTransport:1", 
        "Play", 
        action));

    // Set the speed
    if (NPT_FAILED(action->SetArgumentValue("Speed", speed))) {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    return CallAVTransportAction(action, instance_id, userdata);
}

/*----------------------------------------------------------------------
|   PLT_MediaController::Previous
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaController::Previous(PLT_DeviceDataReference& device, 
                              NPT_UInt32               instance_id,
                              void*                    userdata)
{
    PLT_ActionReference action;
    NPT_CHECK_SEVERE(CreateAction(device, 
        "urn:schemas-upnp-org:service:AVTransport:1", 
        "Previous", 
        action));
    return CallAVTransportAction(action, instance_id, userdata);
}

/*----------------------------------------------------------------------
|   PLT_MediaController::Seek
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaController::Seek(PLT_DeviceDataReference& device, 
                          NPT_UInt32               instance_id,
                          NPT_String               unit,
                          NPT_String               target,
                          void*                    userdata)
{
    PLT_ActionReference action;
    NPT_CHECK_SEVERE(CreateAction(device, 
        "urn:schemas-upnp-org:service:AVTransport:1", 
        "Seek", 
        action));

    // Set the unit
    if (NPT_FAILED(action->SetArgumentValue("Unit", unit))) {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    // Set the target
    if (NPT_FAILED(action->SetArgumentValue("Target", target))) {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    return CallAVTransportAction(action, instance_id, userdata);
}

/*----------------------------------------------------------------------
|   PLT_MediaController::SetAVTransportURI
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaController::SetAVTransportURI(PLT_DeviceDataReference& device, 
                                       NPT_UInt32               instance_id, 
                                       const char*              uri,
                                       const char*              metadata,
                                       void*                    userdata)
{
    PLT_ActionReference action;
    NPT_CHECK_SEVERE(CreateAction(device, 
        "urn:schemas-upnp-org:service:AVTransport:1", 
        "SetAVTransportURI", 
        action));

    // set the uri
    if (NPT_FAILED(action->SetArgumentValue("CurrentURI", uri))) {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    // set the uri metadata
    if (NPT_FAILED(action->SetArgumentValue("CurrentURIMetaData", metadata))) {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    return CallAVTransportAction(action, instance_id, userdata);
}

/*----------------------------------------------------------------------
|   PLT_MediaController::SetPlayMode
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaController::SetPlayMode(PLT_DeviceDataReference& device, 
                                 NPT_UInt32               instance_id,
                                 NPT_String               new_play_mode,
                                 void*                    userdata)
{
    PLT_ActionReference action;
    NPT_CHECK_SEVERE(CreateAction(device, 
        "urn:schemas-upnp-org:service:AVTransport:1", 
        "SetPlayMode", 
        action));

    // set the New PlayMode
    if (NPT_FAILED(action->SetArgumentValue("NewPlayMode", new_play_mode))) {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    return CallAVTransportAction(action, instance_id, userdata);
}

/*----------------------------------------------------------------------
|   PLT_MediaController::Stop
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaController::Stop(PLT_DeviceDataReference& device, 
                          NPT_UInt32               instance_id,
                          void*                    userdata)
{
    PLT_ActionReference action;
    NPT_CHECK_SEVERE(CreateAction(device, 
        "urn:schemas-upnp-org:service:AVTransport:1", 
        "Stop", 
        action));
    return CallAVTransportAction(action, instance_id, userdata);
}

/*----------------------------------------------------------------------
|   PLT_MediaController::GetCurrentConnectionIDs
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaController::GetCurrentConnectionIDs(PLT_DeviceDataReference& device, 
                                             void*                    userdata)
{
    PLT_ActionReference action;
    NPT_CHECK_SEVERE(CreateAction(device, 
        "urn:schemas-upnp-org:service:ConnectionManager:1", 
        "GetCurrentConnectionIDs", 
        action));

    // set the arguments on the action, this will check the argument values
    if (NPT_FAILED(m_CtrlPoint->InvokeAction(action, userdata))) {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaController::GetCurrentConnectionInfo
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaController::GetCurrentConnectionInfo(PLT_DeviceDataReference& device, 
                                              NPT_UInt32               connection_id,
                                              void*                    userdata)
{
    PLT_ActionReference action;
    NPT_CHECK_SEVERE(CreateAction(device, 
        "urn:schemas-upnp-org:service:ConnectionManager:1", 
        "GetCurrentConnectionInfo", 
        action));

    // set the New PlayMode
    if (NPT_FAILED(action->SetArgumentValue("ConnectionID", NPT_String::FromInteger(connection_id)))) {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    // set the arguments on the action, this will check the argument values
    if (NPT_FAILED(m_CtrlPoint->InvokeAction(action, userdata))) {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaController::GetProtocolInfo
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaController::GetProtocolInfo(PLT_DeviceDataReference& device, 
                                     void*                    userdata)
{
    PLT_ActionReference action;
    NPT_CHECK_SEVERE(CreateAction(device, 
        "urn:schemas-upnp-org:service:ConnectionManager:1", 
        "GetProtocolInfo", 
        action));

    // set the arguments on the action, this will check the argument values
    if (NPT_FAILED(m_CtrlPoint->InvokeAction(action, userdata))) {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaController::OnActionResponse
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaController::OnActionResponse(NPT_Result res, PLT_ActionReference& action, void* userdata)
{
    if (m_Listener == NULL) {
        return NPT_SUCCESS;
    }

    /* make sure device is a renderer we've previously found */
    PLT_DeviceDataReference device;
    NPT_String uuid = action->GetActionDesc()->GetService()->GetDevice()->GetUUID();
    if (NPT_FAILED(NPT_ContainerFind(m_MediaRenderers, PLT_DeviceDataFinder(uuid), device))) {
        NPT_LOG_FINE_1("Device (%s) not found in our list of renderers", (const char*)uuid);
        res = NPT_FAILURE;
    }
       
    /* extract action name */
    NPT_String actionName = action->GetActionDesc()->GetName();

    /* AVTransport response ? */
    if (actionName.Compare("GetCurrentTransportActions", true) == 0) {
        return OnGetCurrentTransportActionsResponse(res, device, action, userdata);
    }
    else if (actionName.Compare("GetDeviceCapabilities", true) == 0) {
        return OnGetDeviceCapabilitiesResponse(res, device, action, userdata);
    }
    else if (actionName.Compare("GetMediaInfo", true) == 0) {
        return OnGetMediaInfoResponse(res, device, action, userdata);
    }
    else if (actionName.Compare("GetPositionInfo", true) == 0) {
        return OnGetPositionInfoResponse(res, device, action, userdata);
    }
    else if (actionName.Compare("GetTransportInfo", true) == 0) {
        return OnGetTransportInfoResponse(res, device, action, userdata);
    }
    else if (actionName.Compare("GetTransportSettings", true) == 0) {
        return OnGetTransportSettingsResponse(res, device, action, userdata);
    }
    else if (actionName.Compare("Next", true) == 0) {
        m_Listener->OnNextResult(res, device, userdata);
    }
    else if (actionName.Compare("Pause", true) == 0) {
        m_Listener->OnPauseResult(res, device, userdata);
    }
    else if (actionName.Compare("Play", true) == 0) {
        m_Listener->OnPlayResult(res, device, userdata);
    }
    else if (actionName.Compare("Previous", true) == 0) {
        m_Listener->OnPreviousResult(res, device, userdata);
    }
    else if (actionName.Compare("Seek", true) == 0) {
        m_Listener->OnSeekResult(res, device, userdata);
    }
    else if (actionName.Compare("SetAVTransportURI", true) == 0) {
        m_Listener->OnSetAVTransportURIResult(res, device, userdata);
    }
    else if (actionName.Compare("SetPlayMode", true) == 0) {
        m_Listener->OnSetPlayModeResult(res, device, userdata);
    }
    else if (actionName.Compare("Stop", true) == 0) {
        m_Listener->OnStopResult(res, device, userdata);
    }
    else if (actionName.Compare("GetCurrentConnectionIDs", true) == 0) {
        return OnGetCurrentConnectionIDsResponse(res, device, action, userdata);
    }
    else if (actionName.Compare("GetCurrentConnectionInfo", true) == 0) {
        return OnGetCurrentConnectionInfoResponse(res, device, action, userdata);
    }
    else if (actionName.Compare("GetProtocolInfo", true) == 0) {
        return OnGetProtocolInfoResponse(res, device, action, userdata);
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaController::OnGetCurrentTransportActionsResponse
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaController::OnGetCurrentTransportActionsResponse(NPT_Result res, 
                                                          PLT_DeviceDataReference& device, 
                                                          PLT_ActionReference& action, 
                                                          void* userdata)
{
    NPT_String actions;
    PLT_StringList values;

    if (NPT_FAILED(res) || action->GetErrorCode() != 0) {
        goto bad_action;
    }

    if (NPT_FAILED(action->GetArgumentValue("Actions", actions))) {
        goto bad_action;
    }

    // parse the list of actions and return a list to listener
    ParseCSV(actions, values);

    m_Listener->OnGetCurrentTransportActionsResult(NPT_SUCCESS, device, &values, userdata);
    return NPT_SUCCESS;

bad_action:
    m_Listener->OnGetCurrentTransportActionsResult(NPT_FAILURE, device, NULL, userdata);
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_MediaController::OnGetDeviceCapabilitiesResponse
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaController::OnGetDeviceCapabilitiesResponse(NPT_Result res, 
                                                     PLT_DeviceDataReference& device, 
                                                     PLT_ActionReference& action, 
                                                     void* userdata)
{
    NPT_String value;
    PLT_DeviceCapabilities capabilities;

    if (NPT_FAILED(res) || action->GetErrorCode() != 0) {
        goto bad_action;
    }

    if (NPT_FAILED(action->GetArgumentValue("PlayMedia", value))) {
        goto bad_action;
    }
    // parse the list of medias and return a list to listener
    ParseCSV(value, capabilities.play_media);

    if (NPT_FAILED(action->GetArgumentValue("RecMedia", value))) {
        goto bad_action;
    }
    // parse the list of rec and return a list to listener
    ParseCSV(value, capabilities.rec_media);

    if (NPT_FAILED(action->GetArgumentValue("RecQualityModes", value))) {
        goto bad_action;
    }
    // parse the list of modes and return a list to listener
    ParseCSV(value, capabilities.rec_quality_modes);

    m_Listener->OnGetDeviceCapabilitiesResult(NPT_SUCCESS, device, &capabilities, userdata);
    return NPT_SUCCESS;

bad_action:
    m_Listener->OnGetDeviceCapabilitiesResult(NPT_FAILURE, device, NULL, userdata);
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_MediaController::OnGetMediaInfoResponse
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaController::OnGetMediaInfoResponse(NPT_Result res, 
                                            PLT_DeviceDataReference& device, 
                                            PLT_ActionReference& action, 
                                            void* userdata)
{
    NPT_String      value;
    PLT_MediaInfo   info;

    if (NPT_FAILED(res) || action->GetErrorCode() != 0) {
        goto bad_action;
    }

    if (NPT_FAILED(action->GetArgumentValue("NrTracks", info.num_tracks))) {
        goto bad_action;
    }
    if (NPT_FAILED(action->GetArgumentValue("MediaDuration", value))) {
        goto bad_action;
    }
    if (NPT_FAILED(PLT_Didl::ParseTimeStamp(value, info.media_duration))) {
        goto bad_action;
    }

    if (NPT_FAILED(action->GetArgumentValue("CurrentURI", info.cur_uri))) {
        goto bad_action;
    }
    if (NPT_FAILED(action->GetArgumentValue("CurrentURIMetaData", info.cur_metadata))) {
        goto bad_action;
    }
    if (NPT_FAILED(action->GetArgumentValue("NextURI", info.next_uri))) {
        goto bad_action;
    }
    if (NPT_FAILED(action->GetArgumentValue("NextURIMetaData",  info.next_metadata))) {
        goto bad_action;
    }
    if (NPT_FAILED(action->GetArgumentValue("PlayMedium", info.play_medium))) {
        goto bad_action;
    }
    if (NPT_FAILED(action->GetArgumentValue("RecordMedium", info.rec_medium))) {
        goto bad_action;
    }
    if (NPT_FAILED(action->GetArgumentValue("WriteStatus", info.write_status))) {
        goto bad_action;
    }

    m_Listener->OnGetMediaInfoResult(NPT_SUCCESS, device, &info, userdata);
    return NPT_SUCCESS;

bad_action:
    m_Listener->OnGetMediaInfoResult(NPT_FAILURE, device, NULL, userdata);
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_MediaController::OnGetPositionInfoResponse
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaController::OnGetPositionInfoResponse(NPT_Result res, 
                                               PLT_DeviceDataReference& device, 
                                               PLT_ActionReference& action, 
                                               void* userdata)
{
    NPT_String       value;
    PLT_PositionInfo info;

    if (NPT_FAILED(res) || action->GetErrorCode() != 0) {
        goto bad_action;
    }

    if (NPT_FAILED(action->GetArgumentValue("Track", info.track))) {
        goto bad_action;
    }

    if (NPT_FAILED(action->GetArgumentValue("TrackDuration", value))) {
        goto bad_action;
    }
    if (NPT_FAILED(PLT_Didl::ParseTimeStamp(value, info.track_duration))) {
        goto bad_action;
    }

    if (NPT_FAILED(action->GetArgumentValue("TrackMetaData", info.track_metadata))) {
        goto bad_action;
    }    
    
    if (NPT_FAILED(action->GetArgumentValue("TrackURI", info.track_uri))) {
        goto bad_action;
    }

    if (NPT_FAILED(action->GetArgumentValue("RelTime", value))) {
        goto bad_action;
    }
    if (NPT_FAILED(PLT_Didl::ParseTimeStamp(value, info.rel_time))) {
        goto bad_action;
    }

    if (NPT_FAILED(action->GetArgumentValue("AbsTime", value))) {
        goto bad_action;
    }
    if (NPT_FAILED(PLT_Didl::ParseTimeStamp(value, info.abs_time))) {
        goto bad_action;
    }

    if (NPT_FAILED(action->GetArgumentValue("RelCount", info.rel_count))) {
        goto bad_action;
    }    
    if (NPT_FAILED(action->GetArgumentValue("AbsCount", info.abs_count))) {
        goto bad_action;
    }

    m_Listener->OnGetPositionInfoResult(NPT_SUCCESS, device, &info, userdata);
    return NPT_SUCCESS;

bad_action:
    m_Listener->OnGetPositionInfoResult(NPT_FAILURE, device, NULL, userdata);
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_MediaController::OnGetTransportInfoResponse
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaController::OnGetTransportInfoResponse(NPT_Result res, 
                                                PLT_DeviceDataReference& device, 
                                                PLT_ActionReference& action, 
                                                void* userdata)
{
    PLT_TransportInfo info;

    if (NPT_FAILED(res) || action->GetErrorCode() != 0) {
        goto bad_action;
    }

    if (NPT_FAILED(action->GetArgumentValue("CurrentTransportState", info.cur_transport_state))) {
        goto bad_action;
    }    
    if (NPT_FAILED(action->GetArgumentValue("CurrentTransportStatus", info.cur_transport_status))) {
        goto bad_action;
    }    
    if (NPT_FAILED(action->GetArgumentValue("CurrentSpeed", info.cur_speed))) {
        goto bad_action;
    }    

    m_Listener->OnGetTransportInfoResult(NPT_SUCCESS, device, &info, userdata);
    return NPT_SUCCESS;

bad_action:
    m_Listener->OnGetTransportInfoResult(NPT_FAILURE, device, NULL, userdata);
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_MediaController::OnGetTransportSettingsResponse
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaController::OnGetTransportSettingsResponse(NPT_Result res, 
                                                    PLT_DeviceDataReference& device, 
                                                    PLT_ActionReference& action, 
                                                    void* userdata)
{
    PLT_TransportSettings settings;

    if (NPT_FAILED(res) || action->GetErrorCode() != 0) {
        goto bad_action;
    }

    if (NPT_FAILED(action->GetArgumentValue("PlayMode", settings.play_mode))) {
        goto bad_action;
    }    
    if (NPT_FAILED(action->GetArgumentValue("RecQualityMode", settings.rec_quality_mode))) {
        goto bad_action;
    }    

    m_Listener->OnGetTransportSettingsResult(NPT_SUCCESS, device, &settings, userdata);
    return NPT_SUCCESS;

bad_action:
    m_Listener->OnGetTransportSettingsResult(NPT_FAILURE, device, NULL, userdata);
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_MediaController::OnGetCurrentConnectionIDsResponse
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaController::OnGetCurrentConnectionIDsResponse(NPT_Result res, 
                                                       PLT_DeviceDataReference& device, 
                                                       PLT_ActionReference& action, 
                                                       void* userdata)
{
    NPT_String value;
    PLT_StringList IDs;

    if (NPT_FAILED(res) || action->GetErrorCode() != 0) {
        goto bad_action;
    }

    if (NPT_FAILED(action->GetArgumentValue("ConnectionIDs", value))) {
        goto bad_action;
    }
    // parse the list of medias and return a list to listener
    ParseCSV(value, IDs);

    m_Listener->OnGetCurrentConnectionIDsResult(NPT_SUCCESS, device, &IDs, userdata);
    return NPT_SUCCESS;

bad_action:
    m_Listener->OnGetCurrentConnectionIDsResult(NPT_FAILURE, device, NULL, userdata);
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_MediaController::OnGetCurrentConnectionInfoResponse
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaController::OnGetCurrentConnectionInfoResponse(NPT_Result res, 
                                                        PLT_DeviceDataReference& device, 
                                                        PLT_ActionReference& action, 
                                                        void* userdata)
{
    NPT_String value;
    PLT_ConnectionInfo info;

    if (NPT_FAILED(res) || action->GetErrorCode() != 0) {
        goto bad_action;
    }

    if (NPT_FAILED(action->GetArgumentValue("RcsID", info.rcs_id))) {
        goto bad_action;
    }
    if (NPT_FAILED(action->GetArgumentValue("AVTransportID", info.avtransport_id))) {
        goto bad_action;
    }
    if (NPT_FAILED(action->GetArgumentValue("ProtocolInfo", info.protocol_info))) {
        goto bad_action;
    }
    if (NPT_FAILED(action->GetArgumentValue("PeerConnectionManager", info.peer_connection_mgr))) {
        goto bad_action;
    }
    if (NPT_FAILED(action->GetArgumentValue("PeerConnectionID", info.peer_connection_id))) {
        goto bad_action;
    }
    if (NPT_FAILED(action->GetArgumentValue("Direction", info.direction))) {
        goto bad_action;
    }
    if (NPT_FAILED(action->GetArgumentValue("Status", info.status))) {
        goto bad_action;
    }
    m_Listener->OnGetCurrentConnectionInfoResult(NPT_SUCCESS, device, &info, userdata);
    return NPT_SUCCESS;

bad_action:
    m_Listener->OnGetCurrentConnectionInfoResult(NPT_FAILURE, device, NULL, userdata);
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_MediaController::OnGetProtocolInfoResponse
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaController::OnGetProtocolInfoResponse(NPT_Result res, 
                                               PLT_DeviceDataReference& device, 
                                               PLT_ActionReference& action, 
                                               void* userdata)
{
    NPT_String     source_info, sink_info;
    PLT_StringList sources, sinks;

    if (NPT_FAILED(res) || action->GetErrorCode() != 0) {
        goto bad_action;
    }

    if (NPT_FAILED(action->GetArgumentValue("Source", source_info))) {
        goto bad_action;
    }
    ParseCSV(source_info, sources);

    if (NPT_FAILED(action->GetArgumentValue("Sink", sink_info))) {
        goto bad_action;
    }
    ParseCSV(sink_info, sinks);

    m_Listener->OnGetProtocolInfoResult(NPT_SUCCESS, device, &sources, &sinks, userdata);
    return NPT_SUCCESS;

bad_action:
    m_Listener->OnGetProtocolInfoResult(NPT_FAILURE, device, NULL, NULL, userdata);
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_MediaController::OnEventNotify
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaController::OnEventNotify(PLT_Service* service, NPT_List<PLT_StateVariable*>* vars)
{
    if (m_Listener) {
        // parse LastChange var into smaller vars
        PLT_StateVariable* lastChangeVar = NULL;
        if (NPT_SUCCEEDED(NPT_ContainerFind(*vars, PLT_ListStateVariableNameFinder("LastChange"), lastChangeVar))) {
            vars->Remove(lastChangeVar);
            PLT_Service* service = lastChangeVar->GetService();
            NPT_String text = lastChangeVar->GetValue();
            
            NPT_XmlNode* xml = NULL;
            NPT_XmlParser parser;
            if (NPT_FAILED(parser.Parse(text, xml)) || !xml || !xml->AsElementNode()) {
                delete xml;
                return NPT_FAILURE;
            }

            NPT_XmlElementNode* node = xml->AsElementNode();
            if (!node->GetTag().Compare("Event", true)) {
                // look for the instance with attribute id = 0
                NPT_XmlElementNode* instance = NULL;
                for (NPT_Cardinal i=0; i<node->GetChildren().GetItemCount(); i++) {
                    NPT_XmlElementNode* child;
                    if (NPT_FAILED(PLT_XmlHelper::GetChild(node, child, i)))
                        continue;

                    if (!child->GetTag().Compare("InstanceID", true)) {
                        // extract the "val" attribute value
                        NPT_String value;
                        if (NPT_SUCCEEDED(PLT_XmlHelper::GetAttribute(child, "val", value)) &&
                            !value.Compare("0")) {
                            instance = child;
                            break;
                        }
                    }
                }

                // did we find an instance with id = 0 ?
                if (instance != NULL) {
                    // all the children of the Instance node are state variables
                    for (NPT_Cardinal j=0; j<instance->GetChildren().GetItemCount(); j++) {
                        NPT_XmlElementNode* var_node;
                        if (NPT_FAILED(PLT_XmlHelper::GetChild(instance, var_node, j)))
                            continue;

                        // look for the state variable in this service
                        const NPT_String* value = var_node->GetAttribute("val");
                        PLT_StateVariable* var = service->FindStateVariable(var_node->GetTag());
                        if (value != NULL && var != NULL) {
                            // get the value and set the state variable
                            // if it succeeded, add it to the list of vars we'll event
                            if (NPT_SUCCEEDED(var->SetValue(*value, false))) {
                                vars->Add(var);
                                NPT_LOG_FINE_2("PLT_MediaController received var change for (%s): %s", (const char*)var->GetName(), (const char*)var->GetValue());
                            }
                        }
                    }
                }
            }
            delete xml;
        }

        if (vars->GetItemCount()) {
            m_Listener->OnMRStateVariablesChanged(service, vars);
        }
    }
    return NPT_SUCCESS;
}
