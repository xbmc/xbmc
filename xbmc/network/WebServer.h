#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#ifdef HAS_WEB_SERVER
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include "interfaces/json-rpc/ITransportLayer.h"
#include "threads/CriticalSection.h"
#include "httprequesthandler/IHTTPRequestHandler.h"

class CWebServer : public JSONRPC::ITransportLayer
{
public:
  CWebServer();
  virtual ~CWebServer() { }

  bool Start(int port, const std::string &username, const std::string &password);
  bool Stop();
  bool IsStarted();
  void SetCredentials(const std::string &username, const std::string &password);

  virtual bool PrepareDownload(const char *path, CVariant &details, std::string &protocol);
  virtual bool Download(const char *path, CVariant &result);
  virtual int GetCapabilities();

  static void RegisterRequestHandler(IHTTPRequestHandler *handler);
  static void UnregisterRequestHandler(IHTTPRequestHandler *handler);

  static std::string GetRequestHeaderValue(struct MHD_Connection *connection, enum MHD_ValueKind kind, const std::string &key);
  static int GetRequestHeaderValues(struct MHD_Connection *connection, enum MHD_ValueKind kind, std::map<std::string, std::string> &headerValues);
  static int GetRequestHeaderValues(struct MHD_Connection *connection, enum MHD_ValueKind kind, std::multimap<std::string, std::string> &headerValues);
private:
  struct MHD_Daemon* StartMHD(unsigned int flags, int port);
  static int AskForAuthentication (struct MHD_Connection *connection);
  static bool IsAuthenticated (CWebServer *server, struct MHD_Connection *connection);

  static void* UriRequestLogger(void *cls, const char *uri);

#if (MHD_VERSION >= 0x00090200)
  static ssize_t ContentReaderCallback (void *cls, uint64_t pos, char *buf, size_t max);
#elif (MHD_VERSION >= 0x00040001)
  static int ContentReaderCallback (void *cls, uint64_t pos, char *buf, int max);
#else
  static int ContentReaderCallback (void *cls, size_t pos, char *buf, int max);
#endif

#if (MHD_VERSION >= 0x00040001)
  static int AnswerToConnection (void *cls, struct MHD_Connection *connection,
                        const char *url, const char *method,
                        const char *version, const char *upload_data,
                        size_t *upload_data_size, void **con_cls);
  static int HandlePostField(void *cls, enum MHD_ValueKind kind, const char *key,
                             const char *filename, const char *content_type,
                             const char *transfer_encoding, const char *data, uint64_t off,
                             size_t size);
#else   //libmicrohttpd < 0.4.0
  static int AnswerToConnection (void *cls, struct MHD_Connection *connection,
                        const char *url, const char *method,
                        const char *version, const char *upload_data,
                        unsigned int *upload_data_size, void **con_cls);
  static int HandlePostField(void *cls, enum MHD_ValueKind kind, const char *key,
                             const char *filename, const char *content_type,
                             const char *transfer_encoding, const char *data, uint64_t off,
                             unsigned int size);
#endif
  static int HandleRequest(IHTTPRequestHandler *handler, const HTTPRequest &request);
  static void ContentReaderFreeCallback (void *cls);
  static int CreateRedirect(struct MHD_Connection *connection, const std::string &strURL, struct MHD_Response *&response);
  static int CreateFileDownloadResponse(struct MHD_Connection *connection, const std::string &strURL, HTTPMethod methodType, struct MHD_Response *&response, int &responseCode);
  static int CreateErrorResponse(struct MHD_Connection *connection, int responseType, HTTPMethod method, struct MHD_Response *&response);
  static int CreateMemoryDownloadResponse(struct MHD_Connection *connection, void *data, size_t size, bool free, bool copy, struct MHD_Response *&response);

  static int SendErrorResponse(struct MHD_Connection *connection, int errorType, HTTPMethod method);
  
  static HTTPMethod GetMethod(const char *method);
  static int FillArgumentMap(void *cls, enum MHD_ValueKind kind, const char *key, const char *value);
  static int FillArgumentMultiMap(void *cls, enum MHD_ValueKind kind, const char *key, const char *value);

  static const char *CreateMimeTypeFromExtension(const char *ext);

  struct MHD_Daemon *m_daemon;
  bool m_running, m_needcredentials;
  std::string m_Credentials64Encoded;
  CCriticalSection m_critSection;
  static std::vector<IHTTPRequestHandler *> m_requestHandlers;

  typedef struct ConnectionHandler
  {
    IHTTPRequestHandler *requestHandler;
    struct MHD_PostProcessor *postprocessor;
  } ConnectionHandler;
};
#endif
