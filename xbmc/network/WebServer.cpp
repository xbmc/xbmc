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
#include "filesystem/File.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/Base64.h"
#include "threads/SingleLock.h"
#include "XBDateTime.h"

#ifdef _WIN32
#pragma comment(lib, "libmicrohttpd.dll.lib")
#endif

#define PAGE_FILE_NOT_FOUND "<html><head><title>File not found</title></head><body>File not found</body></html>"
#define NOT_SUPPORTED       "<html><head><title>Not Supported</title></head><body>The method you are trying to use is not supported by this server</body></html>"

using namespace XFILE;
using namespace std;
using namespace JSONRPC;

vector<IHTTPRequestHandler *> CWebServer::m_requestHandlers;

CWebServer::CWebServer()
{
  m_running = false;
  m_daemon = NULL;
  m_needcredentials = true;
  m_Credentials64Encoded = "eGJtYzp4Ym1j"; // xbmc:xbmc
}

int CWebServer::FillArgumentMap(void *cls, enum MHD_ValueKind kind, const char *key, const char *value) 
{
  map<string, string> *arguments = (map<string, string> *)cls;
  arguments->insert(pair<string,string>(key,value));
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

  return (server->m_Credentials64Encoded.compare(headervalue + strlen(strbase)) == 0);
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
  HTTPMethod methodType = GetMethod(method);
  
  if (!IsAuthenticated(server, connection)) 
    return AskForAuthentication(connection);

  for (vector<IHTTPRequestHandler *>::const_iterator it = m_requestHandlers.begin(); it != m_requestHandlers.end(); it++)
  {
    IHTTPRequestHandler *handler = *it;
    if (handler->CheckHTTPRequest(connection, url, methodType, version))
    {
      int ret = handler->HandleHTTPRequest(server, connection, url, methodType, version, upload_data, upload_data_size, con_cls);
      if (ret != MHD_YES)
        return SendErrorResponse(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, methodType);

      struct MHD_Response *response = NULL;
      switch (handler->GetHTTPResponseType())
      {
        case HTTPNone:
          return MHD_YES;

        case HTTPRedirect:
          ret = CreateRedirect(connection, handler->GetHTTPRedirectUrl(), response);
          break;

        case HTTPFileDownload:
          ret = CreateFileDownloadResponse(connection, handler->GetHTTPResponseFile(), methodType, response);
          break;

        case HTTPMemoryDownloadNoFreeNoCopy:
          ret = CreateMemoryDownloadResponse(connection, handler->GetHTTPResponseData(), handler->GetHTTPResonseDataLength(), false, false, response);
          break;

        case HTTPMemoryDownloadNoFreeCopy:
          ret = CreateMemoryDownloadResponse(connection, handler->GetHTTPResponseData(), handler->GetHTTPResonseDataLength(), false, true, response);
          break;

        case HTTPMemoryDownloadFreeNoCopy:
          ret = CreateMemoryDownloadResponse(connection, handler->GetHTTPResponseData(), handler->GetHTTPResonseDataLength(), true, false, response);
          break;

        case HTTPMemoryDownloadFreeCopy:
          ret = CreateMemoryDownloadResponse(connection, handler->GetHTTPResponseData(), handler->GetHTTPResonseDataLength(), true, true, response);
          break;

        case HTTPError:
          ret = CreateErrorResponse(connection, handler->GetHTTPResonseCode(), methodType, response);
          break;

        default:
          return SendErrorResponse(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, methodType);
          break;
      }

      if (ret == MHD_NO)
        return SendErrorResponse(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, methodType);

      map<string, string> header = handler->GetHTTPResponseHeaderFields();
      for (map<string, string>::const_iterator it = header.begin(); it != header.end(); it++)
        MHD_add_response_header(response, it->first.c_str(), it->second.c_str());

      ret = MHD_queue_response(connection, handler->GetHTTPResonseCode(), response);
      MHD_destroy_response(response);

      return ret;
    }
  }

  return SendErrorResponse(connection, MHD_HTTP_NOT_FOUND, methodType);
}

HTTPMethod CWebServer::GetMethod(const char *method)
{
  if (strcmp(method, "GET") == 0)
    return GET;
  if (strcmp(method, "POST") == 0)
    return POST;
  if (strcmp(method, "HEAD") == 0)
    return HEAD;

  return UNKNOWN;
}

int CWebServer::CreateRedirect(struct MHD_Connection *connection, const string &strURL, struct MHD_Response *&response)
{
  response = MHD_create_response_from_data (0, NULL, MHD_NO, MHD_NO);
  if (response)
  {
    MHD_add_response_header(response, "Location", strURL.c_str());
    return MHD_YES;
  }
  return MHD_NO;
}

int CWebServer::CreateFileDownloadResponse(struct MHD_Connection *connection, const string &strURL, HTTPMethod methodType, struct MHD_Response *&response)
{
  int ret = MHD_NO;
  CFile *file = new CFile();

  if (file->Open(strURL, READ_NO_CACHE))
  {
    if (methodType != HEAD)
    {
      response = MHD_create_response_from_callback ( file->GetLength(),
                                                     2048,
                                                     &CWebServer::ContentReaderCallback, file,
                                                     &CWebServer::ContentReaderFreeCallback);
      if (response == NULL)
        return MHD_NO;
    }
    else
    {
      CStdString contentLength;
      contentLength.Format("%I64d", file->GetLength());
      file->Close();
      delete file;

      response = MHD_create_response_from_data (0, NULL, MHD_NO, MHD_NO);
      if (response == NULL)
        return MHD_NO;
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
  }
  else
  {
    delete file;
    CLog::Log(LOGERROR, "WebServer: Failed to open %s", strURL.c_str());
    return SendErrorResponse(connection, MHD_HTTP_NOT_FOUND, GET); /* GET Assumed Temporarily */
  }
  return MHD_YES;
}

int CWebServer::CreateErrorResponse(struct MHD_Connection *connection, int responseType, HTTPMethod method, struct MHD_Response *&response)
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

  response = MHD_create_response_from_data (payloadSize, payload, MHD_NO, MHD_NO);
  if (response)
    return MHD_YES;
  return MHD_NO;
}

int CWebServer::CreateMemoryDownloadResponse(struct MHD_Connection *connection, void *data, size_t size, bool free, bool copy, struct MHD_Response *&response)
{
  response = MHD_create_response_from_data (size, data, free ? MHD_YES : MHD_NO, copy ? MHD_YES : MHD_NO);
  if (response)
    return MHD_YES;
  return MHD_NO;
}

int CWebServer::SendErrorResponse(struct MHD_Connection *connection, int errorType, HTTPMethod method)
{
  struct MHD_Response *response = NULL;
  int ret = CreateErrorResponse(connection, errorType, method, response);
  if (ret == MHD_YES)
  {
    ret = MHD_queue_response (connection, errorType, response);
    MHD_destroy_response (response);
  }

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

bool CWebServer::Start(int port, const string &username, const string &password)
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

void CWebServer::SetCredentials(const string &username, const string &password)
{
  CSingleLock lock (m_critSection);
  CStdString str = username + ":" + password;

  Base64::Encode(str.c_str(), m_Credentials64Encoded);
  m_needcredentials = !password.empty();
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

void CWebServer::RegisterRequestHandler(IHTTPRequestHandler *handler)
{
  if (handler == NULL)
    return;

  for (vector<IHTTPRequestHandler *>::const_iterator it = m_requestHandlers.begin(); it != m_requestHandlers.end(); it++)
  {
    if (*it == handler)
      return;
  }

  m_requestHandlers.push_back(handler);
}

void CWebServer::UnregisterRequestHandler(IHTTPRequestHandler *handler)
{
  if (handler == NULL)
    return;

  for (vector<IHTTPRequestHandler *>::iterator it = m_requestHandlers.begin(); it != m_requestHandlers.end(); it++)
  {
    if (*it == handler)
    {
      m_requestHandlers.erase(it);
      return;
    }
  }
}

int CWebServer::GetRequestHeaderValues(struct MHD_Connection *connection, enum MHD_ValueKind kind, std::map<std::string, std::string> &headerValues)
{
  if (connection == NULL)
    return -1;

  return MHD_get_connection_values(connection, kind, FillArgumentMap, &headerValues);
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
#endif
