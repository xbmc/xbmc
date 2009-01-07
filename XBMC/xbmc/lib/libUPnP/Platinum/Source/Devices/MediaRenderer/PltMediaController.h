/*****************************************************************
|
|   Platinum - AV Media Controller (Media Renderer Control Point)
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

#ifndef _PLT_MEDIA_CONTROLLER_H_
#define _PLT_MEDIA_CONTROLLER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltCtrlPoint.h"
#include "PltMediaControllerListener.h"

/*----------------------------------------------------------------------
|   PLT_MediaController class
+---------------------------------------------------------------------*/
class PLT_MediaController : public PLT_CtrlPointListener
{
public:
    PLT_MediaController(PLT_CtrlPointReference& ctrl_point, PLT_MediaControllerListener* listener);
    virtual ~PLT_MediaController();

    // PLT_CtrlPointListener methods
    virtual NPT_Result OnDeviceAdded(PLT_DeviceDataReference& device);
    virtual NPT_Result OnDeviceRemoved(PLT_DeviceDataReference& device);
    virtual NPT_Result OnActionResponse(NPT_Result res, PLT_ActionReference& action, void* userdata);
    virtual NPT_Result OnEventNotify(PLT_Service* service, NPT_List<PLT_StateVariable*>* vars);

    // AVTransport
    NPT_Result GetCurrentTransportActions(PLT_DeviceDataReference& device, NPT_UInt32 instance_id, void* userdata);
    NPT_Result GetDeviceCapabilities(PLT_DeviceDataReference& device, NPT_UInt32 instance_id, void* userdata);
    NPT_Result GetMediaInfo(PLT_DeviceDataReference& device, NPT_UInt32 instance_id, void* userdata);
    NPT_Result GetPositionInfo(PLT_DeviceDataReference& device,  NPT_UInt32 instance_id, void* userdata);
    NPT_Result GetTransportInfo(PLT_DeviceDataReference& device, NPT_UInt32 instance_id, void* userdata);
    NPT_Result GetTransportSettings(PLT_DeviceDataReference& device, NPT_UInt32 instance_id, void* userdata);
    NPT_Result Next(PLT_DeviceDataReference& device, NPT_UInt32 instance_id, void* userdata);
    NPT_Result Pause(PLT_DeviceDataReference& device, NPT_UInt32 instance_id, void* userdata);
    NPT_Result Play(PLT_DeviceDataReference&  device, NPT_UInt32 instance_id, NPT_String speed, void* userdata);
    NPT_Result Previous(PLT_DeviceDataReference& device, NPT_UInt32 instance_id, void* userdata);
    NPT_Result Seek(PLT_DeviceDataReference&  device, NPT_UInt32 instance_id, NPT_String unit, NPT_String target, void* userdata);
    NPT_Result SetAVTransportURI(PLT_DeviceDataReference& device, NPT_UInt32 instance_id, const char* uri, const char* metadata, void* userdata);
    NPT_Result SetPlayMode(PLT_DeviceDataReference&  device, NPT_UInt32 instance_id, NPT_String new_play_mode, void* userdata);
    NPT_Result Stop(PLT_DeviceDataReference& device, NPT_UInt32 instance_id, void* userdata);

    // ConnectionManager
    NPT_Result GetCurrentConnectionIDs(PLT_DeviceDataReference& device, void* userdata);
    NPT_Result GetCurrentConnectionInfo(PLT_DeviceDataReference& device, NPT_UInt32 connection_id, void* userdata);
    NPT_Result GetProtocolInfo(PLT_DeviceDataReference& device, void* userdata);

private:
    NPT_Result FindActionDesc(PLT_DeviceDataReference& device, 
        const char*              service_type,
        const char*              action_name,
        PLT_ActionDesc*&         action_desc);

    NPT_Result CreateAction(PLT_DeviceDataReference& device, 
        const char*              service_type,
        const char*              action_name,
        PLT_ActionReference&     action);

    NPT_Result CallAVTransportAction(PLT_ActionReference& action,
        NPT_UInt32               instance_id,
        void*                    userdata = NULL);

    NPT_Result OnGetCurrentTransportActionsResponse(NPT_Result res, PLT_DeviceDataReference& device, PLT_ActionReference& action, void* userdata);
    NPT_Result OnGetDeviceCapabilitiesResponse(NPT_Result res, PLT_DeviceDataReference& device, PLT_ActionReference& action, void* userdata);
    NPT_Result OnGetMediaInfoResponse(NPT_Result res, PLT_DeviceDataReference& device, PLT_ActionReference& action, void* userdata);
    NPT_Result OnGetPositionInfoResponse(NPT_Result res, PLT_DeviceDataReference& device, PLT_ActionReference& action, void* userdata);
    NPT_Result OnGetTransportInfoResponse(NPT_Result res, PLT_DeviceDataReference& device, PLT_ActionReference& action, void* userdata);
    NPT_Result OnGetTransportSettingsResponse(NPT_Result res, PLT_DeviceDataReference& device, PLT_ActionReference& action, void* userdata);

    NPT_Result OnGetCurrentConnectionIDsResponse(NPT_Result res, PLT_DeviceDataReference& device, PLT_ActionReference& action, void* userdata);
    NPT_Result OnGetCurrentConnectionInfoResponse(NPT_Result res, PLT_DeviceDataReference& device, PLT_ActionReference& action, void* userdata);
    NPT_Result OnGetProtocolInfoResponse(NPT_Result res, PLT_DeviceDataReference& device, PLT_ActionReference& action, void* userdata);

    static void ParseCSV(const char* csv, PLT_StringList& values) {
        const char* start = csv;
        const char* p = start;

        // look for the , character
        while (*p) {
            if (*p == ',') {
                NPT_String val(start, (int)(p-start));
                val.Trim(' ');
                values.Add(val);
                start = p + 1;
            }
            p++;
        }

        // last one
        NPT_String last(start, (int)(p-start));
        last.Trim(' ');
        if (last.GetLength()) {
            values.Add(last);
        }
    }

protected:
    PLT_CtrlPointReference       m_CtrlPoint;
    PLT_MediaControllerListener* m_Listener;

private:
    NPT_List<PLT_DeviceDataReference> m_MediaRenderers;
};

#endif /* _PLT_MEDIA_CONTROLLER_H_ */
