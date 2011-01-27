/*****************************************************************
|
|   Platinum - HTTP Client Tasks
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

#ifndef _PLT_HTTP_CLIENT_TASK_H_
#define _PLT_HTTP_CLIENT_TASK_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltHttp.h"
#include "PltThreadTask.h"

/*----------------------------------------------------------------------
|   PLT_HttpTcpConnector
+---------------------------------------------------------------------*/
class PLT_HttpTcpConnector : public NPT_HttpClient::Connector
{
public:
    PLT_HttpTcpConnector();
    virtual ~PLT_HttpTcpConnector();
    virtual NPT_Result Connect(const char*                   hostname, 
                               NPT_UInt16                    port, 
                               NPT_Timeout                   connection_timeout,
                               NPT_Timeout                   io_timeout,
                               NPT_Timeout                   name_resolver_timeout,
                               NPT_InputStreamReference&     input_stream, 
                               NPT_OutputStreamReference&    output_stream);

public:
    void    GetInfo(NPT_SocketInfo& info) { info = m_SocketInfo;}
    void    Abort() { if (!m_Socket.IsNull()) m_Socket->Disconnect(); }

private:
    NPT_String                 m_HostName;
    NPT_UInt16                 m_Port;
    NPT_InputStreamReference   m_InputStream;
    NPT_OutputStreamReference  m_OutputStream;
    NPT_SocketInfo             m_SocketInfo;
    NPT_Reference<NPT_Socket>  m_Socket;
};

/*----------------------------------------------------------------------
|   PLT_HttpClientSocketTask class
+---------------------------------------------------------------------*/
class PLT_HttpClientSocketTask : public PLT_ThreadTask
{
friend class PLT_ThreadTask;

public:
    PLT_HttpClientSocketTask(NPT_HttpRequest* request = NULL, 
                             bool             wait_forever = false);

    virtual NPT_Result AddRequest(NPT_HttpRequest* request);

protected:
    virtual ~PLT_HttpClientSocketTask();

protected:
    virtual NPT_Result SetConnector(PLT_HttpTcpConnector* connector);
    
    // PLT_ThreadTask methods
    virtual void DoAbort();
    virtual void DoRun();

    virtual NPT_Result ProcessResponse(NPT_Result                    res, 
                                       NPT_HttpRequest*              request, 
                                       const NPT_HttpRequestContext& context,
                                       NPT_HttpResponse*             response);

private:
    NPT_Result GetNextRequest(NPT_HttpRequest*& request, NPT_Timeout timeout);

protected:
    NPT_HttpClient              m_Client;
    bool                        m_WaitForever;
    NPT_Queue<NPT_HttpRequest>  m_Requests;
    NPT_Mutex                   m_ConnectorLock;
    PLT_HttpTcpConnector*       m_Connector;
};

/*----------------------------------------------------------------------
|   PLT_HttpClientTask class
+---------------------------------------------------------------------*/
template <class T>
class PLT_HttpClientTask : public PLT_HttpClientSocketTask
{
public:
    PLT_HttpClientTask<T>(const NPT_HttpUrl& url, T* data) : 
        PLT_HttpClientSocketTask(new NPT_HttpRequest(url, "GET", NPT_HTTP_PROTOCOL_1_1)), 
                                 m_Data(data) {}
 protected:
    virtual ~PLT_HttpClientTask<T>() {}

protected:
    // PLT_HttpClientSocketTask method
    NPT_Result ProcessResponse(NPT_Result                    res, 
                               NPT_HttpRequest*              request, 
                               const NPT_HttpRequestContext& context, 
                               NPT_HttpResponse*             response) {
        return m_Data->ProcessResponse(res, request, context, response);
    }

protected:
    T* m_Data;
};

/*----------------------------------------------------------------------
|   PLT_FileHttpClientTask class
+---------------------------------------------------------------------*/
class PLT_FileHttpClientTask : public PLT_HttpClientSocketTask
{
public:
    PLT_FileHttpClientTask(const NPT_HttpUrl& url) : 
        PLT_HttpClientSocketTask(new NPT_HttpRequest(url, "GET", NPT_HTTP_PROTOCOL_1_1)) {}

protected:
    virtual ~PLT_FileHttpClientTask() {}

protected:
    // PLT_HttpClientSocketTask method
    NPT_Result ProcessResponse(NPT_Result                    res, 
                               NPT_HttpRequest*              request, 
                               const NPT_HttpRequestContext& context, 
                               NPT_HttpResponse*             response);
};

#endif /* _PLT_HTTP_CLIENT_TASK_H_ */
