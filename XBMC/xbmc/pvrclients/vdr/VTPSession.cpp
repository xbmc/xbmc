#include "VTPSession.h"

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string>
#include <vector>

#if defined(_LINUX)
#include <unistd.h>
#elif defined(_WIN32)
typedef int socklen_t;
extern "C" int inet_pton(int af, const char *src, void *dst);
#endif

using namespace std;

//#define DEBUG

CVTPSession::CVTPSession()
  : m_socket(INVALID_SOCKET)
{}

CVTPSession::~CVTPSession()
{
  Close();
  Quit();
}

bool CVTPSession::OpenStreamSocket(SOCKET& sock, struct sockaddr_in& address2)
{
  struct sockaddr_in address(address2);

  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if(sock == INVALID_SOCKET)
  {
    /*Log::Log(LOG_ERROR, "CVTPSession::OpenStreamSocket - invalid socket");*/
    return false;
  }

  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port        = 0;

  if(bind(sock, (struct sockaddr*) &address, sizeof(address)) == SOCKET_ERROR)
  {
    /*Log::Log(LOG_ERROR, "CVTPSession::OpenStreamSocket - bind failed");*/
    return false;
  }

  socklen_t len = sizeof(address);
  if(getsockname(sock, (struct sockaddr*) &address, &len) == SOCKET_ERROR)
  {
    /*Log::Log(LOG_ERROR, "CVTPSession::OpenStreamSocket - bind failed");*/
    return false;
  }

  if(listen(sock, 1) == SOCKET_ERROR)
  {
    /*Log::Log(LOG_ERROR, "CVTPSession::OpenStreamSocket - listen failed");*/
    return false;
  }

  address2.sin_port = address.sin_port;

  /*Log::Log(LOG_DEBUG, "CVTPSession::OpenStreamSocket - listening on %s:%d", inet_ntoa(address.sin_addr), address.sin_port);*/
  return true;
}

bool CVTPSession::AcceptStreamSocket(SOCKET& sock2)
{
  SOCKET sock;
  sock = accept(sock2, NULL, NULL);
  if(sock == INVALID_SOCKET)
  {
    /*Log::Log(LOG_ERROR, "CVTPStream::Accept - failed to accept incomming connection");*/
    return false;
  }

  closesocket(sock2);
  sock2 = sock;
  return true;
}

void CVTPSession::Close()
{
  if(m_socket != INVALID_SOCKET)
    closesocket(m_socket);
}

bool CVTPSession::Open(const string &host, int port)
{
  m_socket = socket(AF_INET, SOCK_STREAM, 0);

  if(m_socket == INVALID_SOCKET)
    return false;

  struct sockaddr_in sa;
  struct hostent    *hp;
  int s;

  if ((hp=gethostbyname(host.c_str())) == NULL)
  {
    //log Failed to resolve hostname
    Close();
    return false;
  }

  memset(&sa, 0, sizeof(sa));
  memcpy(&sa.sin_addr, hp->h_addr_list[0], hp->h_length);

  sa.sin_family = hp->h_addrtype;
  sa.sin_port = htons((u_short)port);
  /*sa.sin_port = port;*/

  if ((s = (int)socket(hp->h_addrtype, SOCK_STREAM, 0)) < 0)
  {
    Close();
    return false;
  }

  if (connect(s, (struct sockaddr *)&sa, sizeof(sa)) < 0)
  {
    //log failed to connect to server
    Close();
    return false;
  }
    

  // VTP Server will send a greeting
  string line;
  int    code;
  ReadResponse(code, line);

  /*CLog::Log(LOGERROR, "CVTPSession::Open - server greeting: %s", line.c_str());*/

  return true;
}

bool CVTPSession::IsOpen()
{
  if(m_socket == INVALID_SOCKET)
    return false;
  else
    return true;
}

bool CVTPSession::ReadResponse(int &code, string &line)
{
  vector<string> lines;
  if(ReadResponse(code, lines))
  {
    line = lines[lines.size()-1];
    return true;
  }
  return false;
}

bool CVTPSession::ReadResponse(int &code, vector<string> &lines)
{
  fd_set         set_r, set_e;
  timeval        timeout;
  int            result;
  int            retries = 5;
  char           buffer[256];
  char           cont = 0;
  string         line;
  size_t         pos1 = 0, pos2 = 0, pos3 = 0;

  while(true)
  {
    while( (pos1 = line.find("\r\n", pos3)) != std::string::npos)
    {
      if(sscanf(line.c_str(), "%d%c", &code, &cont) != 2)
      {
        /*Log::Log(LOG_DEBUG, "CVTPSession::ReadResponse - unknown line format: %s", line.c_str());*/
        line.erase(0, pos1 + 2);
        continue;
      }

      pos2 = line.find(cont);

      lines.push_back(line.substr(pos2+1, pos1-pos2-1));

      line.erase(0, pos1 + 2);
      pos3 = 0;
    }

    // we only need to recheck 1 byte
    if(line.size() > 0)
      pos3 = line.size() - 1;
    else
      pos3 = 0;

    if(cont == ' ')
      break;

    //TODO set 10 seconds timeout value
    timeout.tv_sec  = 2;
    timeout.tv_usec = 0;

    // fill with new data
    FD_ZERO(&set_r);
    FD_ZERO(&set_e);
    FD_SET(m_socket, &set_r);
    FD_SET(m_socket, &set_e);
    result = select(FD_SETSIZE, &set_r, NULL, &set_e, &timeout);
    if(result < 0)
    {
      /*Log::Log(LOG_DEBUG, "CVTPSession::ReadResponse - select failed");*/
      m_socket = INVALID_SOCKET;
      return false;
    }

    if(result == 0)
    {
      /*Log::Log(LOG_DEBUG, "CVTPSession::ReadResponse - timeout waiting for response, retrying...");*/
      if (retries != 0) {
          retries--;
      continue;
    }
      else {
          m_socket = INVALID_SOCKET;
          return false;
      }
    }

    result = recv(m_socket, buffer, sizeof(buffer), 0);
    if(result < 0)
    {
      /*Log::Log(LOG_DEBUG, "CVTPSession::ReadResponse - recv failed");*/
      m_socket = INVALID_SOCKET;
      return false;
    }
    buffer[result] = 0;

    line.append(buffer);
  }

#ifdef DEBUG
  Log::Log(LOG_DEBUG, "CVTPSession::ReadResponse - Response code %d", code);
  for(unsigned i=0; i<lines.size(); i++)
    Log::Log(LOG_DEBUG, "CVTPSession::ReadResponse - Line %d: %s", i, lines[i].c_str());
#endif

  return true;
}

bool CVTPSession::SendCommand(const string &command)
{
  fd_set set_w, set_e;
  struct timeval tv;
  int  result;
  char buffer[1024];
  int  len;

  len = sprintf(buffer, "%s\r\n", command.c_str());
#ifdef DEBUG
  Log::Log(LOG_ERROR, "CVTPSession::SendCommand - sending '%s'", command.c_str());
#endif
  // fill with new data
  tv.tv_sec  = 0;
  tv.tv_usec = 0;

  FD_ZERO(&set_w);
  FD_ZERO(&set_e);
  FD_SET(m_socket, &set_w);
  FD_SET(m_socket, &set_e);
  result = select(FD_SETSIZE, &set_w, NULL, &set_e, &tv);
  if(result < 0)
  {
    /*Log::Log(LOG_ERROR, "CVTPSession::SendCommand - select failed");*/
    m_socket = INVALID_SOCKET;
    return false;
  }
  if (FD_ISSET(m_socket, &set_w))
  {
    /*Log::Log(LOG_ERROR, "CVTPSession::SendCommand - failed to send data");*/
    m_socket = INVALID_SOCKET;
    return false;
  }
  if(send(m_socket, buffer, len, 0) != len)
  {
    /*Log::Log(LOG_ERROR, "CVTPSession::SendCommand - failed to send data");*/
    m_socket = INVALID_SOCKET;
    return false;
  }
  return true;
}

bool CVTPSession::SendCommand(const string &command, int &code, string line)
{
  vector<string> lines;
  if(SendCommand(command, code, lines))
  {
    line = lines[lines.size()-1];
    return true;
  }
  return false;
}

bool CVTPSession::SendCommand(const string &command, int &code, vector<string> &lines)
{
  if(!SendCommand(command))
    return false;

  if(!ReadResponse(code, lines))
    return false;

  if(code < 200 || code > 299)
  {
    //Log::Log(LOG_ERROR, "CVTPSession::SendCommand - Failed with code: %d (%s)", code, lines[lines.size()-1].c_str());
    return false;
  }

  return true;
}

bool CVTPSession::GetChannels(vector<Channel> &channels)
{
  vector<string> lines;
  int            code;

  if(!SendCommand("LSTC", code, lines))
    return false;

  for(vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
  {
    string& data(*it);
    size_t space, colon;
    struct Channel channel;

    space = data.find(" ");
    if(space == string::npos)
    {
      //Log::Log(LOG_ERROR, "CVTPSession::GetChannels - failed to parse line %s", it->c_str());
      continue;    
    }

    colon = data.find(":", space+1);
    if(colon == string::npos)
    {
      //Log::Log(LOG_ERROR, "CVTPSession::GetChannels - failed to parse line %s", it->c_str());
      continue;
    }

    channel.index = atoi(data.substr(0, space).c_str());
    channel.name  = data.substr(space+1, colon-space-1);

    colon = channel.name.find(";");
    if(colon != string::npos)
    {
      channel.network = channel.name.substr(colon+1);
      channel.name.erase(colon);
    }

    channels.push_back(channel);

#ifdef DEBUG
    Log::Log(LOG_DEBUG, "CVTPSession::GetChannels - Channel:%d, Name: '%s', Network: '%s'", channel.index, channel.name.c_str(), channel.network.c_str());
#endif
  }
  return true;
}

SOCKET CVTPSession::GetStreamLive(int channel)
{

  sockaddr_in address;
  SOCKET      sock;
  socklen_t   len = sizeof(address);
  char        buffer[1024];
  string      result;
  int         code;

  if(!SendCommand("CAPS TS", code, result))
  {
    //Log::Log(LOG_ERROR, "CVTPSession::GetStreamLive - server is unable to provide mpeg-ts");
    return INVALID_SOCKET;
  }

  sprintf(buffer, "PROV %d %d", 100, channel);
  if(!SendCommand(buffer, code, result))
  {
    //Log::Log(LOG_ERROR, "CVTPSession::GetStreamLive - server is unable to provide channel");
    return INVALID_SOCKET;
  }

  sprintf(buffer, "TUNE %d", channel);
  if(!SendCommand(buffer, code, result))
  {
    //Log::Log(LOG_ERROR, "CVTPSession::GetStreamLive - server is unable to tune to said channel");
    return INVALID_SOCKET;
  }

  if(getsockname(m_socket, (struct sockaddr*) &address, &len) == SOCKET_ERROR)
  {
    //Log::Log(LOG_ERROR, "CVTPSession::GetStreamLive - getsockname failed");
    return INVALID_SOCKET;
  }

  //Log::Log(LOG_DEBUG, "CVTPSession::GetStreamLive - local address %s:%d", inet_ntoa(address.sin_addr), ntohs(address.sin_port) );

  if(!OpenStreamSocket(sock, address))
    return INVALID_SOCKET;

  int port = ntohs(address.sin_port);
  int addr = ntohl(address.sin_addr.s_addr);

  sprintf(buffer, "PORT 0 %d,%d,%d,%d,%d,%d"
                , (addr & 0xFF000000)>>24
                , (addr & 0x00FF0000)>>16
                , (addr & 0x0000FF00)>>8
                , (addr & 0x000000FF)>>0
                , (port & 0xFF00)>>8
                , (port & 0x00FF)>>0);

  if(!SendCommand(buffer, code, result))
    return NULL;

  if(!AcceptStreamSocket(sock))
  {
    closesocket(sock);
    return INVALID_SOCKET;
  }
  return sock;
}

SOCKET CVTPSession::GetStreamRecording(int recording, uint64_t *size, uint32_t *frames) {

    sockaddr_in address;
    SOCKET      sock;
    socklen_t   len = sizeof(address);
    char        buffer[1024];
    string      result;
    int         code;
    vector<string> lines;

    sprintf(buffer, "PLAY %d", recording);
    if (!SendCommand(buffer, code, lines)) {
        return INVALID_SOCKET;
    }

    vector<string>::iterator it = lines.begin();
    string& data(*it);

    sscanf(data.c_str(), "%I64u", size);
    data.erase(0,data.find(" ", 0)+1);
    *frames = atol(data.c_str());

    if(getsockname(m_socket, (struct sockaddr*) &address, &len) == SOCKET_ERROR) {
        //Log::Log(LOG_ERROR, "CVTPSession::GetStreamRecording - getsockname failed");
        return INVALID_SOCKET;
    }

    //Log::Log(LOG_DEBUG, "CVTPSession::GetStreamRecording - local address %s:%d", inet_ntoa(address.sin_addr), ntohs(address.sin_port) );

    if(!OpenStreamSocket(sock, address))
        return INVALID_SOCKET;

    int port = ntohs(address.sin_port);
    int addr = ntohl(address.sin_addr.s_addr);

    sprintf(buffer, "PORT 1 %d,%d,%d,%d,%d,%d"
                  , (addr & 0xFF000000)>>24
                  , (addr & 0x00FF0000)>>16
                  , (addr & 0x0000FF00)>>8
                  , (addr & 0x000000FF)>>0
                  , (port & 0xFF00)>>8
                  , (port & 0x00FF)>>0);

    if(!SendCommand(buffer, code, result)) {
        return INVALID_SOCKET;
    }

    if(!AcceptStreamSocket(sock)) {
        closesocket(sock);
        return INVALID_SOCKET;
    }
    return sock;
}

void CVTPSession::AbortStreamLive()
{
  if(m_socket == INVALID_SOCKET)
    return;

  string line;
  int    code;
  if(!SendCommand("ABRT 0", code, line))
    return;
    //Log::Log(LOG_ERROR, "CVTPSession::AbortStreamLive - failed");
}

void CVTPSession::AbortStreamRecording()
{
  if(m_socket == INVALID_SOCKET)
    return;

  string line;
  int    code;
  if(!SendCommand("ABRT 1", code, line))
    return;
    //Log::Log(LOG_ERROR, "CVTPSession::AbortStreamRecording - failed");
}

bool CVTPSession::CanStreamLive(int channel)
{
  if(m_socket == INVALID_SOCKET)
    return false;

  char   buffer[1024];
  string line;
  int    code;

  sprintf(buffer, "PROV %d %d", -1, channel);
  if(!SendCommand(buffer, code, line))
  {
    //Log::Log(LOG_ERROR, "CVTPSession::CanStreamLive - server is unable to provide channel %d", channel);
    return false;
  }
  return true;
}

bool CVTPSession::SuspendServer() {

    vector<string> lines;
    int            code;
    bool           ret;

    ret = SendCommand("SUSP", code, lines);

    if (!ret || code != 220)
    {
        //Log::Log(LOG_ERROR, "CVTPSession::SuspendServer: Couldn't suspend server");
        return false;
    }
    return true;
}

bool CVTPSession::Quit() {

    vector<string> lines;
    int            code;
    bool           ret;

    ret = SendCommand("QUIT", code, lines);

    if (!ret || code != 221)
    {
        //Log::Log(LOG_ERROR, "CVTPSession::Quit: Couldn't quit command connection");
        return false;
    }
    return true;
}
