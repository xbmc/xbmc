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
#ifdef HAS_WEB_SERVER
#include "../lib/libjsonrpc/JSONRPC.h"
#include "../lib/libhttpapi/HttpApi.h"
#include "../FileSystem/File.h"
#include "../FileSystem/Directory.h"
#include "../Util.h"
#include "log.h"
#include "SingleLock.h"
#ifdef _WIN32
#pragma comment(lib, "../../lib/libmicrohttpd_win32/lib/libmicrohttpd.dll.lib")
#endif

#define MAX_STRING_POST_SIZE 20000
#define PAGE_FILE_NOT_FOUND "<html><head><title>File not found</title></head><body>File not found</body></html>"

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

#if (MHD_VERSION >= 0x00040001)
int CWebServer::AnswerToConnection(void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
#else
int CWebServer::AnswerToConnection(void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      unsigned int *upload_data_size, void **con_cls)
#endif
{
  CWebServer *server = (CWebServer *)cls;

  if (!IsAuthenticated(server, connection)) 
    return AskForAuthentication(connection); 

  CLog::Log(LOGNOTICE, "WebServer: %s | %s", method, url);

  CStdString strURL = url;

  if (strURL.Equals("/jsonrpc") && strcmp (method, "POST") == 0)
    return JSONRPC(server, con_cls, connection, upload_data, upload_data_size);

  if (strcmp(method, "GET") == 0)
  {
    if (strURL.Left(18).Equals("/xbmcCmds/xbmcHttp"))
      return HttpApi(connection);
    else if (strURL.Left(6).Equals("/thumb"))
    {
      strURL = strURL.Right(strURL.length() - 7);
      strURL = strURL.Left(strURL.length() - 4);
      return CreateDownloadResponse(connection, strURL);
    }
    else if (strURL.Left(4).Equals("/vfs"))
    {
      strURL = strURL.Right(strURL.length() - 5);
      CUtil::URLDecode(strURL);
      return CreateDownloadResponse(connection, strURL);
    }
#ifdef HAS_WEB_INTERFACE
    else if (strURL.Equals("/"))
      return CreateDownloadResponse(connection, "special://xbmc/web/index.html");
    else
    {
      strURL.Format("special://xbmc/web%s", strURL.c_str());
      if (CDirectory::Exists(strURL))
        strURL += "/index.html";
      return CreateDownloadResponse(connection, strURL);
    }
#endif
  }

  return MHD_NO;
}

#if (MHD_VERSION >= 0x00040001)
int CWebServer::JSONRPC(CWebServer *server, void **con_cls, struct MHD_Connection *connection, const char *upload_data, size_t *upload_data_size)
#else
int CWebServer::JSONRPC(CWebServer *server, void **con_cls, struct MHD_Connection *connection, const char *upload_data, unsigned int *upload_data_size)
#endif
{
#ifdef HAS_JSONRPC
  if ((*con_cls) == NULL)
  {
    *con_cls = new CStdString();

    return MHD_YES;
  }
  if (*upload_data_size) 
  {
    CStdString *post = (CStdString *)(*con_cls);
    if (*upload_data_size + post->size() > MAX_STRING_POST_SIZE)
    {
      CLog::Log(LOGERROR, "WebServer: Stopped uploading post since it exceeded size limitations");
      return MHD_NO;
    }
    else
    {
      post->append(upload_data, *upload_data_size);
      *upload_data_size = 0;
      return MHD_YES;
    }
  }
  else
  {
    CStdString *jsoncall = (CStdString *)(*con_cls);

    if (jsoncall->size() > 2000)
      CLog::Log(LOGINFO, "JSONRPC: Recieved a jsonrpc call wich is longer than 2000 characters, skipping logging it");
    else
      CLog::Log(LOGINFO, "JSONRPC: Recieved a jsonrpc call - %s", jsoncall->c_str());
    CHTTPClient client;
    CStdString jsonresponse = CJSONRPC::MethodCall(*jsoncall, server, &client);

    struct MHD_Response *response = MHD_create_response_from_data(jsonresponse.length(), (void *) jsonresponse.c_str(), MHD_NO, MHD_YES);
    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_add_response_header(response, "Content-Type", "application/json");
    MHD_destroy_response(response);

    delete jsoncall;
    return ret;
  }
#else
  return MHD_NO;
#endif
}

int CWebServer::HttpApi(struct MHD_Connection *connection)
{
#ifdef HAS_HTTPAPI
  map<CStdString, CStdString> arguments;
  if (MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, FillArgumentMap, &arguments) > 0)
  {
    CStdString httpapiresponse = CHttpApi::WebMethodCall(arguments["command"], arguments["parameter"]);

    struct MHD_Response *response = MHD_create_response_from_data(httpapiresponse.length(), (void *) httpapiresponse.c_str(), MHD_NO, MHD_YES);
    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
  }
#endif
  return MHD_NO;
}

int CWebServer::CreateDownloadResponse(struct MHD_Connection *connection, const CStdString &strURL)
{
  int ret = MHD_NO;
  CFile *file = new CFile();
  if (file->Open(strURL))
  {
    struct MHD_Response *response;
    if (file->GetLength() > 0)
    {
      response = MHD_create_response_from_callback ( file->GetLength(),
                                                     2048,
                                                     &CWebServer::ContentReaderCallback, file,
                                                     &CWebServer::ContentReaderFreeCallback);
    }
    else
    {
      //libmicrohttpd calls abort() when CWebServer::ContentReaderCallback return 0
      delete file;
      response = MHD_create_response_from_data(0, NULL, 0, 0);
    }

    CStdString ext = CUtil::GetExtension(strURL);
    ext = ext.ToLower();
    const char *mime = CreateMimeTypeFromExtension(ext.c_str());
    if (mime)
      MHD_add_response_header(response, "Content-Type", mime);

    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
  }
  else
  {
    delete file;
    CLog::Log(LOGERROR, "WebServer: Failed to open %s", strURL.c_str());
    struct MHD_Response *response = MHD_create_response_from_data (strlen (PAGE_FILE_NOT_FOUND),
                                              (void *) PAGE_FILE_NOT_FOUND,
                                              MHD_NO, MHD_NO);
    ret = MHD_queue_response (connection, MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response (response);
  }

  return ret;
}

#if (MHD_VERSION >= 0x00040001)
int CWebServer::ContentReaderCallback(void *cls, uint64_t pos, char *buf, int max)
#else   //libmicrohttpd < 0.4.0
int CWebServer::ContentReaderCallback(void *cls, size_t pos, char *buf, int max)
#endif
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
    m_daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY | MHD_USE_IPv6, port, NULL, NULL, &CWebServer::AnswerToConnection, this, MHD_OPTION_END);
    if (!m_daemon) //try IPv4
      m_daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, port, NULL, this, &CWebServer::AnswerToConnection, this, MHD_OPTION_END);
    m_running = m_daemon != NULL;
    if (m_running)
      CLog::Log(LOGNOTICE, "WebServer: Started the webserver");
    else
      CLog::Log(LOGERROR, "WebServer: Failed to start the webserver");
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
  output = "";

  for (unsigned int i = 0; i < length; i += 3)
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

bool CWebServer::Download(const char *path, Json::Value *result)
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
    (*result)["path"] = str;
  }

  return exists;
}

int CWebServer::GetCapabilities()
{
  return Response | FileDownload;
}

const char *CWebServer::CreateMimeTypeFromExtension(const char *ext)
{
  if      (strcmp(ext, ".aif") == 0)   return "audio/aiff";
  else if (strcmp(ext, ".aiff") == 0)  return "audio/aiff";
  else if (strcmp(ext, ".asf") == 0)   return "video/x-ms-asf";
  else if (strcmp(ext, ".asx") == 0)   return "video/x-ms-asf";
  else if (strcmp(ext, ".avi") == 0)   return "video/avi";
  else if (strcmp(ext, ".avs") == 0)   return "video/avs-video";
  else if (strcmp(ext, ".bin") == 0)   return "application/octet-stream";
  else if (strcmp(ext, ".bmp") == 0)   return "image/bmp";
  else if (strcmp(ext, ".dv") == 0)    return "video/x-dv";
  else if (strcmp(ext, ".fli") == 0)   return "video/fli";
  else if (strcmp(ext, ".gif") == 0)   return "image/gif";
  else if (strcmp(ext, ".htm") == 0)   return "text/html";
  else if (strcmp(ext, ".html") == 0)  return "text/html";
  else if (strcmp(ext, ".htmls") == 0) return "text/html";
  else if (strcmp(ext, ".ico") == 0)   return "image/x-icon";
  else if (strcmp(ext, ".it") == 0)    return "audio/it";
  else if (strcmp(ext, ".jpeg") == 0)  return "image/jpeg";
  else if (strcmp(ext, ".jpg") == 0)   return "image/jpeg";
  else if (strcmp(ext, ".json") == 0)  return "application/json";
  else if (strcmp(ext, ".kar") == 0)   return "audio/midi";
  else if (strcmp(ext, ".list") == 0)  return "text/plain";
  else if (strcmp(ext, ".log") == 0)   return "text/plain";
  else if (strcmp(ext, ".lst") == 0)   return "text/plain";
  else if (strcmp(ext, ".m2v") == 0)   return "video/mpeg";
  else if (strcmp(ext, ".m3u") == 0)   return "audio/x-mpequrl";
  else if (strcmp(ext, ".mid") == 0)   return "audio/midi";
  else if (strcmp(ext, ".midi") == 0)  return "audio/midi";
  else if (strcmp(ext, ".mod") == 0)   return "audio/mod";
  else if (strcmp(ext, ".mov") == 0)   return "video/quicktime";
  else if (strcmp(ext, ".mp2") == 0)   return "audio/mpeg";
  else if (strcmp(ext, ".mp3") == 0)   return "audio/mpeg3";
  else if (strcmp(ext, ".mpa") == 0)   return "audio/mpeg";
  else if (strcmp(ext, ".mpeg") == 0)  return "video/mpeg";
  else if (strcmp(ext, ".mpg") == 0)   return "video/mpeg";
  else if (strcmp(ext, ".mpga") == 0)  return "audio/mpeg";
  else if (strcmp(ext, ".pcx") == 0)   return "image/x-pcx";
  else if (strcmp(ext, ".png") == 0)   return "image/png";
  else if (strcmp(ext, ".rm") == 0)    return "audio/x-pn-realaudio";
  else if (strcmp(ext, ".s3m") == 0)   return "audio/s3m";
  else if (strcmp(ext, ".sid") == 0)   return "audio/x-psid";
  else if (strcmp(ext, ".tif") == 0)   return "image/tiff";
  else if (strcmp(ext, ".tiff") == 0)  return "image/tiff";
  else if (strcmp(ext, ".txt") == 0)   return "text/plain";
  else if (strcmp(ext, ".uni") == 0)   return "text/uri-list";
  else if (strcmp(ext, ".viv") == 0)   return "video/vivo";
  else if (strcmp(ext, ".wav") == 0)   return "audio/wav";
  else if (strcmp(ext, ".xm") == 0)    return "audio/xm";
  else if (strcmp(ext, ".xml") == 0)   return "text/xml";
  else if (strcmp(ext, ".zip") == 0)   return "application/zip";
  else return NULL;
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
#endif
