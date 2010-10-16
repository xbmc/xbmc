/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "vtptransceiver.h"
#include "select.h"
#include "client.h"
#include "timers.h"
#include "channels.h"
#include "recordings.h"
#include "epg.h"
#include "client.h"

//#define DEBUG_VTP
#define MINLOGREPEAT    10 //don't log connect failures too often (seconds)
#define MAX_LINK_LEVEL  6

using namespace std;

CVTPTransceiver VTPTransceiver;

CVTPTransceiver::CVTPTransceiver()
  : m_VTPSocket(INVALID_SOCKET)
{
  memset(m_DataSockets, INVALID_SOCKET, sizeof(SOCKET) * si_Count);
}

CVTPTransceiver::~CVTPTransceiver()
{
  Reset();
  if (IsOpen()) Quit();
}

void CVTPTransceiver::Reset(void)
{
  for (int it = 0; it < si_Count; ++it)
  {
    if (m_DataSockets[it] != INVALID_SOCKET)
    {
      close(m_DataSockets[it]);
      m_DataSockets[it] = INVALID_SOCKET;
    }
  }
}

bool CVTPTransceiver::OpenStreamSocket(SOCKET& sock, struct sockaddr_in& address2)
{
  struct sockaddr_in address(address2);

  sock = __socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if(sock == INVALID_SOCKET)
  {
    XBMC->Log(LOG_ERROR, "CVTPTransceiver::OpenStreamSocket - invalid socket");
    return false;
  }

  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port        = 0;

  if(__bind(sock, (struct sockaddr*) &address, sizeof(address)) == SOCKET_ERROR)
  {
    XBMC->Log(LOG_ERROR, "CVTPTransceiver::OpenStreamSocket - bind failed");
    return false;
  }

  socklen_t len = sizeof(address);
  if(__getsockname(sock, (struct sockaddr*) &address, &len) == SOCKET_ERROR)
  {
    XBMC->Log(LOG_ERROR, "CVTPTransceiver::OpenStreamSocket - bind failed");
    return false;
  }

  if(__listen(sock, 1) == SOCKET_ERROR)
  {
    XBMC->Log(LOG_ERROR, "CVTPTransceiver::OpenStreamSocket - listen failed");
    return false;
  }

  address2.sin_port = address.sin_port;

  XBMC->Log(LOG_DEBUG, "CVTPTransceiver::OpenStreamSocket - listening on %s:%d", inet_ntoa(address.sin_addr), address.sin_port);
  return true;
}

bool CVTPTransceiver::AcceptStreamSocket(SOCKET& sock2)
{
  SOCKET sock;
  sock = __accept(sock2, NULL, NULL);
  if(sock == INVALID_SOCKET)
  {
    XBMC->Log(LOG_ERROR, "CVTPStream::Accept - failed to accept incomming connection");
    return false;
  }

  __close(sock2);

  const char sol=1;
  // Ignore possible errors here, proceed as usual
  __setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &sol, sizeof(sol));

  sock2 = sock;
  return true;
}

void CVTPTransceiver::Close()
{
  if(m_VTPSocket != INVALID_SOCKET)
    __close(m_VTPSocket);
}

bool CVTPTransceiver::Connect(const string &host, int port)
{
  socklen_t       len;
  struct hostent *hp;

  if ((m_VTPSocket = __socket(PF_INET, SOCK_STREAM, IPPROTO_IP)) == INVALID_SOCKET)
  {
    XBMC->Log(LOG_ERROR, "CVTPTransceiver::Connect - Can't open socket '%s:%u'", host.c_str(), port);
    return false;
  }

#ifdef _LINUX
  int flags = fcntl(m_VTPSocket, F_GETFL);
  if (__fcntl(m_VTPSocket, F_SETFL, O_NONBLOCK) == -1)
#elif defined(_WIN32) || defined(_WIN64)
  u_long iMode = 1;
  if (__ioctlsocket(m_VTPSocket, FIONBIO, &iMode) == -1)
#endif
  {
    XBMC->Log(LOG_ERROR, "CVTPTransceiver::Connect - Can't set socket to non blocking mode");
    Close();
    return false;
  }

  if ((hp = __gethostbyname(host.c_str())) == NULL)
  {
    XBMC->Log(LOG_ERROR, "CVTPTransceiver::Connect - failed to resolve hostname: %s", host.c_str());
    Close();
    return false;
  }

  m_LocalAddr.sin_family = AF_INET;
  m_LocalAddr.sin_port   = 0;
  m_LocalAddr.sin_addr.s_addr = INADDR_ANY;
  if (__bind(m_VTPSocket, (struct sockaddr*)&m_LocalAddr, sizeof(m_LocalAddr)) == -1)
  {
    XBMC->Log(LOG_ERROR, "CVTPTransceiver::Connect - Can't bind requested socket '%s:%u'", host.c_str(), port);
    Close();
    return false;
  }

  m_RemoteAddr.sin_family = AF_INET;
  m_RemoteAddr.sin_port   = htons(port);
  memcpy(&m_RemoteAddr.sin_addr.s_addr, hp->h_addr_list[0], hp->h_length);
  if (__connect(m_VTPSocket, (struct sockaddr*)&m_RemoteAddr, sizeof(m_RemoteAddr)) < 0)
  {
    fd_set         set_r, set_w, set_e;
    timeval        timeout;

    timeout.tv_sec  = g_iConnectTimeout;
    timeout.tv_usec = 0;

    // fill with new data
    FD_ZERO(&set_r);
    FD_ZERO(&set_w);
    FD_ZERO(&set_e);
    FD_SET(m_VTPSocket, &set_r);
    FD_SET(m_VTPSocket, &set_w);
    FD_SET(m_VTPSocket, &set_e);
    int result = __select(FD_SETSIZE, &set_r, &set_w, &set_e, &timeout);
    if (result < 0)
    {
      XBMC->Log(LOG_ERROR, "CVTPTransceiver::Connect - select failed '%s:%u'", host.c_str(), port);
      Close();
      return false;
    }
    else if (result == 0)
    {
      XBMC->Log(LOG_ERROR, "CVTPTransceiver::Connect - connect timed out '%s:%u'", host.c_str(), port);
      Close();
      return false;
    }
    else if (!IsConnected(m_VTPSocket, &set_r, &set_w, &set_e))
    {
      XBMC->Log(LOG_ERROR, "CVTPTransceiver::Connect - failed to connect to IP '%d.%d.%d.%d",
                        (ntohl(m_RemoteAddr.sin_addr.s_addr) >> 24) & 0xff,
                        (ntohl(m_RemoteAddr.sin_addr.s_addr) >> 16) & 0xff,
                        (ntohl(m_RemoteAddr.sin_addr.s_addr) >> 8) & 0xff,
                        (ntohl(m_RemoteAddr.sin_addr.s_addr) & 0xff));
      Close();
      return false;
    }
  }

#ifdef _LINUX
  if (__fcntl(m_VTPSocket, F_SETFL, flags) == -1)
#elif defined(_WIN32) || defined(_WIN64)
  iMode = 0;
  if (__ioctlsocket(m_VTPSocket, FIONBIO, &iMode) == -1)
#endif
  {
    XBMC->Log(LOG_ERROR, "CVTPTransceiver::Connect - Can't set socket initial condition");
    Close();
    return false;
  }

  len = sizeof(struct sockaddr_in);
  if (__getpeername(m_VTPSocket, (struct sockaddr*)&m_RemoteAddr, &len) == -1)
  {
    XBMC->Log(LOG_ERROR, "CVTPTransceiver::Connect - Can't get the name of the peer socket '%s:%u'", host.c_str(), port);
    Close();
    return false;
  }

  len = sizeof(struct sockaddr_in);
  if (__getsockname(m_VTPSocket, (struct sockaddr*)&m_LocalAddr, &len) == -1)
  {
    XBMC->Log(LOG_ERROR, "CVTPTransceiver::Connect - Can't get the socket name '%s:%u'", host.c_str(), port);
    Close();
    return false;
  }

  // VTP Server will send a greeting
  string line;
  int    code;
  if (!ReadResponse(code, line))
  {
    XBMC->Log(LOG_ERROR, "CVTPTransceiver::Connect - Failed reading response");
    Close();
    return false;
  }

  XBMC->Log(LOG_INFO, "CVTPTransceiver::Connect - server greeting: %s", line.c_str());
  return true;
}

#if defined(_WIN32) || defined(_WIN64)
bool CVTPTransceiver::IsConnected(SOCKET socket, fd_set *rd, fd_set *wr, fd_set *ex)
{
  WSASetLastError(0);
  if (!FD_ISSET(socket, rd) && !FD_ISSET(socket, wr))
    return false;
  if (FD_ISSET(socket, ex))
    return false;
  return true;
}
#else
bool CVTPTransceiver::IsConnected(SOCKET socket, fd_set *rd, fd_set *wr, fd_set *ex)
{
  int err;
  socklen_t len = sizeof(err);

  errno = 0;             /* assume no error */
  if (!FD_ISSET(socket, rd ) && !FD_ISSET(socket, wr))
    return false;
  if (__getsockopt(socket, SOL_SOCKET, SO_ERROR, &err, &len) < 0)
    return false;
  errno = err;           /* in case we're not connected */
  return err == 0;
}
#endif

bool CVTPTransceiver::ReadResponse(int &code, string &line)
{
  vector<string> lines;
  if(ReadResponse(code, lines))
  {
    line = lines[lines.size()-1];
    return true;
  }
  return false;
}

bool CVTPTransceiver::ReadResponse(int &code, vector<string> &lines)
{
  fd_set         set_r, set_e;
  timeval        timeout;
  int            result;
  int            retries = 5;
  char           buffer[2048];
  char           cont = 0;
  string         line;
  size_t         pos1 = 0, pos2 = 0, pos3 = 0;

  while(true)
  {
    while( (pos1 = line.find("\r\n", pos3)) != std::string::npos)
    {
      if(sscanf(line.c_str(), "%d%c", &code, &cont) != 2)
      {
        XBMC->Log(LOG_DEBUG, "CVTPTransceiver::ReadResponse - unknown line format: %s", line.c_str());
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

    //TODO set 10 seconds timeout value??
    timeout.tv_sec  = 10;
    timeout.tv_usec = 0;

    // fill with new data
    FD_ZERO(&set_r);
    FD_ZERO(&set_e);
    FD_SET(m_VTPSocket, &set_r);
    FD_SET(m_VTPSocket, &set_e);
    result = __select(FD_SETSIZE, &set_r, NULL, &set_e, &timeout);
    if(result < 0)
    {
      XBMC->Log(LOG_DEBUG, "CVTPTransceiver::ReadResponse - select failed");
      m_VTPSocket = INVALID_SOCKET;
      return false;
    }

    if(result == 0)
    {
      XBMC->Log(LOG_DEBUG, "CVTPTransceiver::ReadResponse - timeout waiting for response, retrying...");
      if (retries != 0) {
          retries--;
      continue;
    }
      else {
          m_VTPSocket = INVALID_SOCKET;
          return false;
      }
    }

    result = __recv(m_VTPSocket, buffer, sizeof(buffer) - 1, 0);
    if(result < 0)
    {
      XBMC->Log(LOG_DEBUG, "CVTPTransceiver::ReadResponse - recv failed");
      m_VTPSocket = INVALID_SOCKET;
      return false;
    }
    buffer[result] = 0;

    line.append(buffer);
  }

  return true;
}

bool CVTPTransceiver::SendCommand(const string &command)
{
  fd_set set_w, set_e;
  struct timeval tv;
  int  result;
  char buffer[1024];
  int  len;

  len = sprintf(buffer, "%s\r\n", command.c_str());

  // fill with new data
  tv.tv_sec  = 0;
  tv.tv_usec = 0;

  FD_ZERO(&set_w);
  FD_ZERO(&set_e);
  FD_SET(m_VTPSocket, &set_w);
  FD_SET(m_VTPSocket, &set_e);
  result = __select(FD_SETSIZE, &set_w, NULL, &set_e, &tv);
  if(result < 0)
  {
    XBMC->Log(LOG_ERROR, "CVTPTransceiver::SendCommand - select failed");
    m_VTPSocket = INVALID_SOCKET;
    return false;
  }
  if (FD_ISSET(m_VTPSocket, &set_w))
  {
    XBMC->Log(LOG_ERROR, "CVTPTransceiver::SendCommand - failed to send data");
    m_VTPSocket = INVALID_SOCKET;
    return false;
  }
  if(__send(m_VTPSocket, buffer, len, 0) != len)
  {
    XBMC->Log(LOG_ERROR, "CVTPTransceiver::SendCommand - failed to send data");
    m_VTPSocket = INVALID_SOCKET;
    return false;
  }
  return true;
}

bool CVTPTransceiver::SendCommand(const string &command, int &code, string line)
{
  vector<string> lines;
  if(SendCommand(command, code, lines))
  {
    line = lines[lines.size()-1];
    return true;
  }
  return false;
}

bool CVTPTransceiver::SendCommand(const string &command, int &code, vector<string> &lines)
{
  if(!SendCommand(command))
    return false;

  if(!ReadResponse(code, lines))
    return false;

  if(code < 200 || code > 299)
  {
    if (code == 550 && lines[lines.size()-1] == "No schedule found") // Ignore error for missing EPG
      return true;

    XBMC->Log(LOG_ERROR, "CVTPTransceiver::SendCommand - Failed with code: %d (%s)", code, lines[lines.size()-1].c_str());
    return false;
  }

  return true;
}

bool CVTPTransceiver::CheckConnection()
{
  CMD_LOCK;

  if (IsOpen())
  {
    cTBSelect select;

#ifdef DEBUG_VTP
    XBMC->Log(LOG_DEBUG, "connection open");
#endif

    // XXX+ check if connection is still alive (is there a better way?)
    // There REALLY shouldn't be anything readable according to PROTOCOL here
    // If there is, assume it's an eof signal (subseq. read would return 0)
    select.Add(m_VTPSocket, false);
    int res;
    if ((res = select.Select(0)) == 0)
    {
#ifdef DEBUG_VTP
      XBMC->Log(LOG_DEBUG, "select said nothing happened");
#endif
      return true;
    }
    XBMC->Log(LOG_DEBUG, "closing connection (res was %d)", res);
    Close();
  }

  if (!Connect(g_szHostname, g_iPort))
  {
    static time_t lastTime = 0;
    if (time(NULL) - lastTime > MINLOGREPEAT)
    {
      XBMC->Log(LOG_ERROR, "Couldn't connect to %s:%d: %s", g_szHostname.c_str(), g_iPort, strerror(errno));
      lastTime = time(NULL);
    }
    return false;
  }

  string line;
  int    code = 220;
  if(!SendCommand("CAPS TS", code, line) || code != 220)
  {
    if (errno == 0)
      XBMC->Log(LOG_ERROR, "Couldn't negotiate capabilities on %s:%d", g_szHostname.c_str(), g_iPort);
    Close();
    return false;
  }

  const char *Filters = "";
  if(SendCommand("CAPS FILTERS", code, line) || code != 220)
    Filters = ",FILTERS";

  XBMC->Log(LOG_INFO, "Connected to server %s:%d using capabilities TS%s", g_szHostname.c_str(), g_iPort, Filters);
  return true;
}

bool CVTPTransceiver::ProvidesChannel(unsigned int Channel, int Priority)
{
  if (!CheckConnection()) return false;

  CMD_LOCK;

  string line;
  int    code;

  CStdString command;
  command.Format("PROV %i %d", Priority, Channel);
  if(!SendCommand(command, code, line))
  {
    if (command != "560" && errno == 0)
      XBMC->Log(LOG_ERROR, "Couldn't check if %s:%d provides channel %d", g_szHostname.c_str(), g_iPort, Channel);
    return false;
  }

  return true;
}

bool CVTPTransceiver::CreateDataConnection(eSocketId Id)
{
  if (!CheckConnection()) return false;

  if (m_DataSockets[Id] != INVALID_SOCKET)
    close(m_DataSockets[Id]);

  sockaddr_in address;
  SOCKET      sock;
  socklen_t   len = sizeof(address);
  string      result;
  int         code;

  if(__getsockname(m_VTPSocket, (struct sockaddr*) &address, &len) == SOCKET_ERROR)
  {
    XBMC->Log(LOG_ERROR, "Couldn't get socket name: %s", strerror(errno));
    return false;
  }

  XBMC->Log(LOG_DEBUG, "CVTPTransceiver::CreateDataConnection - local address %s:%d", inet_ntoa(address.sin_addr), ntohs(address.sin_port) );

  if(!OpenStreamSocket(sock, address))
  {
    XBMC->Log(LOG_ERROR, "Couldn't create data connection: %s", strerror(errno));
    return false;
  }

  int port = ntohs(address.sin_port);
  int addr = ntohl(address.sin_addr.s_addr);

  CStdString command;
  command.Format("PORT %d %d,%d,%d,%d,%d,%d", Id
                , (addr & 0xFF000000)>>24
                , (addr & 0x00FF0000)>>16
                , (addr & 0x0000FF00)>>8
                , (addr & 0x000000FF)>>0
                , (port & 0xFF00)>>8
                , (port & 0x00FF)>>0);

  CMD_LOCK;

  if(!SendCommand(command, code, result) || code != 220)
  {
    XBMC->Log(LOG_DEBUG, "error: %m");
    if (errno == 0)
      XBMC->Log(LOG_ERROR, "Couldn't establish data connection to %s:%d", g_szHostname.c_str(), g_iPort);
    return false;
  }

  if(!AcceptStreamSocket(sock))
  {
    XBMC->Log(LOG_ERROR, "Couldn't establish data connection to %s:%d%s%s", g_szHostname.c_str(), g_iPort, errno == 0 ? "" : ": ", errno == 0 ? "" : strerror(errno));
    close(sock);
    return false;
  }

  m_DataSockets[Id] = sock;
  return true;
}

bool CVTPTransceiver::CloseDataConnection(eSocketId Id)
{
  //if (!CheckConnection()) return false;

  CMD_LOCK;

  if(Id == siLive || Id == siLiveFilter || Id == siReplay || Id == siDataRespond)
  {
    if (m_DataSockets[Id] != INVALID_SOCKET)
    {
      string line;
      int    code;
      CStdString command;
      command.Format("ABRT %i", Id);
      if (!SendCommand(command, code, line) || code != 220)
      {
        if (errno == 0)
          XBMC->Log(LOG_ERROR, "Couldn't cleanly close data connection");
        //return false;
      }
      close(m_DataSockets[Id]);
      m_DataSockets[Id] = INVALID_SOCKET;
    }
  }
  return true;
}

bool CVTPTransceiver::SetChannelDevice(unsigned int Channel)
{
  if (!CheckConnection()) return false;

  CMD_LOCK;

  string result;
  int    code;
  CStdString command;
  command.Format("TUNE %d", Channel);
  if(!SendCommand(command, code, result) || code != 220)
  {
    if (errno == 0)
      XBMC->Log(LOG_ERROR, "Couldn't tune %s:%d to channel %d", g_szHostname.c_str(), g_iPort, Channel);
    return false;
	}
	return true;
}

bool CVTPTransceiver::SetRecordingIndex(unsigned int Recording)
{
  if (!CheckConnection()) return false;

  CMD_LOCK;

  string result;
  int    code;
  CStdString command;
  command.Format("PLAY %d", Recording);
  if (!SendCommand(command, code, result) || code != 220)
  {
    if (errno == 0)
      XBMC->Log(LOG_ERROR, "Couldn't open recording %d on %s:%d", Recording, g_szHostname.c_str(), g_iPort);
    return false;
  }
  return true;
}

bool CVTPTransceiver::GetPlayingRecordingSize(uint64_t *size, uint32_t *frames)
{
  vector<string> lines;
  int            code;

  if (!CheckConnection()) return false;

  CMD_LOCK;

  if (!SendCommand("SIZE", code, lines) || code != 220)
    return false;

  vector<string>::iterator it = lines.begin();
  string& data(*it);

  *size = atoll(data.c_str());
  *frames = atol(data.substr(data.find(" ") + 1).c_str());
  return true;
}

uint64_t CVTPTransceiver::SeekRecordingPosition(uint64_t position)
{
  vector<string> lines;
  int            code;

  if (!CheckConnection()) return 0;

  CMD_LOCK;

  CStdString command;
  command.Format("SEEK %llu", position);
  if (!SendCommand(command, code, lines) || code != 220)
  {
    if (errno == 0)
      XBMC->Log(LOG_ERROR, "Couldn't seek to position %llu on %s:%d", position, g_szHostname.c_str(), g_iPort);
    return 0;
  }

  vector<string>::iterator it = lines.begin();
  string& data(*it);
  return atoll(data.c_str());;
}

CStdString CVTPTransceiver::GetBackendName()
{
  vector<string>  lines;
  int             code;

  if (!CheckConnection())
    return "";

  CMD_LOCK;

  if (!SendCommand("STAT name", code, lines))
    return "";

  vector<string>::iterator it = lines.begin();
  string& data(*it);
  return data;
}

CStdString CVTPTransceiver::GetBackendVersion()
{
  vector<string>  lines;
  int             code;

  if (!CheckConnection())
    return "";

  CMD_LOCK;

  if (!SendCommand("STAT version", code, lines))
    return "";

  vector<string>::iterator it = lines.begin();
  string& data(*it);
  return data;
}

PVR_ERROR CVTPTransceiver::GetDriveSpace(long long *total, long long *used)
{
  vector<string>  lines;
  int             code;

  if (!CheckConnection())
    return PVR_ERROR_SERVER_ERROR;

  CMD_LOCK;

  if (!SendCommand("STAT disk", code, lines))
    return PVR_ERROR_SERVER_ERROR;

  vector<string>::iterator it = lines.begin();
  string& data(*it);
  size_t found = data.find("MB");
  if (found != CStdString::npos)
  {
    *total = atol(data.c_str()) * 1024;
    data.erase(0, found + 3);
    *used = atol(data.c_str()) * 1024;
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CVTPTransceiver::GetBackendTime(time_t *localTime, int *gmtOffset)
{
  vector<string>  lines;
  int             code;

  if (!CheckConnection())
    return PVR_ERROR_SERVER_ERROR;

  CMD_LOCK;

  if (!SendCommand("STAT time", code, lines))
    return PVR_ERROR_SERVER_ERROR;

  vector<string>::iterator it = lines.begin();
  string& data(*it);

  *localTime = atol(data.c_str());
  *gmtOffset = atol(data.substr(data.find(" ") + 1).c_str());
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CVTPTransceiver::RequestEPGForChannel(const PVR_CHANNEL &channel, PVRHANDLE handle, time_t start, time_t end)
{
  vector<string> lines;
  int            code;
  cEpg           epg;

  if (!CheckConnection()) return PVR_ERROR_SERVER_ERROR;

  CMD_LOCK;

  CStdString command;
  if (start != 0)
    command.Format("LSTE %d from %lu to %lu", channel.number, (long)start, (long)end);
  else
    command.Format("LSTE %d", channel.number);
  while (!SendCommand(command, code, lines) || code != 215)
  {
    if (code == 550)
      return PVR_ERROR_NO_ERROR;
    else if (code != 451)
      return PVR_ERROR_SERVER_ERROR;
    Sleep(100);
  }

  for (vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
  {
    string& data(*it);
    CStdString str_result = data;

    if (g_bCharsetConv)
      XBMC->UnknownToUTF8(str_result);

    bool isEnd = epg.ParseLine(str_result.c_str());
    if (isEnd && epg.StartTime() != 0)
    {
      PVR_PROGINFO broadcast;
      broadcast.channum         = channel.number;
      broadcast.uid             = epg.UniqueId();
      broadcast.title           = epg.Title();
      broadcast.subtitle        = epg.ShortText();
      broadcast.description     = epg.Description();
      broadcast.starttime       = epg.StartTime();
      broadcast.endtime         = epg.EndTime();
      broadcast.genre_type      = epg.GenreType();
      broadcast.genre_sub_type  = epg.GenreSubType();
      broadcast.parental_rating = epg.ParentalRating();
      PVR->TransferEpgEntry(handle, &broadcast);
      epg.Reset();
    }
  }

  return PVR_ERROR_NO_ERROR;
}

int CVTPTransceiver::GetNumChannels()
{
  vector<string>  lines;
  int             code;

  if (!CheckConnection()) return -1;

  CMD_LOCK;

  if (!SendCommand("STAT channels", code, lines))
    return -1;

  vector<string>::iterator it = lines.begin();
  string& data(*it);
  return atol(data.c_str());
}

PVR_ERROR CVTPTransceiver::RequestChannelList(PVRHANDLE handle, bool radio)
{
  vector<string> lines;
  int            code;

  if (!g_bRadioEnabled && radio) return PVR_ERROR_NO_ERROR;
  if (!CheckConnection()) return PVR_ERROR_SERVER_ERROR;

  CMD_LOCK;

  while (!SendCommand("LSTC", code, lines))
  {
    if (code != 451)
      return PVR_ERROR_SERVER_ERROR;
    Sleep(10);
  }

  for (vector<string>::iterator it = lines.begin(); it < lines.end(); it++)
  {
    string& data(*it);
    CStdString str_result = data;

    if (g_bCharsetConv)
      XBMC->UnknownToUTF8(str_result);

    cChannel channel;
    channel.Parse(str_result.c_str());

    /* Ignore channels without streams */
    if ((g_bNoBadChannels && channel.Vpid() == 0 && channel.Apid(0) == 0 && channel.Dpid(0) == 0) || (g_bOnlyFTA && channel.Ca() != 0))
      continue;

    PVR_CHANNEL tag;
    tag.uid         = channel.Sid();
    tag.number      = channel.Number();
    tag.name        = channel.Name();
    tag.callsign    = channel.Name();
    tag.iconpath    = "";
    tag.encryption  = channel.Ca();
    tag.radio       = (channel.Vpid() == 0) && (channel.Apid(0) != 0) ? true : false;
    tag.hide        = false;
    tag.recording   = false;
    tag.bouquet     = 0;
    tag.multifeed   = false;
    tag.input_format = "mpegts";
    tag.stream_url  = "";

    if (radio == tag.radio)
      PVR->TransferChannelEntry(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
}

int CVTPTransceiver::GetNumRecordings(void)
{
  vector<string>  lines;
  int             code;

  if (!CheckConnection()) return -1;

  CMD_LOCK;

  if (!SendCommand("STAT records", code, lines))
    return -1;

  vector<string>::iterator it = lines.begin();
  string& data(*it);
  return atol(data.c_str());
}

void CVTPTransceiver::ScanVideoDir(PVRHANDLE handle, const char *DirName, bool Deleted, int LinkLevel)
{
  cReadDir d(DirName);
  struct dirent *e;
  while ((e = d.Next()) != NULL)
  {
    if (strcmp(e->d_name, ".") && strcmp(e->d_name, ".."))
    {
      char *buffer = strdup(AddDirectory(DirName, e->d_name));
      struct stat st;
      if (stat(buffer, &st) == 0)
      {
        int Link = 0;
        if (S_ISLNK(st.st_mode))
        {
          if (LinkLevel > MAX_LINK_LEVEL)
          {
            XBMC->Log(LOG_ERROR, "max link level exceeded - not scanning %s", buffer);
            continue;
          }
          Link = 1;
          char *old = buffer;
          buffer = ReadLink(old);
          free(old);
          if (!buffer)
            continue;
          if (stat(buffer, &st) != 0)
          {
            free(buffer);
            continue;
          }
        }
        if (S_ISDIR(st.st_mode))
        {
          if (endswith(buffer, Deleted ? DELEXT : RECEXT))
          {
            cRecording recording(buffer);

            PVR_RECORDINGINFO tag;
            tag.index           = m_recIndex++;
            tag.channel_name    = recording.ChannelName();
            tag.lifetime        = recording.Lifetime();
            tag.priority        = recording.Priority();
            tag.recording_time  = recording.StartTime();
            tag.duration        = recording.Duration();
            tag.subtitle        = recording.ShortText();
            tag.description     = recording.Description();
            tag.title           = recording.Title();
            tag.directory       = recording.Directory();
            tag.stream_url      = recording.StreamURL();

            PVR->TransferRecordingEntry(handle, &tag);
          }
          else
            ScanVideoDir(handle, buffer, Deleted, LinkLevel + Link);
        }
      }
      free(buffer);
    }
  }
}

PVR_ERROR CVTPTransceiver::RequestRecordingsList(PVRHANDLE handle)
{
  if (g_bUseRecordingsDir && g_szRecordingsDir != "")
  {
    m_recIndex = 1;
    ScanVideoDir(handle, g_szRecordingsDir.c_str());
  }
  else
  {
    vector<string> linesShort;
    int            code;

    if (!CheckConnection()) return PVR_ERROR_SERVER_ERROR;

    CMD_LOCK;

    if (!SendCommand("LSTR", code, linesShort))
      return PVR_ERROR_SERVER_ERROR;

    for (vector<string>::iterator it = linesShort.begin(); it != linesShort.end(); it++)
    {
      string& data(*it);
      CStdString str_result = data;

      /* Convert to UTF8 string format */
      if (g_bCharsetConv)
        XBMC->UnknownToUTF8(str_result);

      cRecording recording;
      if (recording.ParseEntryLine(str_result.c_str()))
      {
        vector<string> linesDetails;

        CStdString command;
        command.Format("LSTR %i", recording.Index());
        if (!SendCommand(command, code, linesDetails))
          continue;

        for (vector<string>::iterator it2 = linesDetails.begin(); it2 != linesDetails.end(); it2++)
        {
          string& data2(*it2);
          CStdString str_details = data2;

          /* Convert to UTF8 string format */
          if (g_bCharsetConv)
            XBMC->UnknownToUTF8(str_details);

          recording.ParseLine(str_details.c_str());
        }

        PVR_RECORDINGINFO tag;
        tag.index           = recording.Index();
        tag.channel_name    = recording.ChannelName();
        tag.lifetime        = recording.Lifetime();
        tag.priority        = recording.Priority();
        tag.recording_time  = recording.StartTime();
        tag.duration        = recording.Duration();
        tag.subtitle        = recording.ShortText();
        tag.description     = recording.Description();
        tag.stream_url      = "";
        tag.title           = recording.FileName();
        tag.directory       = recording.Directory();

        PVR->TransferRecordingEntry(handle, &tag);
      }
    }
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CVTPTransceiver::DeleteRecording(const PVR_RECORDINGINFO &recinfo)
{
  vector<string> lines;
  int            code;

  if (!CheckConnection()) return PVR_ERROR_SERVER_ERROR;

  CMD_LOCK;

  CStdString command;
  command.Format("LSTR %i", recinfo.index);
  if (!VTPTransceiver.SendCommand(command, code, lines))
    return PVR_ERROR_SERVER_ERROR;
  if (code != 215)
    return PVR_ERROR_NOT_SYNC;

  command.Format("DELR %i", recinfo.index);
  if (!SendCommand(command, code, lines))
    return PVR_ERROR_SERVER_ERROR;
  if (code != 250)
    return PVR_ERROR_NOT_DELETED;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CVTPTransceiver::RenameRecording(const PVR_RECORDINGINFO &recinfo, const char *newname)
{
  vector<string> lines;
  int            code;

  if (!CheckConnection()) return PVR_ERROR_SERVER_ERROR;

  CMD_LOCK;

  CStdString command;
  command.Format("LSTR %i", recinfo.index);
  if (!SendCommand(command, code, lines))
    return PVR_ERROR_SERVER_ERROR;
  if (code != 215)
    return PVR_ERROR_NOT_SYNC;

  CStdString renamedName = recinfo.directory;
  if (renamedName != "" && renamedName[renamedName.size()-1] != '/')
    renamedName += "/";
  renamedName += newname;
  renamedName.Replace('/','~');;

  command.Format("RENR %d %s", recinfo.index, renamedName.c_str());
  if (!SendCommand(command, code, lines))
    return PVR_ERROR_SERVER_ERROR;
  if (code != 250)
    return PVR_ERROR_NOT_DELETED;

  return PVR_ERROR_NO_ERROR;
}

int CVTPTransceiver::GetNumTimers(void)
{
  vector<string>  lines;
  int             code;

  if (!CheckConnection()) return -1;

  CMD_LOCK;

  if (!SendCommand("STAT timers", code, lines))
    return -1;

  vector<string>::iterator it = lines.begin();
  string& data(*it);
  return atol(data.c_str());
}

PVR_ERROR CVTPTransceiver::RequestTimerList(PVRHANDLE handle)
{
  vector<string> lines;
  int            code;

  if (!CheckConnection()) return PVR_ERROR_SERVER_ERROR;

  CMD_LOCK;

  if (!SendCommand("LSTT", code, lines))
    return PVR_ERROR_SERVER_ERROR;

  for (vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
  {
    string& data(*it);
    CStdString str_result = data;

    /**
     * VDR Format given by LSTT:
     * 250-1 1:6:2008-10-27:0013:0055:50:99:Zeiglers wunderbare Welt des Fußballs:
     * 250 2 0:15:2008-10-26:2000:2138:50:99:ZDFtheaterkanal:
     * 250 3 1:6:MTWTFS-:2000:2129:50:99:WDR Köln:
     */

    if (g_bCharsetConv)
      XBMC->UnknownToUTF8(str_result);

    cTimer timer;
    timer.Parse(str_result.c_str());

    PVR_TIMERINFO tag;
    tag.index       = timer.Index();
    tag.active      = timer.HasFlags(tfActive);
    tag.channelNum  = timer.Channel();
    tag.firstday    = timer.FirstDay();
    tag.starttime   = timer.StartTime();
    tag.endtime     = timer.StopTime();
    tag.recording   = timer.HasFlags(tfRecording) || timer.HasFlags(tfInstant);
    tag.title       = timer.Title();
    tag.directory   = timer.Dir();
    tag.priority    = timer.Priority();
    tag.lifetime    = timer.Lifetime();
    tag.repeat      = timer.WeekDays() == 0 ? false : true;
    tag.repeatflags = timer.WeekDays();

    PVR->TransferTimerEntry(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CVTPTransceiver::GetTimerInfo(unsigned int timernumber, PVR_TIMERINFO &tag)
{
  vector<string>  lines;
  int             code;

  if (!VTPTransceiver.CheckConnection()) return PVR_ERROR_SERVER_ERROR;

  CMD_LOCK;

  CStdString command;
  command.Format("LSTT %i", timernumber);
  if (!VTPTransceiver.SendCommand(command, code, lines))
    return PVR_ERROR_SERVER_ERROR;

  vector<string>::iterator it = lines.begin();
  string& data(*it);
  CStdString str_result = data;

  if (g_bCharsetConv)
    XBMC->UnknownToUTF8(str_result);

  cTimer timer;
  timer.Parse(str_result.c_str());

  tag.index       = timer.Index();
  tag.active      = timer.HasFlags(tfActive);
  tag.channelNum  = timer.Channel();
  tag.firstday    = timer.FirstDay();
  tag.starttime   = timer.StartTime();
  tag.endtime     = timer.StopTime();
  tag.recording   = timer.HasFlags(tfRecording) || timer.HasFlags(tfInstant);
  tag.title       = timer.File();
  tag.priority    = timer.Priority();
  tag.lifetime    = timer.Lifetime();
  tag.repeat      = timer.WeekDays() == 0 ? false : true;
  tag.repeatflags = timer.WeekDays();

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CVTPTransceiver::AddTimer(const PVR_TIMERINFO &timerinfo)
{
  vector<string> lines;
  int            code;

  if (!CheckConnection())
    return PVR_ERROR_SERVER_ERROR;

  cTimer timer(&timerinfo);

  CMD_LOCK;

  CStdString command;
  if (timerinfo.index == -1)
  {
    command.Format("NEWT %s", timer.ToText().c_str());
    if (!SendCommand(command, code, lines))
      return PVR_ERROR_NOT_SAVED;
    if (code != 250)
      return PVR_ERROR_NOT_SYNC;
  }
  else
  {
    // Modified timer
    command.Format("LSTT %i", timerinfo.index);
    if (!SendCommand(command, code, lines))
      return PVR_ERROR_SERVER_ERROR;
    if (code != 250)
      return PVR_ERROR_NOT_SYNC;

    command.Format("MODT %d %s", timerinfo.index, timer.ToText().c_str());
    if (!SendCommand(command, code, lines))
      return PVR_ERROR_NOT_SAVED;
    if (code != 250)
      return PVR_ERROR_NOT_SYNC;
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CVTPTransceiver::DeleteTimer(const PVR_TIMERINFO &timerinfo, bool force)
{
  vector<string> lines;
  int            code;

  if (!CheckConnection()) return PVR_ERROR_SERVER_ERROR;

  CMD_LOCK;

  CStdString command;
  command.Format("LSTT %d", timerinfo.index);
  if (!SendCommand(command, code, lines))
    return PVR_ERROR_SERVER_ERROR;
  if (code != 250)
    return PVR_ERROR_NOT_SYNC;

  lines.erase(lines.begin(), lines.end());

  if (force)
    command.Format("DELT %d FORCE", timerinfo.index);
  else
    command.Format("DELT %d", timerinfo.index);
  if (!SendCommand(command, code, lines))
  {
    vector<string>::iterator it = lines.begin();
    string& data(*it);
    CStdString str_result = data;
    if (str_result.find("is recording", 0) != std::string::npos)
      return PVR_ERROR_RECORDING_RUNNING;
    else
      return PVR_ERROR_NOT_DELETED;
  }
  if (code != 250)
    return PVR_ERROR_NOT_SYNC;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CVTPTransceiver::RenameTimer(const PVR_TIMERINFO &timerinfo, const char *newname)
{
  PVR_TIMERINFO timerinfo1;
  PVR_ERROR ret = GetTimerInfo(timerinfo.index, timerinfo1);
  if (ret != PVR_ERROR_NO_ERROR)
    return ret;

  timerinfo1.title = newname;
  return UpdateTimer(timerinfo1);
}

PVR_ERROR CVTPTransceiver::UpdateTimer(const PVR_TIMERINFO &timerinfo)
{
  vector<string> lines;
  int            code;

  if (!CheckConnection()) return PVR_ERROR_SERVER_ERROR;
  if (timerinfo.index == -1) return PVR_ERROR_NOT_SAVED;

  cTimer timer(&timerinfo);

  CMD_LOCK;

  CStdString command;
  command.Format("LSTT %i", timerinfo.index);
  if (!VTPTransceiver.SendCommand(command, code, lines))
    return PVR_ERROR_SERVER_ERROR;
  if (code != 250)
    return PVR_ERROR_NOT_SYNC;

  command.Format("MODT %d %s", timerinfo.index, timer.ToText().c_str());
  if (!VTPTransceiver.SendCommand(command, code, lines))
    return PVR_ERROR_NOT_SAVED;
  if (code != 250)
    return PVR_ERROR_NOT_SYNC;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CVTPTransceiver::SignalQuality(PVR_SIGNALQUALITY &qualityinfo, unsigned int channel)
{
  vector<string> lines;
  int            code;

  if (!CheckConnection()) return PVR_ERROR_SERVER_ERROR;

  CMD_LOCK;

  CStdString command;
  command.Format("LSTQ %i", channel);
  if (!SendCommand(command, code, lines) || code != 215)
    return PVR_ERROR_SERVER_ERROR;

  for (vector<string>::iterator it = lines.begin(); it < lines.end(); it++)
  {
    string& data(*it);
    const char *s = data.c_str();
    if (!strncasecmp(s, "Device", 6))
      strncpy(qualityinfo.frontend_name, s + 9, sizeof(qualityinfo.frontend_name));
    else if (!strncasecmp(s, "Status", 6))
      strncpy(qualityinfo.frontend_status, s + 9, sizeof(qualityinfo.frontend_status));
    else if (!strncasecmp(s, "Signal", 6))
      qualityinfo.signal = (uint16_t)strtol(s + 9, NULL, 16);
    else if (!strncasecmp(s, "SNR", 3))
      qualityinfo.snr = (uint16_t)strtol(s + 9, NULL, 16);
    else if (!strncasecmp(s, "BER", 3))
      qualityinfo.ber = (uint32_t)strtol(s + 9, NULL, 16);
    else if (!strncasecmp(s, "UNC", 3))
      qualityinfo.unc = (uint32_t)strtol(s + 9, NULL, 16);
    else if (!strncasecmp(s, "Video", 5))
      qualityinfo.video_bitrate = strtod(s + 9, NULL);
    else if (!strncasecmp(s, "Audio", 5))
      qualityinfo.audio_bitrate = strtod(s + 9, NULL);
    else if (!strncasecmp(s, "Dolby", 5))
      qualityinfo.dolby_bitrate = strtod(s + 9, NULL);
  }

  return PVR_ERROR_NO_ERROR;
}

int CVTPTransceiver::TransferRecordingToSocket(uint64_t position, int size)
{
  if (!CheckConnection()) return 0;

  CMD_LOCK;

  vector<string> lines;
  int            code;

  CStdString command;
  command.Format("READ %llu %u", (unsigned long long)position, size);
  if (!SendCommand(command, code, lines) || code != 220)
    return 0;

  vector<string>::iterator it = lines.begin();
  string& data(*it);

  return atol(data.c_str());
}

bool CVTPTransceiver::Quit(void)
{
  vector<string> lines;
  int            code;
  bool           ret;

  if (!CheckConnection()) return false;

  if (!(ret = SendCommand("QUIT", code, lines)) || code != 221)
  {
    if (errno == 0)
      XBMC->Log(LOG_ERROR, "ERROR: Streamdev: Couldn't quit command connection to %s:%d", g_szHostname.c_str(), g_iPort);
  }
  Close();
  return ret;

}

bool CVTPTransceiver::SuspendServer(void)
{
  vector<string> lines;
  int            code;

  if (!CheckConnection()) return 0;

  CMD_LOCK;

  if (!SendCommand("SUSP", code, lines) || code != 220)
  {
    XBMC->Log(LOG_ERROR, "Couldn't suspend server");
    return false;
  }
  return true;
}


