/*****************************************************************
|
|   Platinum - SSDP Listener
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

#ifndef _PLT_SSDP_LISTENER_H_
#define _PLT_SSDP_LISTENER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"

/*----------------------------------------------------------------------
|   PLT_SsdpPacketListener class
+---------------------------------------------------------------------*/
class PLT_SsdpPacketListener
{
public:
    virtual ~PLT_SsdpPacketListener() {}
    virtual NPT_Result OnSsdpPacket(NPT_HttpRequest&              request, 
                                    const NPT_HttpRequestContext& context) = 0;
};

/*----------------------------------------------------------------------
|   PLT_SsdpSearchResponseListener class
+---------------------------------------------------------------------*/
class PLT_SsdpSearchResponseListener
{
public:
    virtual ~PLT_SsdpSearchResponseListener() {}
    virtual NPT_Result ProcessSsdpSearchResponse(NPT_Result                    res,  
                                                 const NPT_HttpRequestContext& context,
                                                 NPT_HttpResponse*             response) = 0;
};

#endif /* _PLT_SSDP_LISTENER_H_ */
