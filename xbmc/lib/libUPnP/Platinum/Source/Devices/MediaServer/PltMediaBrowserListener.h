/*****************************************************************
|
|   Platinum - AV Media Browser Listener
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

#ifndef _PLT_MEDIA_BROWSER_LISTENER_H_
#define _PLT_MEDIA_BROWSER_LISTENER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltMediaItem.h"
#include "PltDeviceData.h"

/*----------------------------------------------------------------------
|   PLT_BrowseInfo
+---------------------------------------------------------------------*/
class PLT_BrowseInfo {
public:
    PLT_BrowseInfo() : nr(0), tm(0), uid(0) {}

    NPT_String                   object_id;
    PLT_MediaObjectListReference items;
    NPT_UInt32                   nr;
    NPT_UInt32                   tm;
    NPT_UInt32                   uid;
};

/*----------------------------------------------------------------------
|   PLT_MediaBrowserListener class
+---------------------------------------------------------------------*/
class PLT_MediaBrowserListener
{
public:
    virtual ~PLT_MediaBrowserListener() {}

    virtual void OnMSAddedRemoved(
        PLT_DeviceDataReference& /*device*/, 
        int                      /*added*/) {}

    virtual void OnMSStateVariablesChanged(
        PLT_Service*                  /*service*/, 
        NPT_List<PLT_StateVariable*>* /*vars*/) {}

    virtual void OnMSBrowseResult(
        NPT_Result               /*res*/, 
        PLT_DeviceDataReference& /*device*/, 
        PLT_BrowseInfo*          /*info*/, 
        void*                    /*userdata*/) {}
};

#endif /* _PLT_MEDIA_BROWSER_LISTENER_H_ */
