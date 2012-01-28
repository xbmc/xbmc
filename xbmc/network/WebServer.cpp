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
#include "interfaces/http-api/HttpApi.h"
#include "interfaces/json-rpc/JSONRPC.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "URL.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "threads/SingleLock.h"
#include "XBDateTime.h"
#include "addons/AddonManager.h"

#ifdef _WIN32
#pragma comment(lib, "libmicrohttpd.dll.lib")
#endif

#define MAX_STRING_POST_SIZE 20000
#define PAGE_FILE_NOT_FOUND "<html><head><title>File not found</title></head><body>File not found</body></html>"
#define PAGE_JSONRPC_INFO   "<html><head><title>JSONRPC</title></head><body>JSONRPC active and working</body></html>"
#define NOT_SUPPORTED       "<html><head><title>Not Supported</title></head><body>The method you are trying to use is not supported by this server</body></html>"
#define DEFAULT_PAGE        "index.html"

using namespace ADDON;
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

int CWebServer::AskForAuthentication(struct MHD_Connection *connection)
{
  int ret;
  struct MHD_Response *response;

  response = MHD_create_response_from_data (0, NULL, MHD_NO, MHD_NO);
  if (!response)
    return MHD_NO;

  ret = MHD_add_response_header(response, MHD_HTTP_HEADER_WWW_AUTHENTICATE, "Basic realm=XBMC");
  ret |= MHD_add_response_header(response, MHD_HTTP_HEADER_CONNECTION, "close");
  if (!ret)
  {
    MHD_destroy_response (response);
    return MHD_NO;
  }

  ret = MHD_queue_response (connection, MHD_HTTP_UNAUTHORIZED, response);

  MHD_destroy_response (response);

  return ret;
}

bool CWebServer::IsAuthenticated(CWebServer *server, struct MHD_Connection *connection)
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
  CStdString strURL = url;
  CStdString originalURL = url;
  HTTPMethod methodType = GetMethod(method);
  
  if (!IsAuthenticated(server, connection)) 
    return AskForAuthentication(connection);

//  if (methodType != GET && methodType != POST) /* Only GET and POST supported, catch other method types here to avoid continual checking later on */
//    return CreateErrorResponse(connection, MHD_HTTP_NOT_IMPLEMENTED, methodType);

#ifdef HAS_JSONRPC
  if (strURL.Equals("/jsonrpc"))
  {
    if (methodType == POST)
      return JSONRPC(server, con_cls, connection, upload_data, upload_data_size);
    else
      return CreateMemoryDownloadResponse(connection, (void *)PAGE_JSONRPC_INFO, strlen(PAGE_JSONRPC_INFO));
  }
#endif

#ifdef HAS_HTTPAPI
  if ((methodType == GET || methodType == POST) && strURL.Left(18).Equals("/xbmcCmds/xbmcHttp"))
    return HttpApi(connection);
#endif

  if (strURL.Left(4).Equals("/vfs"))
  {
    strURL = strURL.Right(strURL.length() - 5);
    CURL::Decode(strURL);
    return CreateFileDownloadResponse(connection, strURL, methodType);
  }

#ifdef HAS_WEB_INTERFACE
  AddonPtr addon;
  CStdString addonPath;
  bool useDefaultWebInterface = true;
  if (strURL.Left(8).Equals("/addons/") || (strURL == "/addons"))
  {
    CStdStringArray components;
    CUtil::Tokenize(strURL,components,"/");
    if (components.size() > 1)
    {
      CAddonMgr::Get().GetAddon(components.at(1),addon);
      if (addon)
      {
        size_t pos;
        pos = strURL.find('/', 8); // /addons/ = 8 characters +1 to start behind the last slash
        if (pos != CStdString::npos)
          strURL = strURL.substr(pos);
        else // missing trailing slash
          return CreateRedirect(connection, originalURL += "/");

        useDefaultWebInterface = false;
        addonPath = addon->Path();
        if (addon->Type() != ADDON_WEB_INTERFACE) // No need to append /htdocs for web interfaces
          addonPath = URIUtils::AddFileToFolder(addonPath, "/htdocs/");
      }
    }
    else
    {
      if (strURL.length() < 8) // missing trailing slash
        return CreateRedirect(connection, originalURL += "/");
      else
        return CreateAddonsListResponse(connection);
    }
  }

  if (strURL.Equals("/"))
    strURL.Format("/%s", DEFAULT_PAGE);

  if (useDefaultWebInterface)
  {
    CAddonMgr::Get().GetDefault(ADDON_WEB_INTERFACE,addon);
    if (addon)
      addonPath = addon->Path();
  }

  if (addon)
    strURL = URIUtils::AddFileToFolder(addon->Path(),strURL);
  if (CDirectory::Exists(strURL))
  {
    if (strURL.Right(1).Equals("/"))
      strURL += DEFAULT_PAGE;
    else
      return CreateRedirect(connection, originalURL += "/");
  }
  return CreateFileDownloadResponse(connection, strURL, methodType);

#endif

  return MHD_NO;
}

CWebServer::HTTPMethod CWebServer::GetMethod(const char *method)
{
  if (strcmp(method, "GET") == 0)
    return GET;
  if (strcmp(method, "POST") == 0)
    return POST;
  if (strcmp(method, "HEAD") == 0)
    return HEAD;

  return UNKNOWN;
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

int CWebServer::CreateRedirect(struct MHD_Connection *connection, const CStdString &strURL)
{
  struct MHD_Response *response = MHD_create_response_from_data (0, NULL, MHD_NO, MHD_NO);
  int ret = MHD_queue_response (connection, MHD_HTTP_FOUND, response);
  MHD_add_response_header(response, "Location", strURL);
  MHD_destroy_response (response);
  return ret;
}

int CWebServer::CreateFileDownloadResponse(struct MHD_Connection *connection, const CStdString &strURL, HTTPMethod methodType)
{
  int ret = MHD_NO;
  CFile *file = new CFile();

  if (file->Open(strURL, READ_NO_CACHE))
  {
    struct MHD_Response *response;
    if (methodType != HEAD)
    {
      response = MHD_create_response_from_callback ( file->GetLength(),
                                                     2048,
                                                     &CWebServer::ContentReaderCallback, file,
                                                     &CWebServer::ContentReaderFreeCallback); 
    } else {
      CStdString contentLength;
      contentLength.Format("%I64d", file->GetLength());
      file->Close();
      delete file;

      response = MHD_create_response_from_data (0, NULL, MHD_NO, MHD_NO);
      MHD_add_response_header(response, "Content-Length", contentLength);
    }

    CStdString ext = URIUtils::GetExtension(strURL);
    ext = ext.ToLower();
    const char *mime = CreateMimeTypeFromExtension(ext.c_str());
    if (mime)
      MHD_add_response_header(response, "Content-Type", mime);

    CDateTime expiryTime = CDateTime::GetCurrentDateTime();
    expiryTime += CDateTimeSpan(1, 0, 0, 0);
    MHD_add_response_header(response, "Expires", expiryTime.GetAsRFC1123DateTime());

    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);

    MHD_destroy_response(response);
  }
  else
  {
    delete file;
    CLog::Log(LOGERROR, "WebServer: Failed to open %s", strURL.c_str());
    return CreateErrorResponse(connection, MHD_HTTP_NOT_FOUND, GET); /* GET Assumed Temporarily */
  }
  return ret;
}

int CWebServer::CreateErrorResponse(struct MHD_Connection *connection, int responseType, HTTPMethod method)
{
  int ret = MHD_NO;
  size_t payloadSize = 0;
  void *payload = NULL;

  if (method != HEAD)
  {
    switch (responseType)
    {
      case MHD_HTTP_NOT_FOUND:
        payloadSize = strlen(PAGE_FILE_NOT_FOUND);
        payload = (void *)PAGE_FILE_NOT_FOUND;
        break;
      case MHD_HTTP_NOT_IMPLEMENTED:
        payloadSize = strlen(NOT_SUPPORTED);
        payload = (void *)NOT_SUPPORTED;
        break;
    }
  }

  struct MHD_Response *response = MHD_create_response_from_data (payloadSize, payload, MHD_NO, MHD_NO);
  ret = MHD_queue_response (connection, MHD_HTTP_NOT_FOUND, response);
  MHD_destroy_response (response);
  return ret;
}

int CWebServer::CreateMemoryDownloadResponse(struct MHD_Connection *connection, void *data, size_t size)
{
  struct MHD_Response *response = MHD_create_response_from_data (size, data, MHD_NO, MHD_NO);
  int ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);
  return ret;
}

int CWebServer::CreateAddonsListResponse(struct MHD_Connection *connection)
{
  CStdString responseData = "<html><head><title>Add-on List</title></head><body>\n<h1>Available web interfaces:</h1>\n<ul>\n";
  VECADDONS addons;
  CAddonMgr::Get().GetAddons(ADDON_WEB_INTERFACE, addons);
  IVECADDONS addons_it;
  for (addons_it=addons.begin(); addons_it!=addons.end(); addons_it++)
    responseData += "<li><a href=/addons/"+ (*addons_it)->ID() + "/>" + (*addons_it)->Name() + "</a></li>\n";

  responseData += "</ul>\n</body></html>";

  struct MHD_Response *response = MHD_create_response_from_data (responseData.length(), (void *)responseData.c_str(), MHD_NO, MHD_YES);
  if (!response)
    return MHD_NO;

  int ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);
  return ret;
}

#if (MHD_VERSION >= 0x00090200)
ssize_t CWebServer::ContentReaderCallback (void *cls, uint64_t pos, char *buf, size_t max)
#elif (MHD_VERSION >= 0x00040001)
int CWebServer::ContentReaderCallback(void *cls, uint64_t pos, char *buf, int max)
#else   //libmicrohttpd < 0.4.0
int CWebServer::ContentReaderCallback(void *cls, size_t pos, char *buf, int max)
#endif
{
  CFile *file = (CFile *)cls;
  if((unsigned int)pos != file->GetPosition())
    file->Seek(pos);
  unsigned res = file->Read(buf, max);
  if(res == 0)
    return -1;
  return res;
}

void CWebServer::ContentReaderFreeCallback(void *cls)
{
  CFile *file = (CFile *)cls;
  file->Close();

  delete file;
}

struct MHD_Daemon* CWebServer::StartMHD(unsigned int flags, int port)
{
  // WARNING: when using MHD_USE_THREAD_PER_CONNECTION, set MHD_OPTION_CONNECTION_TIMEOUT to something higher than 1
  // otherwise on libmicrohttpd 0.4.4-1 it spins a busy loop

  unsigned int timeout = 60 * 60 * 24;
  // MHD_USE_THREAD_PER_CONNECTION = one thread per connection
  // MHD_USE_SELECT_INTERNALLY = use main thread for each connection, can only handle one request at a time [unless you set the thread pool size]

  return MHD_start_daemon(flags,
                          port,
                          NULL,
                          NULL,
                          &CWebServer::AnswerToConnection,
                          this,
#if (MHD_VERSION >= 0x00040002)
                          MHD_OPTION_THREAD_POOL_SIZE, 1,
#endif
                          MHD_OPTION_CONNECTION_LIMIT, 512,
                          MHD_OPTION_CONNECTION_TIMEOUT, timeout,
                          MHD_OPTION_END);
}

bool CWebServer::Start(int port, const CStdString &username, const CStdString &password)
{
  SetCredentials(username, password);
  if (!m_running)
  {
    m_daemon = StartMHD(MHD_USE_SELECT_INTERNALLY, port);

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
  } else 
    CLog::Log(LOGNOTICE, "WebServer: Stopped failed because its not running");

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

bool CWebServer::PrepareDownload(const char *path, CVariant &details, std::string &protocol)
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
    protocol = "http";
    string url = "vfs/";
    CStdString strPath = path;
    CURL::Encode(strPath);
    url += strPath;
    details["path"] = url;
  }

  return exists;
}

bool CWebServer::Download(const char *path, CVariant &result)
{
  return false;
}

int CWebServer::GetCapabilities()
{
  return Response | FileDownloadRedirect;
}

const char *CWebServer::CreateMimeTypeFromExtension(const char *ext)
{
  if (strcmp(ext, ".aif") == 0)   return "audio/aiff";
  if (strcmp(ext, ".aiff") == 0)  return "audio/aiff";
  if (strcmp(ext, ".asf") == 0)   return "video/x-ms-asf";
  if (strcmp(ext, ".asx") == 0)   return "video/x-ms-asf";
  if (strcmp(ext, ".avi") == 0)   return "video/avi";
  if (strcmp(ext, ".avs") == 0)   return "video/avs-video";
  if (strcmp(ext, ".bin") == 0)   return "application/octet-stream";
  if (strcmp(ext, ".bmp") == 0)   return "image/bmp";
  if (strcmp(ext, ".dv") == 0)    return "video/x-dv";
  if (strcmp(ext, ".fli") == 0)   return "video/fli";
  if (strcmp(ext, ".gif") == 0)   return "image/gif";
  if (strcmp(ext, ".htm") == 0)   return "text/html";
  if (strcmp(ext, ".html") == 0)  return "text/html";
  if (strcmp(ext, ".htmls") == 0) return "text/html";
  if (strcmp(ext, ".ico") == 0)   return "image/x-icon";
  if (strcmp(ext, ".it") == 0)    return "audio/it";
  if (strcmp(ext, ".jpeg") == 0)  return "image/jpeg";
  if (strcmp(ext, ".jpg") == 0)   return "image/jpeg";
  if (strcmp(ext, ".json") == 0)  return "application/json";
  if (strcmp(ext, ".kar") == 0)   return "audio/midi";
  if (strcmp(ext, ".list") == 0)  return "text/plain";
  if (strcmp(ext, ".log") == 0)   return "text/plain";
  if (strcmp(ext, ".lst") == 0)   return "text/plain";
  if (strcmp(ext, ".m2v") == 0)   return "video/mpeg";
  if (strcmp(ext, ".m3u") == 0)   return "audio/x-mpequrl";
  if (strcmp(ext, ".mid") == 0)   return "audio/midi";
  if (strcmp(ext, ".midi") == 0)  return "audio/midi";
  if (strcmp(ext, ".mod") == 0)   return "audio/mod";
  if (strcmp(ext, ".mov") == 0)   return "video/quicktime";
  if (strcmp(ext, ".mp2") == 0)   return "audio/mpeg";
  if (strcmp(ext, ".mp3") == 0)   return "audio/mpeg3";
  if (strcmp(ext, ".mpa") == 0)   return "audio/mpeg";
  if (strcmp(ext, ".mpeg") == 0)  return "video/mpeg";
  if (strcmp(ext, ".mpg") == 0)   return "video/mpeg";
  if (strcmp(ext, ".mpga") == 0)  return "audio/mpeg";
  if (strcmp(ext, ".pcx") == 0)   return "image/x-pcx";
  if (strcmp(ext, ".png") == 0)   return "image/png";
  if (strcmp(ext, ".rm") == 0)    return "audio/x-pn-realaudio";
  if (strcmp(ext, ".s3m") == 0)   return "audio/s3m";
  if (strcmp(ext, ".sid") == 0)   return "audio/x-psid";
  if (strcmp(ext, ".tif") == 0)   return "image/tiff";
  if (strcmp(ext, ".tiff") == 0)  return "image/tiff";
  if (strcmp(ext, ".txt") == 0)   return "text/plain";
  if (strcmp(ext, ".uni") == 0)   return "text/uri-list";
  if (strcmp(ext, ".viv") == 0)   return "video/vivo";
  if (strcmp(ext, ".wav") == 0)   return "audio/wav";
  if (strcmp(ext, ".xm") == 0)    return "audio/xm";
  if (strcmp(ext, ".xml") == 0)   return "text/xml";
  if (strcmp(ext, ".zip") == 0)   return "application/zip";
  if (strcmp(ext, ".tbn") == 0)   return "image/jpeg";
  if (strcmp(ext, ".js") == 0)    return "application/javascript";
  if (strcmp(ext, ".css") == 0)   return "text/css";
  return NULL;
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
