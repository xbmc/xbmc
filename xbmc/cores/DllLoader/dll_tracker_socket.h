#ifndef _DLL_TRACKER_SOCKET
#define _DLL_TRACKER_SOCKET

/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "dll_tracker.h"

extern "C" void tracker_socket_free_all(DllTrackInfo* pInfo);

extern "C"
{
  SOCKET __stdcall track_socket(int af, int type, int protocol);
  int __stdcall track_closesocket(SOCKET socket);
  SOCKET __stdcall track_accept(SOCKET s, struct sockaddr FAR * addr, OUT int FAR * addrlen);
}

#endif // _DLL_TRACKER_SOCKET
