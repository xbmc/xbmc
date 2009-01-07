/*****************************************************************
|
|   Platinum - AV Media Browser (Media Server Control Point)
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

#ifndef _PLT_MEDIA_BROWSER_H_
#define _PLT_MEDIA_BROWSER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltCtrlPoint.h"
#include "PltMediaItem.h"
#include "PltMediaBrowserListener.h"

/*----------------------------------------------------------------------
|   PLT_MediaBrowser class
+---------------------------------------------------------------------*/
class PLT_MediaBrowser : public PLT_CtrlPointListener
{
public:
    PLT_MediaBrowser(PLT_CtrlPointReference&   ctrl_point, 
                     PLT_MediaBrowserListener* listener);
    virtual ~PLT_MediaBrowser();

    // PLT_CtrlPointListener methods
    virtual NPT_Result OnDeviceAdded(PLT_DeviceDataReference& device);
    virtual NPT_Result OnDeviceRemoved(PLT_DeviceDataReference& device);
    virtual NPT_Result OnActionResponse(NPT_Result res, PLT_ActionReference& action, void* userdata);
    virtual NPT_Result OnEventNotify(PLT_Service* service, NPT_List<PLT_StateVariable*>* vars);

    // ContentDirectory service
    NPT_Result Browse(
        PLT_DeviceDataReference& device, 
        const char*              object_id, 
        NPT_UInt32               start_index,
        NPT_UInt32               count,
        bool                     browse_metadata = false,
        const char*              filter = "*",
        const char*              sort_criteria = "",
        void*                    userdata = NULL);

    // accessor methods
    const NPT_Lock<PLT_DeviceDataReferenceList>& GetMediaServers() { 
        return m_MediaServers; 
    }

protected:
    // ContentDirectory service responses
    virtual NPT_Result OnBrowseResponse(
        NPT_Result               res, 
        PLT_DeviceDataReference& device, 
        PLT_ActionReference&     action, 
        void*                    userdata);

protected:
    PLT_CtrlPointReference                m_CtrlPoint;
    PLT_MediaBrowserListener*             m_Listener;
    NPT_Lock<PLT_DeviceDataReferenceList> m_MediaServers;
};

#endif /* _PLT_MEDIA_BROWSER_H_ */
