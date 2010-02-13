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
#include "StdString.h"
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <microhttpd.h>
#include "../lib/libjsonrpc/ITransportLayer.h"
#include "CriticalSection.h"

class CWebServer : public JSONRPC::ITransportLayer
{
public:
  CWebServer();

  bool Start(const char *ip, int port);
  bool Stop();
  bool IsStarted();
  void SetCredentials(const CStdString &username, const CStdString &password);
  virtual bool Download(const char *path, Json::Value *result);
  virtual int GetCapabilities();
private:
  static int AskForAuthentication (struct MHD_Connection *connection);
  static bool IsAuthenticated (CWebServer *server, struct MHD_Connection *connection);
  static int AnswerToConnection (void *cls, struct MHD_Connection *connection,
                        const char *url, const char *method,
                        const char *version, const char *upload_data,
                        size_t *upload_data_size, void **con_cls);

  static int ContentReaderCallback (void *cls, uint64_t pos, char *buf, int max);
  static void ContentReaderFreeCallback (void *cls);
  static int JSONRPC(CWebServer *server, struct MHD_Connection *connection, const char *upload_data, size_t *upload_data_size);
  static int HttpApi(struct MHD_Connection *connection);

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
