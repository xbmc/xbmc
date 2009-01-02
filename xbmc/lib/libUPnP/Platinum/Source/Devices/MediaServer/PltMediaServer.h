/*****************************************************************
|
|   Platinum - AV Media Server Device
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
    PLT_MediaServer(const char*  friendly_name,
                    bool         show_ip = false,
                    const char*  uuid = NULL,
                    NPT_UInt16   port = 0);
    virtual ~PLT_MediaServer();

    // PLT_DeviceHost methods
    virtual NPT_Result SetupServices(PLT_DeviceData& data);
    virtual NPT_Result OnAction(PLT_ActionReference&          action, 
                                const NPT_HttpRequestContext& context);

    // class methods
    static NPT_Result  GetBrowseFlag(const char* str, BrowseFlags& flag);

protected:
    // ConnectionManager
    virtual NPT_Result OnGetCurrentConnectionIDs(PLT_ActionReference& action, const NPT_HttpRequestContext& context);
    virtual NPT_Result OnGetProtocolInfo(PLT_ActionReference& action, const NPT_HttpRequestContext& context);
    virtual NPT_Result OnGetCurrentConnectionInfo(PLT_ActionReference& action, const NPT_HttpRequestContext& context);

    // ContentDirectory
    virtual NPT_Result OnGetSortCapabilities(PLT_ActionReference& action, const NPT_HttpRequestContext& context);
    virtual NPT_Result OnGetSearchCapabilities(PLT_ActionReference& action, const NPT_HttpRequestContext& context);
    virtual NPT_Result OnGetSystemUpdateID(PLT_ActionReference& action, const NPT_HttpRequestContext& context);
    virtual NPT_Result OnBrowse(PLT_ActionReference& action, const NPT_HttpRequestContext& context);
    virtual NPT_Result OnSearch(PLT_ActionReference& action, const NPT_HttpRequestContext& context);

    // overridable methods
    virtual NPT_Result OnBrowseMetadata(PLT_ActionReference& action, const char* object_id, const NPT_HttpRequestContext& context);
    virtual NPT_Result OnBrowseDirectChildren(PLT_ActionReference& action, const char* object_id, const NPT_HttpRequestContext& context);
    virtual NPT_Result OnSearch(PLT_ActionReference& action, const NPT_String& object_id, const NPT_String& searchCriteria, const NPT_HttpRequestContext& context);
};

#endif /* _PLT_MEDIA_SERVER_H_ */
