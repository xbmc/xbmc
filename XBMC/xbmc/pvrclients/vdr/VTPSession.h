#pragma once

#ifndef VTPSESSION_H
#define VTPSESSION_H

#include <string>
#include <vector>

#if VTP_STANDALONE

#ifdef HAVE_WINSOCK2
#include <winsock2.h>
typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#define closesocket(a) close(a)
typedef int SOCKET;
#define SOCKET_ERROR   (-1)
#define INVALID_SOCKET (-1)
#endif

//namespace Log {
//  void Log(PVR_LOG level, const char* format) 
//  { printf(format);
//  printf("\n"); 
//  }
//  template<typename T1>
//  void Log(PVR_LOG level, const char* format, T1 p1) 
//  { printf(format, p1); 
//  printf("\n"); 
//  }
//
//  template<typename T1, typename T2>
//  void Log(PVR_LOG level, const char* format, T1 p1, T2 p2) 
//  { printf(format, p1, p2); 
//  printf("\n"); 
//  }
//  template<typename T1, typename T2, typename T3>
//  void Log(PVR_LOG level, const char* format, T1 p1, T2 p2, T3 p3) 
//  { printf(format, p1, p2, p3); 
//  printf("\n"); 
//  }
//}
#endif

class CVTPSession
{
public:
  CVTPSession();
  ~CVTPSession();
  bool Open(const std::string &host, int port);
  void Close();

  bool ReadResponse(int &code, std::string &line);
  bool ReadResponse(int &code, std::vector<std::string> &lines);

  bool SendCommand(const std::string &command);
  bool SendCommand(const std::string &command, int &code, std::string line);
  bool SendCommand(const std::string &command, int &code, std::vector<std::string> &lines);

  struct Channel
  {
    int         index;
    std::string name;
    std::string network;
  };

  bool   GetChannels(std::vector<Channel> &channels);

  SOCKET GetStreamLive(int channel);
  SOCKET GetStreamRecording(int recording, uint64_t *size, uint32_t *frames);
  void   AbortStreamLive();
  void   AbortStreamRecording();
  bool     CanStreamLive(int channel);
  bool IsOpen();
  bool SuspendServer();
  bool Quit();

private:
  bool   OpenStreamSocket(SOCKET& socket, struct sockaddr_in& address);
  bool AcceptStreamSocket(SOCKET& socket);

  SOCKET m_socket;
};

#endif