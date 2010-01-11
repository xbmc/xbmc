/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "WebServer.h"
#include "../lib/libjsonrpc/JSONRPC.h"
#include "../lib/libhttpapi/HttpApi.h"
#include "../FileSystem/File.h"
#include "../Util.h"
#include "log.h"
#include "SingleLock.h"

using namespace XFILE;
using namespace std;
using namespace JSONRPC;

CWebServer::CWebServer()
{
  m_running = false;
  m_daemon = NULL;
  m_needcredentials = true;
  m_Credentials64Encoded = "eGJtYzp4Ym1j"; // xbmc:xbmc
}

int CWebServer::FillArgumentMap(void *cls, enum MHD_ValueKind kind, const char *key, const char *value) 
{
  map<CStdString, CStdString> *arguments = (map<CStdString, CStdString> *)cls;
  arguments->insert( pair<CStdString,CStdString>(key,value) );

  return MHD_YES; 
} 

int CWebServer::AskForAuthentication (struct MHD_Connection *connection)
{
  int ret;
  struct MHD_Response *response;

  response = MHD_create_response_from_data (0, NULL, MHD_NO, MHD_NO);
  if (!response)
    return MHD_NO;

  ret = MHD_add_response_header (response, "WWW-Authenticate", "Basic realm=XBMC");
  if (!ret)
  {
    MHD_destroy_response (response);
    return MHD_NO;
  }

  ret = MHD_queue_response (connection, MHD_HTTP_UNAUTHORIZED, response);

  MHD_destroy_response (response);

  return ret;
}

bool CWebServer::IsAuthenticated (CWebServer *server, struct MHD_Connection *connection)
{
  CSingleLock lock (server->m_critSection);
  if (!server->m_needcredentials)
    return true;

  const char *strbase = "Basic ";
  const char *headervalue = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
  if (NULL == headervalue)
    return false;
  if (strncmp (headervalue, strbase, strlen(strbase)))
    return false;

  return server->m_Credentials64Encoded.Equals(headervalue + strlen(strbase));
}

int CWebServer::answer_to_connection (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{
  CWebServer *server = (CWebServer *)cls;

  if (!IsAuthenticated(server, connection)) 
    return AskForAuthentication(connection); 

  CLog::Log(LOGNOTICE, "WebServer: %s | %s", method, url);

  CStdString strURL = url;
#ifdef HAS_JSONRPC
  if (strURL.Equals("/jsonrpc") && strcmp (method, "POST") == 0)
  {
    char jsoncall[*upload_data_size + 1];
    memcpy(jsoncall, upload_data, *upload_data_size);
    jsoncall[*upload_data_size] = '\0';
    if (*upload_data_size > 204800)
      CLog::Log(LOGINFO, "JSONRPC: Recieved a jsonrpc call wich is bigger than 200KiB, skipping logging it");
    else
      CLog::Log(LOGINFO, "JSONRPC: Recieved a jsonrpc call - %s", jsoncall);
    printf("%s\n", jsoncall);
    CHTTPClient client;
    CStdString jsonresponse = CJSONRPC::MethodCall(jsoncall, (ITransportLayer *)cls, &client);

    struct MHD_Response *response = MHD_create_response_from_data(jsonresponse.length(), (void *) jsonresponse.c_str(), MHD_NO, MHD_YES);
    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
  }
#endif
  if (strcmp(method, "GET") == 0)
  {
    if (strURL.Left(6).Equals("/thumb"))
    {
      strURL = strURL.Right(strURL.length() - 7);
      strURL = strURL.Left(strURL.length() - 4);
    }
    else if (strURL.Left(4).Equals("/vfs"))
    {
      strURL = strURL.Right(strURL.length() - 5);
      CUtil::UrlDecode(strURL);
    }
#ifdef HAS_HTTPAPI
    else if (strURL.Left(18).Equals("/xbmcCmds/xbmcHttp"))
    {
      printf("getting argumentkinds\n");
      map<CStdString, CStdString> arguments;
      if (MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, FillArgumentMap, &arguments) > 0)
      {
        CStdString httpapiresponse = CHttpApi::MethodCall(arguments["command"], arguments["parameter"]);

        struct MHD_Response *response = MHD_create_response_from_data(httpapiresponse.length(), (void *) httpapiresponse.c_str(), MHD_NO, MHD_YES);
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);

        return ret;
      }
    }
#endif
#ifdef HAS_WEB_INTERFACE
    else if (strURL.Equals("/"))
      strURL = "special://home/web/index.html";
    else
      strURL.Format("special://home/web%s", strURL.c_str());
#endif
    CFile *file = new CFile();
    if (file->Open(strURL))
    {
      struct MHD_Response *response = MHD_create_response_from_callback ( file->GetLength(),
                                                                          2048,
                                                                          &CWebServer::ContentReaderCallback, file,
                                                                          &CWebServer::ContentReaderFreeCallback);
      int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
      MHD_destroy_response(response);
      return ret;
    }
    else
    {
      CLog::Log(LOGERROR, "WebServer: Failed to open %s", strURL.c_str());
      delete file;
    }
  }

  return MHD_NO;
}

int CWebServer::ContentReaderCallback(void *cls, uint64_t pos, char *buf, int max)
{
  CFile *file = (CFile *)cls;
  file->Seek(pos);
  return file->Read(buf, max);
}

void CWebServer::ContentReaderFreeCallback(void *cls)
{
  CFile *file = (CFile *)cls;
  file->Close();

  delete file;
}

bool CWebServer::Start(const char *ip, int port)
{
  if (!m_running)
  {
    // To stream perfectly we should probably have MHD_USE_THREAD_PER_CONNECTION instead of MHD_USE_SELECT_INTERNALLY as it provides multiple clients concurrently
    m_daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY | MHD_USE_SSL, port, NULL, NULL, &CWebServer::answer_to_connection, this, MHD_OPTION_END);
    m_running = m_daemon != NULL;
    CLog::Log(LOGNOTICE, "WebServer: Started the webserver");
  }
  return m_running;
}

bool CWebServer::Stop()
{
  if (m_running)
  {
    MHD_stop_daemon(m_daemon);
    m_running = false;
    CLog::Log(LOGNOTICE, "WebServer: Stopped the webserver");
  }

  return !m_running;
}

bool CWebServer::IsStarted()
{
  return m_running;
}

void CWebServer::StringToBase64(const char *input, CStdString &output)
{
  const char *lookup = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  unsigned long l;
  size_t length = strlen (input);

/*  tmp = (char *)malloc (length * 2);
  if (NULL == tmp)
    return tmp;

  tmp[0] = 0;*/
  output = "";

  for (int i = 0; i < length; i += 3)
  {
    l = (((unsigned long) input[i]) << 16)
      | (((i + 1) < length) ? (((unsigned long) input[i + 1]) << 8) : 0)
      | (((i + 2) < length) ? ((unsigned long) input[i + 2]) : 0);


    output.push_back(lookup[(l >> 18) & 0x3F]);
    output.push_back(lookup[(l >> 12) & 0x3F]);

    if (i + 1 < length)
      output.push_back(lookup[(l >> 6) & 0x3F]);
    if (i + 2 < length)
      output.push_back(lookup[l & 0x3F]);
  }

  int left = 3 - (length % 3);

  if (length % 3)
  {
    for (int i = 0; i < left; i++)
      output.push_back('=');
  }
}

void CWebServer::SetCredentials(const CStdString &username, const CStdString &password)
{
  CSingleLock lock (m_critSection);
  CStdString str = username + ":" + password;

  StringToBase64(str.c_str(), m_Credentials64Encoded);
  m_needcredentials = !password.IsEmpty();
}

bool CWebServer::Download(const char *path, Json::Value &result)
{
  bool exists = false;
  CFile *file = new CFile();
  if (file->Open(path))
  {
    exists = true;
    file->Close();
  }

  delete file;

  if (exists)
  {
    string str = "vfs/";
    str += path;
    result["path"] = str;
  }

  return exists;
}

int CWebServer::GetCapabilities()
{
  return Response | FileDownload;
}

int CWebServer::CHTTPClient::GetPermissionFlags()
{
  return OPERATION_PERMISSION_ALL;
}

int CWebServer::CHTTPClient::GetAnnouncementFlags()
{
  // Does not support broadcast
  return 0;
}

bool CWebServer::CHTTPClient::SetAnnouncementFlags(int flags)
{
  return false;
}
