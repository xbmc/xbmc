#pragma once
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

#include "system.h"

#ifdef HAS_WEB_SERVER
#include <memory>
#include <vector>

#include "network/httprequesthandler/IHTTPRequestHandler.h"
#include "threads/CriticalSection.h"

namespace XFILE
{
  class CFile;
}
class CDateTime;
class CVariant;

class CWebServer
{
public:
  CWebServer();
  virtual ~CWebServer() { }

  bool Start(uint16_t port, const std::string &username, const std::string &password);
  bool Stop();
  bool IsStarted();
  void SetCredentials(const std::string &username, const std::string &password);

  void RegisterRequestHandler(IHTTPRequestHandler *handler);
  void UnregisterRequestHandler(IHTTPRequestHandler *handler);

protected:
  typedef struct ConnectionHandler
  {
    std::string fullUri;
    bool isNew;
    std::shared_ptr<IHTTPRequestHandler> requestHandler;
    struct MHD_PostProcessor *postprocessor;
    int errorStatus;

    ConnectionHandler(const std::string& uri)
      : fullUri(uri)
      , isNew(true)
      , requestHandler(nullptr)
      , postprocessor(nullptr)
      , errorStatus(MHD_HTTP_OK)
    { }
  } ConnectionHandler;

  virtual void LogRequest(const char* uri) const;

  virtual int HandlePartialRequest(struct MHD_Connection *connection, ConnectionHandler* connectionHandler, HTTPRequest request,
                                   const char *upload_data, size_t *upload_data_size, void **con_cls);
  virtual int HandleRequest(const std::shared_ptr<IHTTPRequestHandler>& handler);
  virtual int FinalizeRequest(const std::shared_ptr<IHTTPRequestHandler>& handler, int responseStatus, struct MHD_Response *response);

private:
  struct MHD_Daemon* StartMHD(unsigned int flags, int port);

  int AskForAuthentication(struct MHD_Connection *connection) const;
  bool IsAuthenticated(struct MHD_Connection *connection) const;

  int CreateMemoryDownloadResponse(const std::shared_ptr<IHTTPRequestHandler>& handler, struct MHD_Response *&response) const;
  int CreateRangedMemoryDownloadResponse(const std::shared_ptr<IHTTPRequestHandler>& handler, struct MHD_Response *&response) const;

  int CreateRedirect(struct MHD_Connection *connection, const std::string &strURL, struct MHD_Response *&response) const;
  int CreateFileDownloadResponse(const std::shared_ptr<IHTTPRequestHandler>& handler, struct MHD_Response *&response) const;
  int CreateErrorResponse(struct MHD_Connection *connection, int responseType, HTTPMethod method, struct MHD_Response *&response) const;
  int CreateMemoryDownloadResponse(struct MHD_Connection *connection, const void *data, size_t size, bool free, bool copy, struct MHD_Response *&response) const;

  int SendErrorResponse(struct MHD_Connection *connection, int errorType, HTTPMethod method) const;

  int AddHeader(struct MHD_Response *response, const std::string &name, const std::string &value) const;

  static std::string CreateMimeTypeFromExtension(const char *ext);

  // MHD callback implementations
  static void* UriRequestLogger(void *cls, const char *uri);

#if (MHD_VERSION >= 0x00090200)
  static ssize_t ContentReaderCallback (void *cls, uint64_t pos, char *buf, size_t max);
#elif (MHD_VERSION >= 0x00040001)
  static int ContentReaderCallback (void *cls, uint64_t pos, char *buf, int max);
#else
  static int ContentReaderCallback (void *cls, size_t pos, char *buf, int max);
#endif
  static void ContentReaderFreeCallback(void *cls);

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

  uint16_t m_port;
  struct MHD_Daemon *m_daemon_ip6;
  struct MHD_Daemon *m_daemon_ip4;
  bool m_running;
  bool m_needcredentials;
  size_t m_thread_stacksize;
  std::string m_Credentials64Encoded;
  CCriticalSection m_critSection;
  std::vector<IHTTPRequestHandler *> m_requestHandlers;
};
#endif
