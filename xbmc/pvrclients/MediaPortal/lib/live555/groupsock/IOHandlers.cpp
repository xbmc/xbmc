/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "mTunnel" multicast access service
// Copyright (c) 1996-2010 Live Networks, Inc.  All rights reserved.
// IO event handlers
// Implementation

#include "IOHandlers.hh"
#include "TunnelEncaps.hh"

//##### TEMP: Use a single buffer, sized for UDP tunnels:
//##### This assumes that the I/O handlers are non-reentrant
static unsigned const maxPacketLength = 50*1024; // bytes
    // This is usually overkill, because UDP packets are usually no larger
    // than the typical Ethernet MTU (1500 bytes).  However, I've seen
    // reports of Windows Media Servers sending UDP packets as large as
    // 27 kBytes.  These will probably undego lots of IP-level
    // fragmentation, but that occurs below us.  We just have to hope that
    // fragments don't get lost.
static unsigned const ioBufferSize
	= maxPacketLength + TunnelEncapsulationTrailerMaxSize;
static unsigned char ioBuffer[ioBufferSize];


void socketReadHandler(Socket* sock, int /*mask*/) {
  unsigned bytesRead;
  struct sockaddr_in fromAddress;
  UsageEnvironment& saveEnv = sock->env();
      // because handleRead(), if it fails, may delete "sock"
  if (!sock->handleRead(ioBuffer, ioBufferSize, bytesRead, fromAddress)) {
    saveEnv.reportBackgroundError();
  }
}
