/*****************************************************************
|
|   Platinum - AV Media Renderer Device
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
#include "Neptune.h"
#include "PltMediaRenderer.h"
#include "PltService.h"

NPT_SET_LOCAL_LOGGER("platinum.media.renderer")

/*----------------------------------------------------------------------
|   external references
+---------------------------------------------------------------------*/
extern NPT_UInt8 RDR_ConnectionManagerSCPD[];
extern NPT_UInt8 RDR_AVTransportSCPD[];
extern NPT_UInt8 RDR_RenderingControlSCPD[];

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::PLT_MediaRenderer
+---------------------------------------------------------------------*/
PLT_MediaRenderer::PLT_MediaRenderer(const char*  friendly_name, 
                                     bool         show_ip, 
                                     const char*  uuid, 
                                     unsigned int port) :	
    PLT_DeviceHost("/", uuid, "urn:schemas-upnp-org:device:MediaRenderer:1", friendly_name, show_ip, port)
{
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::~PLT_MediaRenderer
+---------------------------------------------------------------------*/
PLT_MediaRenderer::~PLT_MediaRenderer()
{
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::SetupServices
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::SetupServices(PLT_DeviceData& data)
{
    PLT_Service* service;

    {
        /* AVTransport */
        service = new PLT_Service(
            &data,
            "urn:schemas-upnp-org:service:AVTransport:1", 
            "urn:upnp-org:serviceId:AVT_1-0",
            "urn:schemas-upnp-org:metadata-1-0/AVT/");
        NPT_CHECK_FATAL(service->SetSCPDXML((const char*) RDR_AVTransportSCPD));
        NPT_CHECK_FATAL(service->InitURLs("AVTransport", data.GetUUID()));
        NPT_CHECK_FATAL(data.AddService(service));

        service->SetStateVariableRate("LastChange", NPT_TimeInterval(0.2f));
        service->SetStateVariable("A_ARG_TYPE_InstanceID", "0"); 

        // GetCurrentTransportActions
        service->SetStateVariable("CurrentTransportActions", "Play,Pause,Stop,Seek,Next,Previous");

        // GetDeviceCapabilities
        service->SetStateVariable("PossiblePlaybackStorageMedia", "NONE,NETWORK");
        service->SetStateVariable("PossibleRecordStorageMedia", "NOT_IMPLEMENTED");
        service->SetStateVariable("PossibleRecordQualityModes", "NOT_IMPLEMENTED");

        // GetMediaInfo
        service->SetStateVariable("NumberOfTracks", "0");
        service->SetStateVariable("CurrentMediaDuration", "00:00:00");;
        service->SetStateVariable("AVTransportURI", "");
        service->SetStateVariable("AVTransportURIMetadata", "");;
        service->SetStateVariable("NextAVTransportURI", "NOT_IMPLEMENTED");
        service->SetStateVariable("NextAVTransportURIMetadata", "NOT_IMPLEMENTED");
        service->SetStateVariable("PlaybackStorageMedium", "NONE");
        service->SetStateVariable("RecordStorageMedium", "NOT_IMPLEMENTED");
        service->SetStateVariable("RecordMediumWriteStatus", "NOT_IMPLEMENTED");

        // GetPositionInfo
        service->SetStateVariable("CurrentTrack", "0");
        service->SetStateVariable("CurrentTrackDuration", "00:00:00");
        service->SetStateVariable("CurrentTrackMetadata", "");
        service->SetStateVariable("CurrentTrackURI", "");
        service->SetStateVariable("RelativeTimePosition", "00:00:00"); 
        service->SetStateVariable("AbsoluteTimePosition", "00:00:00");
        service->SetStateVariable("RelativeCounterPosition", "2147483647"); // means NOT_IMPLEMENTED
        service->SetStateVariable("AbsoluteCounterPosition", "2147483647"); // means NOT_IMPLEMENTED

        // disable indirect eventing for certain state variables
        PLT_StateVariable* var;
        var = service->FindStateVariable("RelativeTimePosition");
        if (var) var->DisableIndirectEventing();
        var = service->FindStateVariable("AbsoluteTimePosition");
        if (var) var->DisableIndirectEventing();
        var = service->FindStateVariable("RelativeCounterPosition");
        if (var) var->DisableIndirectEventing();
        var = service->FindStateVariable("AbsoluteCounterPosition");
        if (var) var->DisableIndirectEventing();

        // GetTransportInfo
        service->SetStateVariable("TransportState", "NO_MEDIA_PRESENT");
        service->SetStateVariable("TransportStatus", "OK");
        service->SetStateVariable("TransportPlaySpeed", "1");

        // GetTransportSettings
        service->SetStateVariable("CurrentPlayMode", "NORMAL");
        service->SetStateVariable("CurrentRecordQualityMode", "NOT_IMPLEMENTED");
    }

    {
        /* ConnectionManager */
        service = new PLT_Service(
            &data,
            "urn:schemas-upnp-org:service:ConnectionManager:1", 
            "urn:upnp-org:serviceId:CMGR_1-0");
        NPT_CHECK_FATAL(service->SetSCPDXML((const char*) RDR_ConnectionManagerSCPD));
        NPT_CHECK_FATAL(service->InitURLs("ConnectionManager", data.GetUUID()));
        NPT_CHECK_FATAL(data.AddService(service));

        service->SetStateVariable("CurrentConnectionIDs", "0");

        // put all supported mime types here instead
        service->SetStateVariable("SinkProtocolInfo", "http-get:*:*:*");
        service->SetStateVariable("SourceProtocolInfo", "");
    }

    {
        /* RenderingControl */
        service = new PLT_Service(
            &data,
            "urn:schemas-upnp-org:service:RenderingControl:1", 
            "urn:upnp-org:serviceId:RCS_1-0",
            "urn:schemas-upnp-org:metadata-1-0/RCS/");
        NPT_CHECK_FATAL(service->SetSCPDXML((const char*) RDR_RenderingControlSCPD));
        NPT_CHECK_FATAL(service->InitURLs("RenderingControl", data.GetUUID()));
        NPT_CHECK_FATAL(data.AddService(service));

        service->SetStateVariableRate("LastChange", NPT_TimeInterval(0.2f));

        service->SetStateVariable("Mute", "0");
        service->SetStateVariable("Volume", "100");
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnAction
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnAction(PLT_ActionReference&          action, 
                            const NPT_HttpRequestContext& context)
{
    NPT_COMPILER_UNUSED(context);

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

    /* Is it a RendererControl Service Action ? */
    if (serviceType.Compare("urn:schemas-upnp-org:service:RenderingControl:1", true) == 0) {
        /* we only support master channel */
        if (NPT_FAILED(action->VerifyArgumentValue("Channel", "Master"))) {
            action->SetError(402,"Invalid Args.");
            return NPT_FAILURE;
        }
    }

    if (name.Compare("GetVolume", true) == 0) {
        NPT_CHECK_SEVERE(action->SetArgumentsOutFromStateVariable());
        return NPT_SUCCESS;
    }

    if (name.Compare("GetMute", true) == 0) {
        NPT_CHECK_SEVERE(action->SetArgumentsOutFromStateVariable());
        return NPT_SUCCESS;
    }

    if (name.Compare("SetVolume", true) == 0) {
          return OnSetVolume(action);
    }

    if (name.Compare("SetMute", true) == 0) {
          return OnSetMute(action);
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

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnSetVolume
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnSetVolume(PLT_ActionReference& /* action */)
{
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnSetMute
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnSetMute(PLT_ActionReference& /* action */)
{
    return NPT_SUCCESS;
}
