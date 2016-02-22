/*
 *      vdr-plugin-vnsi - XBMC server plugin for VDR
 *
 *      Copyright (C) 2003-2006 Petri Hintukainen
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2011 Alexander Pipelka
 *
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

#ifndef VNSI_CXSOCKET_H
#define VNSI_CXSOCKET_H

#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <vdr/thread.h>
#include <vdr/tools.h>

class cxSocket {
 private:
  int m_fd;
  cMutex m_MutexWrite;
  cMutex m_MutexRead;

  cPoller *m_pollerRead;
  cPoller *m_pollerWrite;

  cxSocket(const cxSocket& s);
  cxSocket &operator=(const cxSocket &S);

 public:

  cxSocket() : m_fd(-1), m_pollerRead(NULL), m_pollerWrite(NULL) {}

  ~cxSocket();

  void set_handle(int h);

  void close(void);

  ssize_t read(void *buffer, size_t size, int timeout_ms = -1);

  ssize_t write(const void *buffer, size_t size, int timeout_ms = -1, bool more_data = false);

  static char *ip2txt(uint32_t ip, unsigned int port, char *str);
};

#endif // VNSI_CXSOCKET_H
