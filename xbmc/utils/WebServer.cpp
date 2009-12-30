#include "system.h"
#include "WebServer.h"
#include "../lib/libjsonrpc/JSONRPC.h"
#include "../lib/libhttpapi/HttpApi.h"
#include "../FileSystem/File.h"
#include "../Util.h"
#include "log.h"

using namespace XFILE;
using namespace std;

CWebServer::CWebServer()
{
  m_running = false;
  m_daemon = NULL;
}

int CWebServer::answer_to_connection (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{
  printf("%s | %s\n", method, url);

  CStdString strURL = url;

  if (strcmp(method, "GET") == 0)
  {
#ifdef HAS_WEB_INTERFACE
    else if (strURL.Equals("/"))
      strURL = "special://home/web/index.html";
    else
      strURL.Format("special://home/web%s", strURL.c_str());
#endif
    CFile *file = new CFile();
    if (file->Open(strURL))
    {
      struct MHD_Response *response = MHD_create_response_from_callback ( file->GetLength(),
                                                                          2048,
                                                                          &CWebServer::ContentReaderCallback, file,
                                                                          &CWebServer::ContentReaderFreeCallback);
      int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
      MHD_destroy_response(response);
      return ret;
    }
    else
      delete file;
  }

  return MHD_NO;
}

int CWebServer::ContentReaderCallback(void *cls, uint64_t pos, char *buf, int max)
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

bool CWebServer::Initialize()
{
  CJSONRPC::Initialize();

  return true;
}

bool CWebServer::Start(const char *ip, int port)
{
  if (!m_running)
  {
    Initialize();

    // To stream perfectly we should probably have MHD_USE_THREAD_PER_CONNECTION instead of MHD_USE_SELECT_INTERNALLY as it provides multiple clients concurrently
    m_daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY | MHD_USE_SSL, port, NULL, NULL, &CWebServer::answer_to_connection, NULL, MHD_OPTION_END);
    m_running = m_daemon != NULL;
  }
  return m_running;
}

bool CWebServer::Stop()
{
  if (m_running)
  {
    MHD_stop_daemon(m_daemon);
    m_running = false;
  }

  return !m_running;
}
