#pragma once

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <microhttpd.h>

class CWebServer
{
public:
  CWebServer();

  bool Start(const char *ip, int port);
  bool Stop();
private:
  static int answer_to_connection (void *cls, struct MHD_Connection *connection,
                        const char *url, const char *method,
                        const char *version, const char *upload_data,
                        size_t *upload_data_size, void **con_cls);

  static int ContentReaderCallback (void *cls, uint64_t pos, char *buf, int max);
  static void ContentReaderFreeCallback (void *cls);
  static bool Initialize();

  struct MHD_Daemon *m_daemon;
  bool m_running;
};


