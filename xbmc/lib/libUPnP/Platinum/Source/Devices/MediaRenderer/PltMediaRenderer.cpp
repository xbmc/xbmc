/*****************************************************************
|
|   Platinum - AV Media Renderer Device
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltMediaRenderer.h"
#include "PltService.h"

//NPT_SET_LOCAL_LOGGER("platinum.media.renderer")

/*----------------------------------------------------------------------
|   external references
+---------------------------------------------------------------------*/
extern NPT_UInt8 RDR_ConnectionManagerSCPD[];
extern NPT_UInt8 RDR_AVTransportSCPD[];
extern NPT_UInt8 RDR_RenderingControlSCPD[];

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::PLT_MediaRenderer
+---------------------------------------------------------------------*/
PLT_MediaRenderer::PLT_MediaRenderer(PlaybackCmdListener* listener, 
                                     const char*          friendly_name, 
                                     bool                 show_ip, 
                                     const char*          uuid, 
                                     unsigned int         port) :	
    PLT_DeviceHost("/", uuid, "urn:schemas-upnp-org:device:MediaRenderer:1", friendly_name, show_ip, port)
{
    NPT_COMPILER_UNUSED(listener);

    PLT_Service* service;

    /* AVTransport */
    service = new PLT_Service(
        this,
        "urn:schemas-upnp-org:service:AVTransport:1", 
        "urn:upnp-org:serviceId:AVT_1-0");
    if (NPT_SUCCEEDED(service->SetSCPDXML((const char*) RDR_AVTransportSCPD))) {
        service->InitURLs("AVTransport", m_UUID);
        AddService(service);
        service->SetStateVariable("LastChange", "<Event xmlns = \"urn:schemas-upnp-org:metadata-1-0/AVT/\"/>", false);

        // GetCurrentTransportActions
        service->SetStateVariable("CurrentTransportActions", "", false);
        
        // GetDeviceCapabilities
        service->SetStateVariable("PossiblePlaybackStorageMedia", "vendor-defined ,NOT_IMPLEMENTED,NONE,NETWORK,MICRO-MV,HDD,LD,DAT,DVD-AUDIO,DVD-RAM,DVD-RW,DVD+RW,DVD-R,DVD-VIDEO,DVD-ROM,MD-PICTURE,MD-AUDIO,SACD,VIDEO-CD,CD-RW,CD-R,CD-DA,CD-ROM,HI8,VIDEO8,VHSC,D-VHS,S-VHS,W-VHS,VHS,MINI-DV,DV,UNKNOWN", false);
        service->SetStateVariable("PossibleRecordStorageMedia", "vendor-defined ,NOT_IMPLEMENTED,NONE,NETWORK,MICRO-MV,HDD,LD,DAT,DVD-AUDIO,DVD-RAM,DVD-RW,DVD+RW,DVD-R,DVD-VIDEO,DVD-ROM,MD-PICTURE,MD-AUDIO,SACD,VIDEO-CD,CD-RW,CD-R,CD-DA,CD-ROM,HI8,VIDEO8,VHSC,D-VHS,S-VHS,W-VHS,VHS,MINI-DV,DV,UNKNOWN", false);
        service->SetStateVariable("PossibleRecordQualityModes", "vendor-defined ,NOT_IMPLEMENTED,2:HIGH,1:MEDIUM,0:BASIC,2:SP,1:LP,0:EP", false);
        
        // GetMediaInfo
        service->SetStateVariable("PlaybackStorageMedium", "UNKNOWN", false);
        service->SetStateVariable("RecordStorageMedium", "UNKNOWN", false);
        service->SetStateVariable("RecordMediumWriteStatus", "UNKNOWN", false);
        service->SetStateVariable("NumberOfTracks", "0", false);
        service->SetStateVariable("CurrentTrackDuration", "00:00:00", false);
        service->SetStateVariable("CurrentMediaDuration", "00:00:00", false);
        service->SetStateVariable("NextAVTransportURI", "NOT_IMPLEMENTED", false);
        service->SetStateVariable("NextAVTransportURIMetadata", "NOT_IMPLEMENTED", false);
        
        // GetPositionInfo
        service->SetStateVariable("AbsTime", "NOT_IMPLEMENTED", false);
        service->SetStateVariable("CurrentTrack", "0", false);
        service->SetStateVariable("RelativeTimePosition", "00:00:00", false); //??
        service->SetStateVariable("AbsoluteTimePosition", "NOT_IMPLEMENTED", false); //??
        service->SetStateVariable("RelativeCounterPosition", "0", false); //??
        service->SetStateVariable("AbsoluteCounterPosition", "0", false); //??

        // GetTransportInfo
        service->SetStateVariable("TransportState", "NO_MEDIA_PRESENT", false);
        service->SetStateVariable("TransportStatus", "OK", false);
        service->SetStateVariable("TransportPlaySpeed", "1", false);
        
        // GetTransportSettings
        service->SetStateVariable("CurrentPlayMode", "NORMAL", false);
        service->SetStateVariable("CurrentRecordQualityMode", "NOT_IMPLEMENTED", false);
    }

    /* ConnectionManager */
    service = new PLT_Service(
        this,
        "urn:schemas-upnp-org:service:ConnectionManager:1", 
        "urn:upnp-org:serviceId:CMGR_1-0");
    if (NPT_SUCCEEDED(service->SetSCPDXML((const char*) RDR_ConnectionManagerSCPD))) {
        service->InitURLs("ConnectionManager", m_UUID);
        AddService(service);
        service->SetStateVariable("CurrentConnectionIDs", "0", false);
        // put all supported mime types here instead
        service->SetStateVariable("SinkProtocolInfo", "http-get:*:*:*;rtsp:*:*:*;http-get:*:video/mpeg:*;http-get:*:audio/mpeg:*", false);
        service->SetStateVariable("SourceProtocolInfo", "", false);
    }

    /* RenderingControl */
    service = new PLT_Service(
        this,
        "urn:schemas-upnp-org:service:RenderingControl:1", 
        "urn:upnp-org:serviceId:RCS_1-0");
    if (NPT_SUCCEEDED(service->SetSCPDXML((const char*) RDR_RenderingControlSCPD))) {
        service->InitURLs("RenderingControl", m_UUID);
        AddService(service);
        service->SetStateVariable("LastChange", "<Event xmlns = \"urn:schemas-upnp-org:metadata-1-0/RCS/\"/>", false);
    }
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::~PLT_MediaRenderer
+---------------------------------------------------------------------*/
PLT_MediaRenderer::~PLT_MediaRenderer()
{
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnAction
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnAction(PLT_ActionReference& action, NPT_SocketInfo* info /* = NULL */)
{
    NPT_COMPILER_UNUSED(info);

    /* parse the action name */
    NPT_String name = action->GetActionDesc()->GetName();

    /* Is it a ConnectionManager Service Action ? */
    if (name.Compare("GetCurrentConnectionIDs", true) == 0) {
        if (NPT_FAILED(action->SetArgumentsOutFromStateVariable())) {
            return NPT_FAILURE;
        }
        return NPT_SUCCESS;
    }
    if (name.Compare("GetProtocolInfo", true) == 0) {
        if (NPT_FAILED(action->SetArgumentsOutFromStateVariable())) {
            return NPT_FAILURE;
        }
        return NPT_SUCCESS;
    }    
    if (name.Compare("GetCurrentConnectionInfo", true) == 0) {
        return OnGetCurrentConnectionInfo(action);
    }  

    /* Is it a AVTransport Service Action ? */

    // since all actions take an instance ID and we only support 1 instance
    // verify that the Instance ID is 0 and return an error here now if not
    NPT_String serviceType = action->GetActionDesc()->GetService()->GetServiceType();
    if (serviceType.Compare("urn:schemas-upnp-org:service:AVTransport:1", true) == 0) {
        if (NPT_FAILED(action->VerifyArgumentValue("InstanceID", "0"))) {
            action->SetError(802,"Not valid InstanceID.");
            return NPT_FAILURE;
        }
    }

    if (name.Compare("GetCurrentTransportActions", true) == 0) {
        if (NPT_FAILED(action->SetArgumentsOutFromStateVariable())) {
            return NPT_FAILURE;
        }
        return NPT_SUCCESS;
    }
    if (name.Compare("GetDeviceCapabilities", true) == 0) {
        if (NPT_FAILED(action->SetArgumentsOutFromStateVariable())) {
            return NPT_FAILURE;
        }
        return NPT_SUCCESS;
    }
    if (name.Compare("GetMediaInfo", true) == 0) {
        if (NPT_FAILED(action->SetArgumentsOutFromStateVariable())) {
            return NPT_FAILURE;
        }
        return NPT_SUCCESS;
    }
    if (name.Compare("GetPositionInfo", true) == 0) {
        if (NPT_FAILED(action->SetArgumentsOutFromStateVariable())) {
            return NPT_FAILURE;
        }
        return NPT_SUCCESS;
    }
    if (name.Compare("GetTransportInfo", true) == 0) {
        if (NPT_FAILED(action->SetArgumentsOutFromStateVariable())) {
            return NPT_FAILURE;
        }
        return NPT_SUCCESS;
    }
    if (name.Compare("GetTransportSettings", true) == 0) {
        if (NPT_FAILED(action->SetArgumentsOutFromStateVariable())) {
            return NPT_FAILURE;
        }
        return NPT_SUCCESS;
    }
    if (name.Compare("Next", true) == 0) {
        return OnNext(action);
    }
    if (name.Compare("Pause", true) == 0) {
        return OnPause(action);
    }
    if (name.Compare("Play", true) == 0) {
        return OnPlay(action);
    }
    if (name.Compare("Previous", true) == 0) {
        return OnPrevious(action);
    }
    if (name.Compare("Seek", true) == 0) {
        return OnSeek(action);
    }
    if (name.Compare("Stop", true) == 0) {
        return OnStop(action);
    }
    if (name.Compare("SetAVTransportURI", true) == 0) {
        return OnSetAVTransportURI(action);
    }
    if (name.Compare("SetPlayMode", true) == 0) {
        return OnSetPlayMode(action);
    }

    action->SetError(401,"No Such Action.");
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnGetCurrentConnectionInfo
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnGetCurrentConnectionInfo(PLT_ActionReference& action)
{
    if (NPT_FAILED(action->VerifyArgumentValue("ConnectionID", "0"))) {
        action->SetError(706,"No Such Connection.");
        return NPT_FAILURE;
    }

    if (NPT_FAILED(action->SetArgumentValue("RcsID", "0"))){
        return NPT_FAILURE;
    }
    if (NPT_FAILED(action->SetArgumentValue("AVTransportID", "0"))) {
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
    if (NPT_FAILED(action->SetArgumentValue("Direction", "Input"))) {
        return NPT_FAILURE;
    }
    if (NPT_FAILED(action->SetArgumentValue("Status", "Unknown"))) {
        return NPT_FAILURE;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnNext
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnNext(PLT_ActionReference& /* action */)
{
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnPause
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnPause(PLT_ActionReference& /* action */)
{
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnPlay
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnPlay(PLT_ActionReference& /* action */)
{
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnPrevious
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnPrevious(PLT_ActionReference& /* action */)
{
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnSeek
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnSeek(PLT_ActionReference& /* action */)
{
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnStop
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnStop(PLT_ActionReference& /* action */)
{
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnSetAVTransportURI
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnSetAVTransportURI(PLT_ActionReference& /* action */)
{
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnSetPlayMode
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnSetPlayMode(PLT_ActionReference& /* action */)
{
    return NPT_SUCCESS;
}
