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
// "liveMedia"
// Copyright (c) 1996-2010 Live Networks, Inc.  All rights reserved.
// An abstraction of a network interface used for RTP (or RTCP).
// (This allows the RTP-over-TCP hack (RFC 2326, section 10.12) to
// be implemented transparently.)
// C++ header

#ifndef _RTP_INTERFACE_HH
#define _RTP_INTERFACE_HH

#ifndef _MEDIA_HH
#include <Media.hh>
#endif
#ifndef _GROUPSOCK_HH
#include "Groupsock.hh"
#endif

// Typedef for an optional auxilliary handler function, to be called
// when each new packet is read:
typedef void AuxHandlerFunc(void* clientData, unsigned char* packet,
			    unsigned packetSize);

typedef void ServerRequestAlternativeByteHandler(void* instance, u_int8_t requestByte);
// A hack that allows a handler for RTP/RTCP packets received over TCP to process RTSP commands that may also appear within
// the same TCP connection.  A RTSP server implementation would supply a function like this - as a parameter to
// "ServerMediaSubsession::startStream()".

class tcpStreamRecord {
public:
  tcpStreamRecord(int streamSocketNum, unsigned char streamChannelId,
		  tcpStreamRecord* next);
  virtual ~tcpStreamRecord();

public:
  tcpStreamRecord* fNext;
  int fStreamSocketNum;
  unsigned char fStreamChannelId;
};

class RTPInterface {
public:
  RTPInterface(Medium* owner, Groupsock* gs);
  virtual ~RTPInterface();

  Groupsock* gs() const { return fGS; }

  void setStreamSocket(int sockNum, unsigned char streamChannelId);
  void addStreamSocket(int sockNum, unsigned char streamChannelId);
  void removeStreamSocket(int sockNum, unsigned char streamChannelId);
  void setServerRequestAlternativeByteHandler(ServerRequestAlternativeByteHandler* handler, void* clientData);

  void sendPacket(unsigned char* packet, unsigned packetSize);
  void startNetworkReading(TaskScheduler::BackgroundHandlerProc*
                           handlerProc);
  Boolean handleRead(unsigned char* buffer, unsigned bufferMaxSize,
		     unsigned& bytesRead,
		     struct sockaddr_in& fromAddress);
  void stopNetworkReading();

  UsageEnvironment& envir() const { return fOwner->envir(); }

  void setAuxilliaryReadHandler(AuxHandlerFunc* handlerFunc,
				void* handlerClientData) {
    fAuxReadHandlerFunc = handlerFunc;
    fAuxReadHandlerClientData = handlerClientData;
  }

  // A hack for supporting handlers for RTCP packets arriving interleaved over TCP:
  int nextTCPReadStreamSocketNum() const { return fNextTCPReadStreamSocketNum; }
  unsigned char nextTCPReadStreamChannelId() const { return fNextTCPReadStreamChannelId; }

private:
  friend class SocketDescriptor;
  Medium* fOwner;
  Groupsock* fGS;
  tcpStreamRecord* fTCPStreams; // optional, for RTP-over-TCP streaming/receiving

  unsigned short fNextTCPReadSize;
    // how much data (if any) is available to be read from the TCP stream
  int fNextTCPReadStreamSocketNum;
  unsigned char fNextTCPReadStreamChannelId;
  TaskScheduler::BackgroundHandlerProc* fReadHandlerProc; // if any

  AuxHandlerFunc* fAuxReadHandlerFunc;
  void* fAuxReadHandlerClientData;
};

#endif
