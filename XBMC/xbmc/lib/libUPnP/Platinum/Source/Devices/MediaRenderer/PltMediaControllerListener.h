/*****************************************************************
|
|   Platinum - AV Media Controller Listener
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

#ifndef _PLT_MEDIA_CONTROLLER_LISTENER_H_
#define _PLT_MEDIA_CONTROLLER_LISTENER_H_

/*----------------------------------------------------------------------
|   Defines
+---------------------------------------------------------------------*/
typedef NPT_List<NPT_String> PLT_StringList;

struct PLT_DeviceCapabilities {
    PLT_StringList play_media;
    PLT_StringList rec_media;
    PLT_StringList rec_quality_modes;
};

struct PLT_MediaInfo {
    NPT_UInt32    num_tracks;
    NPT_TimeStamp media_duration;
    NPT_String    cur_uri;
    NPT_String    cur_metadata;
    NPT_String    next_uri;
    NPT_String    next_metadata;
    NPT_String    play_medium;
    NPT_String    rec_medium;
    NPT_String    write_status;
};

struct PLT_PositionInfo {
    NPT_UInt32    track;
    NPT_TimeStamp track_duration;
    NPT_String    track_metadata;
    NPT_String    track_uri;
    NPT_TimeStamp rel_time;
    NPT_TimeStamp abs_time;
    NPT_Int32     rel_count;
    NPT_Int32     abs_count;
};

struct PLT_TransportInfo {
    NPT_String cur_transport_state;
    NPT_String cur_transport_status;
    NPT_String cur_speed;
};

struct PLT_TransportSettings {
    NPT_String play_mode;
    NPT_String rec_quality_mode;
};

struct PLT_ConnectionInfo {
    NPT_UInt32 rcs_id;
    NPT_UInt32 avtransport_id;
    NPT_String protocol_info;
    NPT_String peer_connection_mgr;
    NPT_UInt32 peer_connection_id;
    NPT_String direction;
    NPT_String status;
};

/*----------------------------------------------------------------------
|   PLT_MediaControllerListener class
+---------------------------------------------------------------------*/
class PLT_MediaControllerListener
{
public:
    virtual ~PLT_MediaControllerListener() {}

    virtual void OnMRAddedRemoved(PLT_DeviceDataReference& /* device */, 
                                  int                      /* added */) {}
    virtual void OnMRStateVariablesChanged(PLT_Service*                  /* service */, 
                                           NPT_List<PLT_StateVariable*>* /* vars */) {}

    // AVTransport
    virtual void OnGetCurrentTransportActionsResult(
        NPT_Result               /* res */, 
        PLT_DeviceDataReference& /* device */,
        PLT_StringList*          /* actions */, 
        void*                    /* userdata */) {}

    virtual void OnGetDeviceCapabilitiesResult(
        NPT_Result               /* res */, 
        PLT_DeviceDataReference& /* device */,
        PLT_DeviceCapabilities*  /* capabilities */,
        void*                    /* userdata */) {}

    virtual void OnGetMediaInfoResult(
        NPT_Result               /* res */,
        PLT_DeviceDataReference& /* device */,
        PLT_MediaInfo*           /* info */,
        void*                    /* userdata */) {}

    virtual void OnGetPositionInfoResult(
        NPT_Result               /* res */,
        PLT_DeviceDataReference& /* device */,
        PLT_PositionInfo*        /* info */,
        void*                    /* userdata */) {}

    virtual void OnGetTransportInfoResult(
        NPT_Result               /* res */,
        PLT_DeviceDataReference& /* device */,
        PLT_TransportInfo*       /* info */,
        void*                    /* userdata */) {}

    virtual void OnGetTransportSettingsResult(
        NPT_Result               /* res */,
        PLT_DeviceDataReference& /* device */,
        PLT_TransportSettings*   /* settings */,
        void*                    /* userdata */) {}

    virtual void OnNextResult(
        NPT_Result               /* res */,
        PLT_DeviceDataReference& /* device */,
        void*                    /* userdata */) {}

    virtual void OnPauseResult(
        NPT_Result               /* res */,
        PLT_DeviceDataReference& /* device */,
        void*                    /* userdata */) {}  

    virtual void OnPlayResult(
        NPT_Result               /* res */,
        PLT_DeviceDataReference& /* device */,
        void*                    /* userdata */) {}

    virtual void OnPreviousResult(
        NPT_Result               /* res */,
        PLT_DeviceDataReference& /* device */,
        void*                    /* userdata */) {}

    virtual void OnSeekResult(
        NPT_Result               /* res */,
        PLT_DeviceDataReference& /* device */,
        void*                    /* userdata */) {}

    virtual void OnSetAVTransportURIResult(
        NPT_Result               /* res */,
        PLT_DeviceDataReference& /* device */,
        void*                    /* userdata */) {}

    virtual void OnSetPlayModeResult(
        NPT_Result               /* res */,
        PLT_DeviceDataReference& /* device */,
        void*                    /* userdata */) {}

    virtual void OnStopResult(
        NPT_Result               /* res */,
        PLT_DeviceDataReference& /* device */,
        void*                    /* userdata */) {}

    // ConnectionManager
    virtual void OnGetCurrentConnectionIDsResult(
        NPT_Result               /* res */,
        PLT_DeviceDataReference& /* device */,
        PLT_StringList*          /* ids */,
        void*                    /* userdata */) {}

    virtual void OnGetCurrentConnectionInfoResult(
        NPT_Result               /* res */,
        PLT_DeviceDataReference& /* device */,
        PLT_ConnectionInfo*      /* info */,
        void*                    /* userdata */) {}

    virtual void OnGetProtocolInfoResult(
        NPT_Result               /* res */,
        PLT_DeviceDataReference& /* device */,
        PLT_StringList*          /* sources */,
        PLT_StringList*          /* sinks */,
        void*                    /* userdata */) {}
};

#endif /* _PLT_MEDIA_CONTROLLER_LISTENER_H_ */
