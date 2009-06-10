#pragma once

#ifndef VTPSESSION_H
#define VTPSESSION_H

#include <string>
#include <vector>

#ifdef HAVE_WINSOCK2
#include <winsock2.h>
typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
typedef int SOCKET;
#define SOCKET_ERROR   (-1)
#endif

class CVTPTransceiver
{
public:
  CVTPTransceiver();
  ~CVTPTransceiver();
  bool Open(const std::string &host, int port);
  void Close();
  bool IsConnected(SOCKET socket, fd_set *rd, fd_set *wr, fd_set *ex);

  bool ReadResponse(int &code, std::string &line);
  bool ReadResponse(int &code, std::vector<std::string> &lines);

  bool SendCommand(const std::string &command);
  bool SendCommand(const std::string &command, int &code, std::string line);
  bool SendCommand(const std::string &command, int &code, std::vector<std::string> &lines);

  SOCKET GetStreamLive(int channel);
  SOCKET GetStreamRecording(int recording, uint64_t *size, uint32_t *frames);
  SOCKET GetStreamData();
  void   AbortStreamLive();
  void   AbortStreamRecording();
  void	 AbortStreamData();
  bool   CanStreamLive(int channel);
  bool   IsOpen();
  bool   SuspendServer();
  bool   Quit();

private:
  struct sockaddr_in m_LocalAddr;
  struct sockaddr_in m_RemoteAddr;

  bool   OpenStreamSocket(SOCKET& socket, struct sockaddr_in& address);
  bool   AcceptStreamSocket(SOCKET& socket);

  SOCKET m_socket;
};

#endif
