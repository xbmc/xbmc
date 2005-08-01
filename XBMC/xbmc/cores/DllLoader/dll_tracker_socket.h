
#ifndef _DLL_TRACKER_SOCKET
#define _DLL_TRACKER_SOCKET

#include "dll_tracker.h"

extern "C" void tracker_socket_free_all(DllTrackInfo* pInfo);

extern "C"
{
  int __stdcall track_socket(int af, int type, int protocol);
  int __stdcall track_closesocket(int socket);
  int __stdcall track_accept(SOCKET s, struct sockaddr FAR * addr, OUT int FAR * addrlen);
}

#endif // _DLL_TRACKER_SOCKET