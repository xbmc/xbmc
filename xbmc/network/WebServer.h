#pragma once
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

#include "system.h"
#ifdef HAS_WEB_SERVER
#include "utils/StdString.h"
#include <sys/types.h>
#ifndef _WIN32
#include <sys/select.h>
#include <sys/socket.h>
#else
#include <WS2tcpip.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#ifdef __APPLE__
#include "lib/libmicrohttpd/src/include/microhttpd.h"
#else
#include <microhttpd.h>
#endif
#include "interfaces/json-rpc/ITransportLayer.h"
#include "threads/CriticalSection.h"

class CWebServer : public JSONRPC::ITransportLayer
{
public:
  CWebServer();

  bool Start(int port, const CStdString &username, const CStdString &password);
  bool Stop();
  bool IsStarted();
  void SetCredentials(const CStdString &username, const CStdString &password);
  virtual bool Download(const char *path, Json::Value *result);
  virtual int GetCapabilities();
private:
  enum HTTPMethod
  {
    UNKNOWN,
    POST,
    GET,
    HEAD
  };
  struct MHD_Daemon* StartMHD(unsigned int flags, int port);
  static int AskForAuthentication (struct MHD_Connection *connection);
  static bool IsAuthenticated (CWebServer *server, struct MHD_Connection *connection);

#if (MHD_VERSION >= 0x00090200)
  static ssize_t ContentReaderCallback (void *cls, uint64_t pos, char *buf, size_t max);
#elif (MHD_VERSION >= 0x00040001)
  static int ContentReaderCallback (void *cls, uint64_t pos, char *buf, int max);
#else
  static int ContentReaderCallback (void *cls, size_t pos, char *buf, int max);
#endif

#if (MHD_VERSION >= 0x00040001)
  static int JSONRPC(CWebServer *server, void **con_cls, struct MHD_Connection *connection, const char *upload_data, size_t *upload_data_size);
  static int AnswerToConnection (void *cls, struct MHD_Connection *connection,
                        const char *url, const char *method,
                        const char *version, const char *upload_data,
                        size_t *upload_data_size, void **con_cls);
#else   //libmicrohttpd < 0.4.0
  static int JSONRPC(CWebServer *server, void **con_cls, struct MHD_Connection *connection, const char *upload_data, unsigned int *upload_data_size);
  static int AnswerToConnection (void *cls, struct MHD_Connection *connection,
                        const char *url, const char *method,
                        const char *version, const char *upload_data,
                        unsigned int *upload_data_size, void **con_cls);
#endif
  static void ContentReaderFreeCallback (void *cls);
  static int HttpApi(struct MHD_Connection *connection);
  static HTTPMethod GetMethod(const char *method);
  static int CreateRedirect(struct MHD_Connection *connection, const CStdString &strURL);
  static int CreateFileDownloadResponse(struct MHD_Connection *connection, const CStdString &strURL);
  static int CreateErrorResponse(struct MHD_Connection *connection, int responseType, HTTPMethod method);
  static int CreateMemoryDownloadResponse(struct MHD_Connection *connection, void *data, size_t size);
  static int CreateAddonsListResponse(struct MHD_Connection *connection);

  static int FillArgumentMap(void *cls, enum MHD_ValueKind kind, const char *key, const char *value);
  static void StringToBase64(const char *input, CStdString &output);

  static const char *CreateMimeTypeFromExtension(const char *ext);

  struct MHD_Daemon *m_daemon;
  bool m_running, m_needcredentials;
  CStdString m_Credentials64Encoded;
  CCriticalSection m_critSection;

  class CHTTPClient : public JSONRPC::IClient
  {
  public:
    virtual int  GetPermissionFlags();
    virtual int  GetAnnouncementFlags();
    virtual bool SetAnnouncementFlags(int flags);
  };
};
#endif
