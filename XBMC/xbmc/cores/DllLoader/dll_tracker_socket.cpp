
#include "../../stdafx.h"
#include "dll_tracker_socket.h"
#include "dll_tracker.h"
#include "exports/emu_socket.h"

extern "C" void tracker_socket_track(unsigned long caller, SOCKET socket)
{
  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(caller);
  if (pInfo)
  {
    pInfo->socketList.push_back(socket);
  }
}

extern "C" void tracker_socket_free(unsigned long caller, SOCKET socket)
{
  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(caller);
  if (pInfo)
  {
    pInfo->socketList.remove(socket);
  }
}

extern "C" void tracker_socket_free_all(DllTrackInfo* pInfo)
{
  if (!pInfo->fileList.empty())
  {
    SOCKET socket;
    CLog::DebugLog("%s: Detected open sockets: %d", pInfo->pDll->GetFileName(), pInfo->socketList.size());
    for (SocketListIter it = pInfo->socketList.begin(); it != pInfo->socketList.end(); ++it)
    {
      socket = *it;
      CLog::DebugLog("socket des. : %x", socket);
      dllclosesocket(socket);
    }
  }
  pInfo->socketList.erase(pInfo->socketList.begin(), pInfo->socketList.end());
}

extern "C"
{
  int __stdcall track_socket(int af, int type, int protocol)
  {
    unsigned loc;
    __asm mov eax, [ebp + 4]
    __asm mov loc, eax
    
    SOCKET socket = dllsocket(af, type, protocol);
    if (socket) tracker_socket_track(loc, socket);
    return socket;
  }

  int __stdcall track_closesocket(int socket)
  {
    unsigned loc;
    __asm mov eax, [ebp + 4]
    __asm mov loc, eax
    
    tracker_socket_free(loc, socket);
    return dllclosesocket(socket);
  }
  
  int __stdcall track_accept(SOCKET s, struct sockaddr FAR * addr, OUT int FAR * addrlen)
  {
    unsigned loc;
    __asm mov eax, [ebp + 4]
    __asm mov loc, eax
    
    SOCKET socket = dllaccept(s, addr, addrlen);
    if (socket) tracker_socket_track(loc, socket);
    return socket;
  }
}