/*****************************************************************
|
|   Platinum - Frame Server
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

#ifndef _PLT_FRAME_SERVER_H_
#define _PLT_FRAME_SERVER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltFileMediaServer.h"
#include "PltFrameBuffer.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_SocketPolicyServer;

/*----------------------------------------------------------------------
|   PLT_StreamValidator class
+---------------------------------------------------------------------*/
class PLT_StreamValidator
{
public:
    virtual ~PLT_StreamValidator() {}
    virtual bool OnNewRequestAccept(const NPT_HttpRequestContext& context) = 0;
};

/*----------------------------------------------------------------------
|   PLT_FrameServer class
+---------------------------------------------------------------------*/
class PLT_FrameServer : public PLT_FileMediaServer
{
public:
    PLT_FrameServer(PLT_FrameBuffer& frame_buffer, 
                    const char*      www_root,
                    const char*      friendly_name,
                    bool             show_ip = false,
                    const char*      uuid = NULL,
                    NPT_UInt16       port = 0,
                    PLT_StreamValidator* stream_validator = NULL);

protected:
    virtual ~PLT_FrameServer();

    // overridable
    virtual NPT_Result ProcessStreamRequest(NPT_HttpRequest&              request, 
                                            const NPT_HttpRequestContext& context,
                                            NPT_HttpResponse&             response);
    // PLT_DeviceHost methods
    virtual NPT_Result SetupDevice();
    virtual NPT_Result ProcessHttpRequest(NPT_HttpRequest&              request, 
                                          const NPT_HttpRequestContext& context,
                                          NPT_HttpResponse&             response);
    
    // PLT_FileMediaServer methods
    virtual NPT_String BuildResourceUri(const NPT_HttpUrl& base_uri, const char* host, const char* file_path);

    // PLT_MediaServer methods
    virtual NPT_Result OnBrowseMetadata(PLT_ActionReference&          action, 
                                        const char*                   object_id, 
                                        const char*                   filter,
                                        NPT_UInt32                    starting_index,
                                        NPT_UInt32                    requested_count,
                                        const NPT_List<NPT_String>&   sort_criteria,
                                        const PLT_HttpRequestContext& context);
    virtual NPT_Result OnBrowseDirectChildren(PLT_ActionReference&          action, 
                                              const char*                   object_id, 
                                              const char*                   filter,
                                              NPT_UInt32                    starting_index,
                                              NPT_UInt32                    requested_count,
                                              const NPT_List<NPT_String>&   sort_criteria,
                                              const PLT_HttpRequestContext& context);
    virtual PLT_MediaObject* BuildFromID(const char* id,
                                         const NPT_SocketAddress* req_local_address /* = NULL */);

private:
    NPT_Result Replace(const char* input, NPT_MemoryStream& output);
    NPT_Result ProcessTemplate(NPT_HttpRequest&              request, 
                               const NPT_HttpRequestContext& context,
                               NPT_HttpResponse&             response);
protected:
    friend class PLT_MediaItem;

    PLT_FrameBuffer&        m_FrameBuffer;
    NPT_HttpUrl             m_StreamBaseUri;
    PLT_SocketPolicyServer* m_PolicyServer;
    PLT_StreamValidator*    m_StreamValidator;
};

#endif /* _PLT_FRAME_SERVER_H_ */
