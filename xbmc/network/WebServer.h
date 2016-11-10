/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "network/httprequesthandler/IHTTPRequestHandler.h"
#include "threads/CriticalSection.h"
#include "utils/logtypes.h"

#include <memory>
#include <vector>

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
  virtual ~CWebServer() = default;

  bool Start(uint16_t port, const std::string &username, const std::string &password);
  bool Stop();
  bool IsStarted();
  static bool WebServerSupportsSSL();
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

    explicit ConnectionHandler(const std::string& uri)
      : fullUri(uri)
      , isNew(true)
      , requestHandler(nullptr)
      , postprocessor(nullptr)
      , errorStatus(MHD_HTTP_OK)
    { }
  } ConnectionHandler;

  virtual void LogRequest(const char* uri) const;

  virtual int HandlePartialRequest(struct MHD_Connection *connection, ConnectionHandler* connectionHandler, const HTTPRequest& request,
                                   const char *upload_data, size_t *upload_data_size, void **con_cls);
  virtual int HandleRequest(const std::shared_ptr<IHTTPRequestHandler>& handler);
  virtual int FinalizeRequest(const std::shared_ptr<IHTTPRequestHandler>& handler, int responseStatus, struct MHD_Response *response);

private:
  struct MHD_Daemon* StartMHD(unsigned int flags, int port);

  std::shared_ptr<IHTTPRequestHandler> FindRequestHandler(const HTTPRequest& request) const;

  int AskForAuthentication(const HTTPRequest& request) const;
  bool IsAuthenticated(const HTTPRequest& request) const;

  bool IsRequestCacheable(const HTTPRequest& request) const;
  bool IsRequestRanged(const HTTPRequest& request, const CDateTime &lastModified) const;

  void SetupPostDataProcessing(const HTTPRequest& request, ConnectionHandler *connectionHandler, std::shared_ptr<IHTTPRequestHandler> handler, void **con_cls) const;
  bool ProcessPostData(const HTTPRequest& request, ConnectionHandler *connectionHandler, const char *upload_data, size_t *upload_data_size, void **con_cls) const;
  void FinalizePostDataProcessing(ConnectionHandler *connectionHandler) const;

  int CreateMemoryDownloadResponse(const std::shared_ptr<IHTTPRequestHandler>& handler, struct MHD_Response *&response) const;
  int CreateRangedMemoryDownloadResponse(const std::shared_ptr<IHTTPRequestHandler>& handler, struct MHD_Response *&response) const;

  int CreateRedirect(struct MHD_Connection *connection, const std::string &strURL, struct MHD_Response *&response) const;
  int CreateFileDownloadResponse(const std::shared_ptr<IHTTPRequestHandler>& handler, struct MHD_Response *&response) const;
  int CreateErrorResponse(struct MHD_Connection *connection, int responseType, HTTPMethod method, struct MHD_Response *&response) const;
  int CreateMemoryDownloadResponse(struct MHD_Connection *connection, const void *data, size_t size, bool free, bool copy, struct MHD_Response *&response) const;

  int SendResponse(const HTTPRequest& request, int responseStatus, MHD_Response *response) const;
  int SendErrorResponse(const HTTPRequest& request, int errorType, HTTPMethod method) const;

  int AddHeader(struct MHD_Response *response, const std::string &name, const std::string &value) const;

  void LogRequest(const HTTPRequest& request) const;
  void LogResponse(const HTTPRequest& request, int responseStatus) const;

  static std::string CreateMimeTypeFromExtension(const char *ext);

  // MHD callback implementations
  static void* UriRequestLogger(void *cls, const char *uri);

  static ssize_t ContentReaderCallback (void *cls, uint64_t pos, char *buf, size_t max);
  static void ContentReaderFreeCallback(void *cls);

  static int AnswerToConnection (void *cls, struct MHD_Connection *connection,
                        const char *url, const char *method,
                        const char *version, const char *upload_data,
                        size_t *upload_data_size, void **con_cls);
  static int HandlePostField(void *cls, enum MHD_ValueKind kind, const char *key,
                             const char *filename, const char *content_type,
                             const char *transfer_encoding, const char *data, uint64_t off,
                             size_t size);

  bool LoadCert(std::string &skey, std::string &scert);

  uint16_t m_port = 0;
  struct MHD_Daemon *m_daemon_ip6 = nullptr;
  struct MHD_Daemon *m_daemon_ip4 = nullptr;
  bool m_running = false;
  size_t m_thread_stacksize = 0;
  bool m_authenticationRequired = false;
  std::string m_authenticationUsername;
  std::string m_authenticationPassword;
  std::string m_key;
  std::string m_cert;
  mutable CCriticalSection m_critSection;
  std::vector<IHTTPRequestHandler *> m_requestHandlers;

  Logger m_logger;
  static Logger s_logger;
};
