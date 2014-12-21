/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif

#include "WebServer.h"
#ifdef HAS_WEB_SERVER
#include <memory>

#include "URL.h"
#include "Util.h"
#include "XBDateTime.h"
#include "filesystem/File.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/Base64.h"
#include "utils/log.h"
#include "utils/Mime.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

//#define WEBSERVER_DEBUG

#ifdef TARGET_WINDOWS
#pragma comment(lib, "libmicrohttpd.dll.lib")
#endif

#define MAX_POST_BUFFER_SIZE 2048

#define PAGE_FILE_NOT_FOUND "<html><head><title>File not found</title></head><body>File not found</body></html>"
#define NOT_SUPPORTED       "<html><head><title>Not Supported</title></head><body>The method you are trying to use is not supported by this server</body></html>"

#define CONTENT_RANGE_FORMAT  "bytes %" PRId64 "-%" PRId64 "/%" PRId64

using namespace XFILE;
using namespace std;
using namespace JSONRPC;

typedef struct {
  std::shared_ptr<CFile> file;
  HttpRanges ranges;
  size_t rangeCount;
  int64_t rangesLength;
  string boundary;
  string boundaryWithHeader;
  bool boundaryWritten;
  string contentType;
  int64_t writePosition;
} HttpFileDownloadContext;

vector<IHTTPRequestHandler *> CWebServer::m_requestHandlers;

CWebServer::CWebServer()
{
  m_running = false;
  m_daemon_ip6 = NULL;
  m_daemon_ip4 = NULL;
  m_needcredentials = true;
  m_Credentials64Encoded = "eGJtYzp4Ym1j"; // xbmc:xbmc
}

int CWebServer::FillArgumentMap(void *cls, enum MHD_ValueKind kind, const char *key, const char *value) 
{
  if (cls == NULL || key == NULL)
    return MHD_NO;

  map<string, string> *arguments = (map<string, string> *)cls;
  arguments->insert(pair<string, string>(key, value != NULL ? value : StringUtils::Empty));
  return MHD_YES; 
}

int CWebServer::FillArgumentMultiMap(void *cls, enum MHD_ValueKind kind, const char *key, const char *value) 
{
  if (cls == NULL || key == NULL)
    return MHD_NO;

  multimap<string, string> *arguments = (multimap<string, string> *)cls;
  arguments->insert(pair<string, string>(key, value != NULL ? value : StringUtils::Empty));
  return MHD_YES; 
}

int CWebServer::AskForAuthentication(struct MHD_Connection *connection)
{
  int ret;
  struct MHD_Response *response;

  response = MHD_create_response_from_data (0, NULL, MHD_NO, MHD_NO);
  if (!response)
    return MHD_NO;

  ret = AddHeader(response, MHD_HTTP_HEADER_WWW_AUTHENTICATE, "Basic realm=XBMC");
  ret |= AddHeader(response, MHD_HTTP_HEADER_CONNECTION, "close");
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
  HTTPRequest request = { connection, url, methodType, version, server };

  if (!IsAuthenticated(server, connection)) 
    return AskForAuthentication(connection);

  // Check if this is the first call to
  // AnswerToConnection for this request
  if (*con_cls == NULL)
  {
    // Look for a IHTTPRequestHandler which can
    // take care of the current request
    for (vector<IHTTPRequestHandler *>::const_iterator it = m_requestHandlers.begin(); it != m_requestHandlers.end(); it++)
    {
      IHTTPRequestHandler *requestHandler = *it;
      if (requestHandler->CheckHTTPRequest(request))
      {
        // We found a matching IHTTPRequestHandler
        // so let's get a new instance for this request
        IHTTPRequestHandler *handler = requestHandler->GetInstance();

        // If we got a POST request we need to take
        // care of the POST data
        if (methodType == POST)
        {
          ConnectionHandler *conHandler = new ConnectionHandler();
          conHandler->requestHandler = handler;

          // Get the content-type of the POST data
          string contentType = GetRequestHeaderValue(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_CONTENT_TYPE);
          if (!contentType.empty())
          {
            // If the content-type is application/x-ww-form-urlencoded or multipart/form-data
            // we can use MHD's POST processor
            if (stricmp(contentType.c_str(), MHD_HTTP_POST_ENCODING_FORM_URLENCODED) == 0 ||
                stricmp(contentType.c_str(), MHD_HTTP_POST_ENCODING_MULTIPART_FORMDATA) == 0)
            {
              // Get a new MHD_PostProcessor
              conHandler->postprocessor = MHD_create_post_processor(connection, MAX_POST_BUFFER_SIZE, &CWebServer::HandlePostField, (void*)conHandler);

              // MHD doesn't seem to be able to handle
              // this post request
              if (conHandler->postprocessor == NULL)
              {
                delete conHandler->requestHandler;
                delete conHandler;

                return SendErrorResponse(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, methodType);
              }
            }
          }
          // otherwise we need to handle the POST data ourselves
          // which is done in the next call to AnswerToConnection

          *con_cls = (void*)conHandler;
          return MHD_YES;
        }
        // No POST request so nothing special to handle
        else
          return HandleRequest(handler, request);
      }
    }
  }
  // This is a subsequent call to
  // AnswerToConnection for this request
  else
  {
    // Again we need to take special care
    // of the POST data
    if (methodType == POST)
    {
      ConnectionHandler *conHandler = (ConnectionHandler *)*con_cls;
      if (conHandler->requestHandler == NULL)
        return SendErrorResponse(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, methodType);

      // We only need to handle POST data
      // if there actually is data left to handle
      if (*upload_data_size > 0)
      {
        // Either use MHD's POST processor
        if (conHandler->postprocessor != NULL)
          MHD_post_process(conHandler->postprocessor, upload_data, *upload_data_size);
        // or simply copy the data to the handler
        else
          conHandler->requestHandler->AddPostData(upload_data, *upload_data_size);

        // Signal that we have handled the data
        *upload_data_size = 0;

        return MHD_YES;
      }
      // We have handled all POST data
      // so it's time to invoke the IHTTPRequestHandler
      else
      {
        if (conHandler->postprocessor != NULL)
          MHD_destroy_post_processor(conHandler->postprocessor);
        *con_cls = NULL;

        int ret = HandleRequest(conHandler->requestHandler, request);
        delete conHandler;
        return ret;
      }
    }
    // It's unusual to get more than one call
    // to AnswerToConnection for none-POST
    // requests, but let's handle it anyway
    else
    {
      for (vector<IHTTPRequestHandler *>::const_iterator it = m_requestHandlers.begin(); it != m_requestHandlers.end(); it++)
      {
        IHTTPRequestHandler *requestHandler = *it;
        if (requestHandler->CheckHTTPRequest(request))
          return HandleRequest(requestHandler->GetInstance(), request);
      }
    }
  }

  return SendErrorResponse(connection, MHD_HTTP_NOT_FOUND, methodType);
}

#if (MHD_VERSION >= 0x00040001)
int CWebServer::HandlePostField(void *cls, enum MHD_ValueKind kind, const char *key,
                                const char *filename, const char *content_type,
                                const char *transfer_encoding, const char *data, uint64_t off,
                                size_t size)
#else
int CWebServer::HandlePostField(void *cls, enum MHD_ValueKind kind, const char *key,
                                const char *filename, const char *content_type,
                                const char *transfer_encoding, const char *data, uint64_t off,
                                unsigned int size)
#endif
{
  ConnectionHandler *conHandler = (ConnectionHandler *)cls;

  if (conHandler == NULL || conHandler->requestHandler == NULL || size == 0)
    return MHD_NO;

  conHandler->requestHandler->AddPostField(key, string(data, size));
  return MHD_YES;
}

int CWebServer::HandleRequest(IHTTPRequestHandler *handler, const HTTPRequest &request)
{
  if (handler == NULL)
    return SendErrorResponse(request.connection, MHD_HTTP_INTERNAL_SERVER_ERROR, request.method);

  int ret = handler->HandleHTTPRequest(request);
  if (ret == MHD_NO)
  {
    delete handler;
    return SendErrorResponse(request.connection, MHD_HTTP_INTERNAL_SERVER_ERROR, request.method);
  }

  struct MHD_Response *response = NULL;
  int responseCode = handler->GetHTTPResonseCode();
  switch (handler->GetHTTPResponseType())
  {
    case HTTPNone:
      delete handler;
      return MHD_NO;

    case HTTPRedirect:
      ret = CreateRedirect(request.connection, handler->GetHTTPRedirectUrl(), response);
      break;

    case HTTPFileDownload:
      ret = CreateFileDownloadResponse(request.connection, handler->GetHTTPResponseFile(), request.method, response, responseCode);
      break;

    case HTTPMemoryDownloadNoFreeNoCopy:
      ret = CreateMemoryDownloadResponse(request.connection, handler->GetHTTPResponseData(), handler->GetHTTPResonseDataLength(), false, false, response);
      break;

    case HTTPMemoryDownloadNoFreeCopy:
      ret = CreateMemoryDownloadResponse(request.connection, handler->GetHTTPResponseData(), handler->GetHTTPResonseDataLength(), false, true, response);
      break;

    case HTTPMemoryDownloadFreeNoCopy:
      ret = CreateMemoryDownloadResponse(request.connection, handler->GetHTTPResponseData(), handler->GetHTTPResonseDataLength(), true, false, response);
      break;

    case HTTPMemoryDownloadFreeCopy:
      ret = CreateMemoryDownloadResponse(request.connection, handler->GetHTTPResponseData(), handler->GetHTTPResonseDataLength(), true, true, response);
      break;

    case HTTPError:
      ret = CreateErrorResponse(request.connection, handler->GetHTTPResonseCode(), request.method, response);
      break;

    default:
      delete handler;
      return SendErrorResponse(request.connection, MHD_HTTP_INTERNAL_SERVER_ERROR, request.method);
  }

  if (ret == MHD_NO)
  {
    delete handler;
    return SendErrorResponse(request.connection, MHD_HTTP_INTERNAL_SERVER_ERROR, request.method);
  }

  multimap<string, string> header = handler->GetHTTPResponseHeaderFields();
  for (multimap<string, string>::const_iterator it = header.begin(); it != header.end(); it++)
    AddHeader(response, it->first.c_str(), it->second.c_str());

  MHD_queue_response(request.connection, responseCode, response);
  MHD_destroy_response(response);
  delete handler;

  return MHD_YES;
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
    AddHeader(response, "Location", strURL.c_str());
    return MHD_YES;
  }
  return MHD_NO;
}

int CWebServer::CreateFileDownloadResponse(struct MHD_Connection *connection, const string &strURL, HTTPMethod methodType, struct MHD_Response *&response, int &responseCode)
{
  std::shared_ptr<CFile> file = std::make_shared<CFile>();

#ifdef WEBSERVER_DEBUG
  CLog::Log(LOGDEBUG, "webserver  [IN] %s", strURL.c_str());
  multimap<string, string> headers;
  if (GetRequestHeaderValues(connection, MHD_HEADER_KIND, headers) > 0)
  {
    for (multimap<string, string>::const_iterator header = headers.begin(); header != headers.end(); header++)
      CLog::Log(LOGDEBUG, "webserver  [IN] %s: %s", header->first.c_str(), header->second.c_str());
  }
#endif

  if (file->Open(strURL, READ_NO_CACHE))
  {
    bool getData = true;
    bool ranged = false;
    int64_t fileLength = file->GetLength();

    // try to get the file's last modified date
    CDateTime lastModified;
    if (!GetLastModifiedDateTime(file.get(), lastModified))
      lastModified.Reset();

    // get the MIME type for the Content-Type header
    std::string ext = URIUtils::GetExtension(strURL);
    StringUtils::ToLower(ext);
    string mimeType = CreateMimeTypeFromExtension(ext.c_str());

    if (methodType != HEAD)
    {
      int64_t firstPosition = 0;
      int64_t lastPosition = fileLength - 1;
      uint64_t totalLength = 0;
      std::auto_ptr<HttpFileDownloadContext> context(new HttpFileDownloadContext());
      context->file = file;
      context->rangesLength = fileLength;
      context->contentType = mimeType;
      context->boundaryWritten = false;
      context->writePosition = 0;

      if (methodType == GET)
      {
        // handle If-Modified-Since
        string ifModifiedSince = GetRequestHeaderValue(connection, MHD_HEADER_KIND, "If-Modified-Since");
        if (!ifModifiedSince.empty() && lastModified.IsValid())
        {
          CDateTime ifModifiedSinceDate;
          ifModifiedSinceDate.SetFromRFC1123DateTime(ifModifiedSince);

          if (lastModified.GetAsUTCDateTime() <= ifModifiedSinceDate)
          {
            getData = false;
            response = MHD_create_response_from_data(0, NULL, MHD_NO, MHD_NO);
            if (response == NULL)
              return MHD_NO;

            responseCode = MHD_HTTP_NOT_MODIFIED;
          }
        }

        if (getData)
        {
          // handle Range header
          context->rangesLength = ParseRangeHeader(GetRequestHeaderValue(connection, MHD_HEADER_KIND, "Range"), fileLength, context->ranges, firstPosition, lastPosition);

          // handle If-Range header but only if the Range header is present
          if (!context->ranges.empty())
          {
            string ifRange = GetRequestHeaderValue(connection, MHD_HEADER_KIND, "If-Range");
            if (!ifRange.empty() && lastModified.IsValid())
            {
              CDateTime ifRangeDate;
              ifRangeDate.SetFromRFC1123DateTime(ifRange);

              // check if the last modification is newer than the If-Range date
              // if so we have to server the whole file instead
              if (lastModified.GetAsUTCDateTime() > ifRangeDate)
                context->ranges.clear();
            }
          }
        }
      }

      if (getData)
      {
        // if there are no ranges, add the whole range
        if (context->ranges.empty() || context->rangesLength == fileLength)
        {
          if (context->rangesLength == fileLength)
            context->ranges.clear();

          context->ranges.push_back(HttpRange(0, fileLength - 1));
          context->rangesLength = fileLength;
          firstPosition = 0;
          lastPosition = fileLength - 1;
        }
        else
          responseCode = MHD_HTTP_PARTIAL_CONTENT;

        // remember the total number of ranges
        context->rangeCount = context->ranges.size();
        // remember the total length
        totalLength = context->rangesLength;

        // we need to remember whether we are ranged because the range length
        // might change and won't be reliable anymore for length comparisons
        ranged = context->rangeCount > 1 || context->rangesLength < fileLength;

        // adjust the MIME type and range length in case of multiple ranges
        // which requires multipart boundaries
        if (context->rangeCount > 1)
        {
          context->boundary = GenerateMultipartBoundary();
          mimeType = "multipart/byteranges; boundary=" + context->boundary;

          // build part of the boundary with the optional Content-Type header
          // "--<boundary>\r\nContent-Type: <content-type>\r\n
          context->boundaryWithHeader = "\r\n--" + context->boundary + "\r\n";
          if (!context->contentType.empty())
            context->boundaryWithHeader += "Content-Type: " + context->contentType + "\r\n";

          // for every range, we need to add a boundary with header
          for (HttpRanges::const_iterator range = context->ranges.begin(); range != context->ranges.end(); range++)
          {
            // we need to temporarily add the Content-Range header to the
            // boundary to be able to determine the length
            string completeBoundaryWithHeader = context->boundaryWithHeader;
            completeBoundaryWithHeader += StringUtils::Format("Content-Range: " CONTENT_RANGE_FORMAT,
                                                              range->first, range->second, range->second - range->first + 1);
            completeBoundaryWithHeader += "\r\n\r\n";

            totalLength += completeBoundaryWithHeader.size();
          }
          // and at the very end a special end-boundary "\r\n--<boundary>--"
          totalLength += 4 + context->boundary.size() + 2;
        }

        // set the initial write position
        context->writePosition = context->ranges.begin()->first;

        // create the response object
        response = MHD_create_response_from_callback(totalLength,
                                                     2048,
                                                     &CWebServer::ContentReaderCallback, context.get(),
                                                     &CWebServer::ContentReaderFreeCallback);
        if (response == NULL)
          return MHD_NO;
        
        context.release(); // ownership was passed to mhd
      }

      // add Content-Range header
      if (ranged)
        AddHeader(response, "Content-Range", StringUtils::Format(CONTENT_RANGE_FORMAT, firstPosition, lastPosition, fileLength).c_str());
    }
    else
    {
      getData = false;

      std::string contentLength = StringUtils::Format("%" PRId64, fileLength);

      response = MHD_create_response_from_data(0, NULL, MHD_NO, MHD_NO);
      if (response == NULL)
        return MHD_NO;

      AddHeader(response, "Content-Length", contentLength);
    }

    // add "Accept-Ranges: bytes" header
    AddHeader(response, "Accept-Ranges", "bytes");

    // set the Content-Type header
    if (!mimeType.empty())
      AddHeader(response, "Content-Type", mimeType.c_str());

    // set the Last-Modified header
    if (lastModified.IsValid())
      AddHeader(response, "Last-Modified", lastModified.GetAsRFC1123DateTime());

    // set the Expires header
    CDateTime expiryTime = CDateTime::GetCurrentDateTime();
    if (StringUtils::EqualsNoCase(mimeType, "text/html") ||
        StringUtils::EqualsNoCase(mimeType, "text/css") ||
        StringUtils::EqualsNoCase(mimeType, "application/javascript"))
      expiryTime += CDateTimeSpan(1, 0, 0, 0);
    else
      expiryTime += CDateTimeSpan(365, 0, 0, 0);
    AddHeader(response, "Expires", expiryTime.GetAsRFC1123DateTime());
  }
  else
  {
    CLog::Log(LOGERROR, "WebServer: Failed to open %s", strURL.c_str());
    return SendErrorResponse(connection, MHD_HTTP_NOT_FOUND, methodType);
  }

  return MHD_YES;
}

int CWebServer::CreateErrorResponse(struct MHD_Connection *connection, int responseType, HTTPMethod method, struct MHD_Response *&response)
{
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

void* CWebServer::UriRequestLogger(void *cls, const char *uri)
{
  CLog::Log(LOGDEBUG, "webserver: request received for %s", uri);
  return NULL;
}

#if (MHD_VERSION >= 0x00090200)
ssize_t CWebServer::ContentReaderCallback(void *cls, uint64_t pos, char *buf, size_t max)
#elif (MHD_VERSION >= 0x00040001)
int CWebServer::ContentReaderCallback(void *cls, uint64_t pos, char *buf, int max)
#else   //libmicrohttpd < 0.4.0
int CWebServer::ContentReaderCallback(void *cls, size_t pos, char *buf, int max)
#endif
{
  HttpFileDownloadContext *context = (HttpFileDownloadContext *)cls;
  if (context == NULL || context->file == NULL)
    return -1;

#ifdef WEBSERVER_DEBUG
  CLog::Log(LOGDEBUG, "webserver [OUT] write maximum %d bytes from %" PRIu64 " (%" PRIu64 ")", max, context->writePosition, pos);
#endif

  // check if we need to add the end-boundary
  if (context->rangeCount > 1 && context->ranges.empty())
  {
    // put together the end-boundary
    string endBoundary = "\r\n--" + context->boundary + "--";
    if ((unsigned int)max != endBoundary.size())
      return -1;

    // copy the boundary into the buffer
    memcpy(buf, endBoundary.c_str(), endBoundary.size());
    return endBoundary.size();
  }

  if (context->ranges.empty())
    return -1;

  int64_t start = context->ranges.at(0).first;
  int64_t end = context->ranges.at(0).second;
  int64_t maximum = (int64_t)max;
  int written = 0;

  if (context->rangeCount > 1 && !context->boundaryWritten)
  {
    // put together the boundary for the current range
    string boundary = context->boundaryWithHeader;
    boundary += StringUtils::Format("Content-Range: " CONTENT_RANGE_FORMAT, start, end, end - start + 1) + "\r\n\r\n";

    // copy the boundary into the buffer
    memcpy(buf, boundary.c_str(), boundary.size());
    // advance the buffer position
    buf += boundary.size();
    // update the number of written byte
    written += boundary.size();
    // update the maximum number of bytes
    maximum -= boundary.size();
    context->boundaryWritten = true;
  }

  // check if the current position is within this range
  // if not, set it to the start position
  if (context->writePosition < start || context->writePosition > end)
    context->writePosition = start;
  // adjust the maximum number of read bytes
  maximum = std::min(maximum, end - context->writePosition + 1);

  // seek to the position if necessary
  if(context->writePosition != context->file->GetPosition())
    context->file->Seek(context->writePosition);

  // read data from the file
  ssize_t res = context->file->Read(buf, maximum);
  if (res <= 0)
    return -1;

  // add the number of read bytes to the number of written bytes
  written += res;
#ifdef WEBSERVER_DEBUG
  CLog::Log(LOGDEBUG, "webserver [OUT] wrote %d bytes from %" PRId64 " in range (%" PRId64 " - %" PRId64 ")", written, context->writePosition, start, end);
#endif
  // update the current write position
  context->writePosition += res;

  // if we have read all the data from the current range
  // remove it from the list
  if (context->writePosition >= end + 1)
  {
    context->ranges.erase(context->ranges.begin());
    context->boundaryWritten = false;
  }

  return written;
}

void CWebServer::ContentReaderFreeCallback(void *cls)
{
  HttpFileDownloadContext *context = (HttpFileDownloadContext *)cls;
  delete context;

#ifdef WEBSERVER_DEBUG
  CLog::Log(LOGDEBUG, "webserver [OUT] done");
#endif
}

struct MHD_Daemon* CWebServer::StartMHD(unsigned int flags, int port)
{
  unsigned int timeout = 60 * 60 * 24;

  return MHD_start_daemon(flags |
#if (MHD_VERSION >= 0x00040002) && (MHD_VERSION < 0x00090B01)
                          // use main thread for each connection, can only handle one request at a
                          // time [unless you set the thread pool size]
                          MHD_USE_SELECT_INTERNALLY
#else
                          // one thread per connection
                          // WARNING: set MHD_OPTION_CONNECTION_TIMEOUT to something higher than 1
                          // otherwise on libmicrohttpd 0.4.4-1 it spins a busy loop
                          MHD_USE_THREAD_PER_CONNECTION
#endif
                          ,
                          port,
                          NULL,
                          NULL,
                          &CWebServer::AnswerToConnection,
                          this,

#if (MHD_VERSION >= 0x00040002) && (MHD_VERSION < 0x00090B01)
                          MHD_OPTION_THREAD_POOL_SIZE, 4,
#endif
                          MHD_OPTION_CONNECTION_LIMIT, 512,
                          MHD_OPTION_CONNECTION_TIMEOUT, timeout,
                          MHD_OPTION_URI_LOG_CALLBACK, &CWebServer::UriRequestLogger, this,
                          MHD_OPTION_END);
}

bool CWebServer::Start(int port, const string &username, const string &password)
{
  SetCredentials(username, password);
  if (!m_running)
  {
    int v6testSock;
    if ((v6testSock = socket(AF_INET6, SOCK_STREAM, 0)) >= 0)
    {
      closesocket(v6testSock);
      m_daemon_ip6 = StartMHD(MHD_USE_IPv6, port);
    }
    
    m_daemon_ip4 = StartMHD(0 , port);
    
    m_running = (m_daemon_ip6 != NULL) || (m_daemon_ip4 != NULL);
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
    if (m_daemon_ip6 != NULL)
      MHD_stop_daemon(m_daemon_ip6);

    if (m_daemon_ip4 != NULL)
      MHD_stop_daemon(m_daemon_ip4);
    
    m_running = false;
    CLog::Log(LOGNOTICE, "WebServer: Stopped the webserver");
  }
  else 
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
  std::string str = username + ':' + password;

  Base64::Encode(str.c_str(), m_Credentials64Encoded);
  m_needcredentials = !password.empty();
}

bool CWebServer::PrepareDownload(const char *path, CVariant &details, std::string &protocol)
{
  if (CFile::Exists(path))
  {
    protocol = "http";
    string url;
    std::string strPath = path;
    if (StringUtils::StartsWith(strPath, "image://") ||
       (StringUtils::StartsWith(strPath, "special://") && StringUtils::EndsWith(strPath, ".tbn")))
      url = "image/";
    else
      url = "vfs/";
    url += CURL::Encode(strPath);
    details["path"] = url;
    return true;
  }

  return false;
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

  for (vector<IHTTPRequestHandler *>::iterator it = m_requestHandlers.begin(); it != m_requestHandlers.end(); it++)
  {
    if (*it == handler)
      return;

    if ((*it)->GetPriority() < handler->GetPriority())
    {
      m_requestHandlers.insert(it, handler);
      return;
    }
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

std::string CWebServer::GetRequestHeaderValue(struct MHD_Connection *connection, enum MHD_ValueKind kind, const std::string &key)
{
  if (connection == NULL)
    return "";

  const char* value = MHD_lookup_connection_value(connection, kind, key.c_str());
  if (value == NULL)
    return "";

  if (stricmp(key.c_str(), MHD_HTTP_HEADER_CONTENT_TYPE) == 0)
  {
    // Work around a bug in firefox (see https://bugzilla.mozilla.org/show_bug.cgi?id=416178)
    // by cutting of anything that follows a ";" in a "Content-Type" header field
    string strValue(value);
    size_t pos = strValue.find(';');
    if (pos != string::npos)
      strValue = strValue.substr(0, pos);

    return strValue;
  }

  return value;
}

int CWebServer::GetRequestHeaderValues(struct MHD_Connection *connection, enum MHD_ValueKind kind, std::map<std::string, std::string> &headerValues)
{
  if (connection == NULL)
    return -1;

  return MHD_get_connection_values(connection, kind, FillArgumentMap, &headerValues);
}

int CWebServer::GetRequestHeaderValues(struct MHD_Connection *connection, enum MHD_ValueKind kind, std::multimap<std::string, std::string> &headerValues)
{
  if (connection == NULL)
    return -1;

  return MHD_get_connection_values(connection, kind, FillArgumentMultiMap, &headerValues);
}

std::string CWebServer::CreateMimeTypeFromExtension(const char *ext)
{
  if (strcmp(ext, ".kar") == 0)   return "audio/midi";
  if (strcmp(ext, ".tbn") == 0)   return "image/jpeg";
  return CMime::GetMimeType(ext);
}

int CWebServer::AddHeader(struct MHD_Response *response, const std::string &name, const std::string &value)
{
  if (response == NULL || name.empty())
    return 0;

#ifdef WEBSERVER_DEBUG
  CLog::Log(LOGDEBUG, "webserver [OUT] %s: %s", name.c_str(), value.c_str());
#endif
  return MHD_add_response_header(response, name.c_str(), value.c_str());
}

int64_t CWebServer::ParseRangeHeader(const std::string &rangeHeaderValue, int64_t totalLength, HttpRanges &ranges, int64_t &firstPosition, int64_t &lastPosition)
{
  firstPosition = 0;
  lastPosition = totalLength - 1;

  if (rangeHeaderValue.empty() || !StringUtils::StartsWithNoCase(rangeHeaderValue, "bytes="))
    return totalLength;

  int64_t rangesLength = 0;

  // remove "bytes=" from the beginning
  string rangesValue = rangeHeaderValue.substr(6);
  // split the value of the "Range" header by ","
  vector<string> rangeValues = StringUtils::Split(rangesValue, ",");
  for (vector<string>::const_iterator range = rangeValues.begin(); range != rangeValues.end(); range++)
  {
    // there must be a "-" in the range definition
    if (range->find("-") == string::npos)
    {
      ranges.clear();
      return totalLength;
    }

    vector<string> positions = StringUtils::Split(*range, "-");
    if (positions.size() > 2)
    {
      ranges.clear();
      return totalLength;
    }

    int64_t positionStart = -1;
    int64_t positionEnd = -1;
    if (!positions.at(0).empty())
      positionStart = str2int64(positions.at(0), -1);
    if (!positions.at(1).empty())
      positionEnd = str2int64(positions.at(1), -1);

    if (positionStart < 0 && positionEnd < 0)
    {
      ranges.clear();
      return totalLength;
    }

    // if there's no end position, use the file's length
    if (positionEnd < 0)
      positionEnd = totalLength - 1;
    else if (positionStart < 0)
    {
      positionStart = totalLength - positionEnd;
      positionEnd = totalLength - 1;
    }

    if (positionEnd < positionStart)
    {
      ranges.clear();
      return totalLength;
    }

    if (ranges.empty())
    {
      firstPosition = positionStart;
      lastPosition = positionEnd;
    }
    else
    {
      if (positionStart < firstPosition)
        firstPosition = positionStart;
      if (positionEnd > lastPosition)
        lastPosition = positionEnd;
    }

    ranges.push_back(HttpRange(positionStart, positionEnd));
    rangesLength += positionEnd - positionStart + 1;
  }

  if (!ranges.empty() || rangesLength > 0)
    return rangesLength;

  return totalLength;
}

std::string CWebServer::GenerateMultipartBoundary()
{
  static char chars[] = "-_1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

  // create a string of length 30 to 40 and pre-fill it with "-"
  size_t count = (size_t)CUtil::GetRandomNumber() % 11 + 30;
  string boundary(count, '-');

  for (size_t i = (size_t)CUtil::GetRandomNumber() % 5 + 8; i < count; i++)
    boundary.replace(i, 1, 1, chars[(size_t)CUtil::GetRandomNumber() % 64]);

  return boundary;
}

bool CWebServer::GetLastModifiedDateTime(XFILE::CFile *file, CDateTime &lastModified)
{
  if (file == NULL)
    return false;

  struct __stat64 statBuffer;
  if (file->Stat(&statBuffer) != 0)
    return false;

  struct tm *time;
#ifdef HAVE_LOCALTIME_R
  struct tm result = {};
  time = localtime_r((time_t*)&statBuffer.st_mtime, &result);
#else
  time = localtime((time_t *)&statBuffer.st_mtime);
#endif
  if (time == NULL)
    return false;

  lastModified = *time;
  return true;
}
#endif
