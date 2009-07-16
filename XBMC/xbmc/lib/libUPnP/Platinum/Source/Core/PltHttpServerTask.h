/*****************************************************************
|
|   Platinum - HTTP Server Tasks
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

#ifndef _PLT_HTTP_SERVER_TASK_H_
#define _PLT_HTTP_SERVER_TASK_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltHttp.h"
#include "PltDatagramStream.h"
#include "PltThreadTask.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
template <class T> class PLT_HttpListenTask;
class PLT_HttpServerListener; 
typedef PLT_HttpListenTask<PLT_HttpServerListener> PLT_HttpServerListenTask;

/*----------------------------------------------------------------------
|   PLT_HttpServerSocketTask class
+---------------------------------------------------------------------*/
class PLT_HttpServerSocketTask : public PLT_ThreadTask
{
    friend class PLT_ThreadTask;

public:
    PLT_HttpServerSocketTask(NPT_Socket* socket, bool stay_alive_forever = false);

protected:
    virtual ~PLT_HttpServerSocketTask();

protected:
    // Request callback handler
    virtual NPT_Result ProcessRequest(NPT_HttpRequest&              request, 
                                      const NPT_HttpRequestContext& context,
                                      NPT_HttpResponse*&            response,
                                      bool&                         headers_only) = 0;

    // overridables
    virtual NPT_Result GetInputStream(NPT_InputStreamReference& stream);
    virtual NPT_Result GetInfo(NPT_SocketInfo& info);

    // PLT_ThreadTask methods
    virtual void DoAbort() { m_Socket->Disconnect(); }
    virtual void DoRun();

private:
    virtual NPT_Result Read(NPT_BufferedInputStreamReference& buffered_input_stream, 
                            NPT_HttpRequest*&                 request,
                            NPT_HttpRequestContext*           context = NULL);
    virtual NPT_Result Write(NPT_HttpResponse* response, 
                             bool&             keep_alive, 
                             bool              headers_only = false);

protected:
    NPT_Socket*         m_Socket;
    bool                m_StayAliveForever;
};

/*----------------------------------------------------------------------
|   PLT_HttpServerTask class
+---------------------------------------------------------------------*/
template <class T>
class PLT_HttpServerTask : public PLT_HttpServerSocketTask
{
public:
    PLT_HttpServerTask<T>(T*          data, 
                          NPT_Socket* socket, 
                          bool        keep_alive = false) : 
        PLT_HttpServerSocketTask(socket, keep_alive), m_Data(data) {}

protected:
    virtual ~PLT_HttpServerTask<T>() {}

protected:
    NPT_Result ProcessRequest(NPT_HttpRequest&              request, 
                              const NPT_HttpRequestContext& context,
                              NPT_HttpResponse*&            response,
                              bool&                         headers_only) {
        return m_Data->ProcessHttpRequest(request, context, response, headers_only);
    }

protected:
    T* m_Data;
};

/*----------------------------------------------------------------------
|   PLT_HttpListenTask class
+---------------------------------------------------------------------*/
template <class T>
class PLT_HttpListenTask : public PLT_ThreadTask
{
public:
    PLT_HttpListenTask<T>(T* data, NPT_TcpServerSocket* socket, bool cleanup_socket = true) : 
        m_Data(data), m_Socket(socket), m_CleanupSocket(cleanup_socket) {}

protected:
    virtual ~PLT_HttpListenTask<T>() { 
        if (m_CleanupSocket) delete m_Socket;
    }

protected:
    // PLT_ThreadTask methods
    virtual void DoAbort() { m_Socket->Disconnect(); }
    virtual void DoRun() {
        while (!IsAborting(0)) {
            NPT_Socket* client = NULL;
            NPT_Result  result = m_Socket->WaitForNewClient(client, 5000);
            if (NPT_FAILED(result) && result != NPT_ERROR_TIMEOUT) {
                if (client) delete client;
                //NPT_LOG_WARNING_2("PLT_HttpListenTask exiting with %d (%s)", result, NPT_ResultText(result));
                break;
            }

            if (NPT_SUCCEEDED(result)) {
                PLT_ThreadTask* task = new PLT_HttpServerTask<T>(m_Data, client);
                if (NPT_FAILED(m_TaskManager->StartTask(task))) {
                    task->Kill();
                }
            }
        }
    }

protected:
    T*                   m_Data;
    NPT_TcpServerSocket* m_Socket;
    bool                 m_CleanupSocket;
};

#endif /* _PLT_HTTP_SERVER_TASK_H_ */
