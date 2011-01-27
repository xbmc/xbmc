/*****************************************************************
|
|   Platinum - UPnP Engine
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

#ifndef _PLT_UPNP_H_
#define _PLT_UPNP_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltTaskManager.h"
#include "PltCtrlPoint.h"
#include "PltDeviceHost.h"
#include "PltUPnPHelper.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define PLT_DLNA_SSDP_DELAY 0.02f

/*----------------------------------------------------------------------
|   forward definitions
+---------------------------------------------------------------------*/
class PLT_SsdpListenTask;

/*----------------------------------------------------------------------
|   PLT_UPnP class
+---------------------------------------------------------------------*/
class PLT_UPnP
{
public:
    PLT_UPnP(NPT_UInt32 ssdp_port = 1900, bool multicast = true);
    ~PLT_UPnP();

    NPT_Result AddDevice(PLT_DeviceHostReference& device);
    NPT_Result AddCtrlPoint(PLT_CtrlPointReference& ctrlpoint);

    NPT_Result RemoveDevice(PLT_DeviceHostReference& device);
    NPT_Result RemoveCtrlPoint(PLT_CtrlPointReference& ctrlpoint);

    NPT_Result Start();
    NPT_Result Stop();

	void       SetIgnoreLocalUUIDs(bool ignore) { m_IgnoreLocalUUIDs = ignore; }

private:
    // members
    NPT_Mutex                           m_Lock;
    NPT_List<PLT_DeviceHostReference>   m_Devices;
    NPT_List<PLT_CtrlPointReference>    m_CtrlPoints;
    PLT_TaskManager                     m_TaskManager;

    // since we can only have one socket listening on port 1900, 
    // we create it in here and we will attach every control points
    // and devices to it when they're added
    bool                                m_Started;
    NPT_UInt32                          m_Port;
    bool                                m_Multicast;
    PLT_SsdpListenTask*                 m_SsdpListenTask; 
	bool								m_IgnoreLocalUUIDs;
};

#endif /* _PLT_UPNP_H_ */
