/*****************************************************************
|
|   Platinum - HTTP Server
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

#ifndef _PLT_HTTP_SERVER_H_
#define _PLT_HTTP_SERVER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltHttpServerListener.h"
#include "PltHttpServerTask.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_HttpServerStartIterator;

/*----------------------------------------------------------------------
|   PLT_HttpServer class
+---------------------------------------------------------------------*/
class PLT_HttpServer : public PLT_HttpServerListener,
                       public NPT_HttpServer
{
    friend class PLT_HttpServerTask<class PLT_HttpServer>;
    friend class PLT_HttpServerStartIterator;

public:
    PLT_HttpServer(unsigned int port = 0,
                   bool         port_rebind = false,
                   NPT_Cardinal max_clients = 0,
                   bool         reuse_address = false);
    virtual ~PLT_HttpServer();

    // PLT_HttpServerListener method
    virtual NPT_Result ProcessHttpRequest(NPT_HttpRequest&              request, 
                                          const NPT_HttpRequestContext& context,
                                          NPT_HttpResponse*&            response,
                                          bool&                         headers_only);

    virtual NPT_Result   Start();
    virtual NPT_Result   Stop();
    virtual unsigned int GetPort() { return m_Port; }

private:
    PLT_TaskManager*          m_TaskManager;
    unsigned int              m_Port;
    bool                      m_PortRebind;
    bool                      m_ReuseAddress;
    PLT_HttpServerListenTask* m_HttpListenTask;
};

/*----------------------------------------------------------------------
|   PLT_FileServer class
+---------------------------------------------------------------------*/
class PLT_FileServer
{
public:
    // class methods
    static NPT_Result ServeFile(NPT_HttpResponse& response, 
                                NPT_String        file_path, 
                                NPT_Position      start = (NPT_Position)-1, 
                                NPT_Position      end = (NPT_Position)-1,
                                bool              request_is_head = false);
    static const char* GetMimeType(const NPT_String& filename);
};

#endif /* _PLT_HTTP_SERVER_H_ */
