/*****************************************************************
|
|   Platinum - Downloader
|
| Copyright (c) 2004-2010, Plutinosoft, LLC.
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
| licensing@plutinosoft.com
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

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltDownloader.h"
#include "PltTaskManager.h"
#include "NptLogging.h"


NPT_SET_LOCAL_LOGGER("platinum.extra.downloader")

/*----------------------------------------------------------------------
|   PLT_Downloader::PLT_Downloader
+---------------------------------------------------------------------*/
PLT_Downloader::PLT_Downloader(NPT_HttpUrl&               url, 
                               NPT_OutputStreamReference& output) :
    PLT_HttpClientSocketTask(new NPT_HttpRequest(url,
                                                 "GET",
                                                 NPT_HTTP_PROTOCOL_1_1)),
    m_URL(url),
    m_Output(output),
    m_State(PLT_DOWNLOADER_IDLE)
{
}
    
/*----------------------------------------------------------------------
|   PLT_Downloader::~PLT_Downloader
+---------------------------------------------------------------------*/
PLT_Downloader::~PLT_Downloader()
{
}

/*----------------------------------------------------------------------
|   PLT_Downloader::DoRun
+---------------------------------------------------------------------*/
void
PLT_Downloader::DoRun()
{
    m_State = PLT_DOWNLOADER_STARTED;
    return PLT_HttpClientSocketTask::DoRun();
}

/*----------------------------------------------------------------------
|   PLT_Downloader::DoAbort
+---------------------------------------------------------------------*/
void
PLT_Downloader::DoAbort()
{
    PLT_HttpClientSocketTask::DoAbort();
    m_State = PLT_DOWNLOADER_IDLE;
}

/*----------------------------------------------------------------------
|   PLT_Downloader::ProcessResponse
+---------------------------------------------------------------------*/
NPT_Result 
PLT_Downloader::ProcessResponse(NPT_Result                    res, 
                                const NPT_HttpRequest&        request, 
                                const NPT_HttpRequestContext& context, 
                                NPT_HttpResponse*             response)
{
    NPT_COMPILER_UNUSED(request);
    NPT_COMPILER_UNUSED(context);

    if (NPT_FAILED(res)) {
        NPT_LOG_WARNING_2("Downloader error %d for %s", res, m_URL.ToString().GetChars());
        m_State = PLT_DOWNLOADER_ERROR;
        return res;
    }

    m_State = PLT_DOWNLOADER_DOWNLOADING;

    NPT_HttpEntity* entity;
    NPT_InputStreamReference body;
    if (!response || 
        !(entity = response->GetEntity()) || 
        NPT_FAILED(entity->GetInputStream(body)) || 
        body.IsNull()) {
        m_State = PLT_DOWNLOADER_ERROR;
        NPT_LOG_WARNING_2("No body %d for %s", res, m_URL.ToString().GetChars());
        return NPT_FAILURE;
    }

    // Read body (no content length means until socket is closed)
    res = NPT_StreamToStreamCopy(*body.AsPointer(), 
        *m_Output.AsPointer(), 
        0, 
        entity->GetContentLength());

    if (NPT_FAILED(res)) {
        NPT_LOG_WARNING_2("Downloader error %d for %s", res, m_URL.ToString().GetChars());
        m_State = PLT_DOWNLOADER_ERROR;
        return res;
    }
    
    NPT_LOG_INFO_1("Finished downloading %s", m_URL.ToString().GetChars());
    m_State = PLT_DOWNLOADER_SUCCESS;
    return NPT_SUCCESS;
}
