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
// Copyright (c) 1996-2010 Live Networks, Inc.  All rights reserved.
// 'Group sockets'
// Implementation

#include "Groupsock.hh"
#include "GroupsockHelper.hh"
//##### Eventually fix the following #include; we shouldn't know about tunnels
#include "TunnelEncaps.hh"

#ifndef NO_SSTREAM
#include <sstream>
#endif
#include <stdio.h>

///////// OutputSocket //////////

OutputSocket::OutputSocket(UsageEnvironment& env)
  : Socket(env, 0 /* let kernel choose port */),
    fSourcePort(0), fLastSentTTL(0) {
}

OutputSocket::OutputSocket(UsageEnvironment& env, Port port)
  : Socket(env, port),
    fSourcePort(0), fLastSentTTL(0) {
}

OutputSocket::~OutputSocket() {
}

Boolean OutputSocket::write(netAddressBits address, Port port, u_int8_t ttl,
			    unsigned char* buffer, unsigned bufferSize) {
  if (ttl == fLastSentTTL) {
    // Optimization: So we don't do a 'set TTL' system call again
    ttl = 0;
  } else {
    fLastSentTTL = ttl;
  }
  struct in_addr destAddr; destAddr.s_addr = address;
  if (!writeSocket(env(), socketNum(), destAddr, port, ttl,
		   buffer, bufferSize))
    return False;

  if (sourcePortNum() == 0) {
    // Now that we've sent a packet, we can find out what the
    // kernel chose as our ephemeral source port number:
    if (!getSourcePort(env(), socketNum(), fSourcePort)) {
      if (DebugLevel >= 1)
	env() << *this
	     << ": failed to get source port: "
	     << env().getResultMsg() << "\n";
      return False;
    }
  }

  return True;
}

// By default, we don't do reads:
Boolean OutputSocket
::handleRead(unsigned char* /*buffer*/, unsigned /*bufferMaxSize*/,
	     unsigned& /*bytesRead*/, struct sockaddr_in& /*fromAddress*/) {
  return True;
}


///////// destRecord //////////

destRecord
::destRecord(struct in_addr const& addr, Port const& port, u_int8_t ttl,
	     destRecord* next)
  : fNext(next), fGroupEId(addr, port.num(), ttl), fPort(port) {
}

destRecord::~destRecord() {
  delete fNext;
}


///////// Groupsock //////////

NetInterfaceTrafficStats Groupsock::statsIncoming;
NetInterfaceTrafficStats Groupsock::statsOutgoing;
NetInterfaceTrafficStats Groupsock::statsRelayedIncoming;
NetInterfaceTrafficStats Groupsock::statsRelayedOutgoing;

// Constructor for a source-independent multicast group
Groupsock::Groupsock(UsageEnvironment& env, struct in_addr const& groupAddr,
		     Port port, u_int8_t ttl)
  : OutputSocket(env, port),
    deleteIfNoMembers(False), isSlave(False),
    fIncomingGroupEId(groupAddr, port.num(), ttl), fDests(NULL), fTTL(ttl) {
  addDestination(groupAddr, port);

  if (!socketJoinGroup(env, socketNum(), groupAddr.s_addr)) {
    if (DebugLevel >= 1) {
      env << *this << ": failed to join group: "
	  << env.getResultMsg() << "\n";
    }
  }

  // Make sure we can get our source address:
  if (ourIPAddress(env) == 0) {
    if (DebugLevel >= 0) { // this is a fatal error
      env << "Unable to determine our source address: "
	  << env.getResultMsg() << "\n";
    }
  }

  if (DebugLevel >= 2) env << *this << ": created\n";
}

// Constructor for a source-specific multicast group
Groupsock::Groupsock(UsageEnvironment& env, struct in_addr const& groupAddr,
		     struct in_addr const& sourceFilterAddr,
		     Port port)
  : OutputSocket(env, port),
    deleteIfNoMembers(False), isSlave(False),
    fIncomingGroupEId(groupAddr, sourceFilterAddr, port.num()),
    fDests(NULL), fTTL(255) {
  addDestination(groupAddr, port);

  // First try a SSM join.  If that fails, try a regular join:
  if (!socketJoinGroupSSM(env, socketNum(), groupAddr.s_addr,
			  sourceFilterAddr.s_addr)) {
    if (DebugLevel >= 3) {
      env << *this << ": SSM join failed: "
	  << env.getResultMsg();
      env << " - trying regular join instead\n";
    }
    if (!socketJoinGroup(env, socketNum(), groupAddr.s_addr)) {
      if (DebugLevel >= 1) {
	env << *this << ": failed to join group: "
	     << env.getResultMsg() << "\n";
      }
    }
  }

  if (DebugLevel >= 2) env << *this << ": created\n";
}

Groupsock::~Groupsock() {
  if (isSSM()) {
    if (!socketLeaveGroupSSM(env(), socketNum(), groupAddress().s_addr,
			     sourceFilterAddress().s_addr)) {
      socketLeaveGroup(env(), socketNum(), groupAddress().s_addr);
    }
  } else {
    socketLeaveGroup(env(), socketNum(), groupAddress().s_addr);
  }

  delete fDests;

  if (DebugLevel >= 2) env() << *this << ": deleting\n";
}

void
Groupsock::changeDestinationParameters(struct in_addr const& newDestAddr,
				       Port newDestPort, int newDestTTL) {
  if (fDests == NULL) return;

  struct in_addr destAddr = fDests->fGroupEId.groupAddress();
  if (newDestAddr.s_addr != 0) {
    if (newDestAddr.s_addr != destAddr.s_addr
	&& IsMulticastAddress(newDestAddr.s_addr)) {
      // If the new destination is a multicast address, then we assume that
      // we want to join it also.  (If this is not in fact the case, then
      // call "multicastSendOnly()" afterwards.)
      socketLeaveGroup(env(), socketNum(), destAddr.s_addr);
      socketJoinGroup(env(), socketNum(), newDestAddr.s_addr);
    }
    destAddr.s_addr = newDestAddr.s_addr;
  }

  portNumBits destPortNum = fDests->fGroupEId.portNum();
  if (newDestPort.num() != 0) {
    if (newDestPort.num() != destPortNum
	&& IsMulticastAddress(destAddr.s_addr)) {
      // Also bind to the new port number:
      changePort(newDestPort);
      // And rejoin the multicast group:
      socketJoinGroup(env(), socketNum(), destAddr.s_addr);
    }
    destPortNum = newDestPort.num();
    fDests->fPort = newDestPort;
  }

  u_int8_t destTTL = ttl();
  if (newDestTTL != ~0) destTTL = (u_int8_t)newDestTTL;

  fDests->fGroupEId = GroupEId(destAddr, destPortNum, destTTL);
}

void Groupsock::addDestination(struct in_addr const& addr, Port const& port) {
  // Check whether this destination is already known:
  for (destRecord* dests = fDests; dests != NULL; dests = dests->fNext) {
    if (addr.s_addr == dests->fGroupEId.groupAddress().s_addr
	&& port.num() == dests->fPort.num()) {
      return;
    }
  }

  fDests = new destRecord(addr, port, ttl(), fDests);
}

void Groupsock::removeDestination(struct in_addr const& addr, Port const& port) {
  for (destRecord** destsPtr = &fDests; *destsPtr != NULL;
       destsPtr = &((*destsPtr)->fNext)) {
    if (addr.s_addr == (*destsPtr)->fGroupEId.groupAddress().s_addr
	&& port.num() == (*destsPtr)->fPort.num()) {
      // Remove the record pointed to by *destsPtr :
      destRecord* next = (*destsPtr)->fNext;
      (*destsPtr)->fNext = NULL;
      delete (*destsPtr);
      *destsPtr = next;
      return;
    }
  }
}

void Groupsock::removeAllDestinations() {
  delete fDests; fDests = NULL;
}

void Groupsock::multicastSendOnly() {
  socketLeaveGroup(env(), socketNum(), fIncomingGroupEId.groupAddress().s_addr);
  for (destRecord* dests = fDests; dests != NULL; dests = dests->fNext) {
    socketLeaveGroup(env(), socketNum(), dests->fGroupEId.groupAddress().s_addr);
  }
}

Boolean Groupsock::output(UsageEnvironment& env, u_int8_t ttlToSend,
			  unsigned char* buffer, unsigned bufferSize,
			  DirectedNetInterface* interfaceNotToFwdBackTo) {
  do {
    // First, do the datagram send, to each destination:
    Boolean writeSuccess = True;
    for (destRecord* dests = fDests; dests != NULL; dests = dests->fNext) {
      if (!write(dests->fGroupEId.groupAddress().s_addr, dests->fPort, ttlToSend,
		 buffer, bufferSize)) {
	writeSuccess = False;
	break;
      }
    }
    if (!writeSuccess) break;
    statsOutgoing.countPacket(bufferSize);
    statsGroupOutgoing.countPacket(bufferSize);

    // Then, forward to our members:
    int numMembers = 0;
    if (!members().IsEmpty()) {
      numMembers =
	outputToAllMembersExcept(interfaceNotToFwdBackTo,
				 ttlToSend, buffer, bufferSize,
				 ourIPAddress(env));
      if (numMembers < 0) break;
    }

    if (DebugLevel >= 3) {
      env << *this << ": wrote " << bufferSize << " bytes, ttl "
	  << (unsigned)ttlToSend;
      if (numMembers > 0) {
	env << "; relayed to " << numMembers << " members";
      }
      env << "\n";
    }
    return True;
  } while (0);

  if (DebugLevel >= 0) { // this is a fatal error
    env.setResultMsg("Groupsock write failed: ", env.getResultMsg());
  }
  return False;
}

Boolean Groupsock::handleRead(unsigned char* buffer, unsigned bufferMaxSize,
			      unsigned& bytesRead,
			      struct sockaddr_in& fromAddress) {
  // Read data from the socket, and relay it across any attached tunnels
  //##### later make this code more general - independent of tunnels

  bytesRead = 0;

  int maxBytesToRead = bufferMaxSize - TunnelEncapsulationTrailerMaxSize;
  int numBytes = readSocket(env(), socketNum(),
			    buffer, maxBytesToRead, fromAddress);
  if (numBytes < 0) {
    if (DebugLevel >= 0) { // this is a fatal error
      env().setResultMsg("Groupsock read failed: ",
			 env().getResultMsg());
    }
    return False;
  }

  // If we're a SSM group, make sure the source address matches:
  if (isSSM()
      && fromAddress.sin_addr.s_addr != sourceFilterAddress().s_addr) {
    return True;
  }

  // We'll handle this data.
  // Also write it (with the encapsulation trailer) to each member,
  // unless the packet was originally sent by us to begin with.
  bytesRead = numBytes;

  int numMembers = 0;
  if (!wasLoopedBackFromUs(env(), fromAddress)) {
    statsIncoming.countPacket(numBytes);
    statsGroupIncoming.countPacket(numBytes);
    numMembers =
      outputToAllMembersExcept(NULL, ttl(),
			       buffer, bytesRead,
			       fromAddress.sin_addr.s_addr);
    if (numMembers > 0) {
      statsRelayedIncoming.countPacket(numBytes);
      statsGroupRelayedIncoming.countPacket(numBytes);
    }
  }
  if (DebugLevel >= 3) {
    env() << *this << ": read " << bytesRead << " bytes from ";
    env() << our_inet_ntoa(fromAddress.sin_addr);
    if (numMembers > 0) {
      env() << "; relayed to " << numMembers << " members";
    }
    env() << "\n";
  }

  return True;
}

Boolean Groupsock::wasLoopedBackFromUs(UsageEnvironment& env,
				       struct sockaddr_in& fromAddress) {
  if (fromAddress.sin_addr.s_addr
      == ourIPAddress(env)) {
    if (fromAddress.sin_port == sourcePortNum()) {
#ifdef DEBUG_LOOPBACK_CHECKING
      if (DebugLevel >= 3) {
	env() << *this << ": got looped-back packet\n";
      }
#endif
      return True;
    }
  }

  return False;
}

int Groupsock::outputToAllMembersExcept(DirectedNetInterface* exceptInterface,
					u_int8_t ttlToFwd,
					unsigned char* data, unsigned size,
					netAddressBits sourceAddr) {
  // Don't forward TTL-0 packets
  if (ttlToFwd == 0) return 0;

  DirectedNetInterfaceSet::Iterator iter(members());
  unsigned numMembers = 0;
  DirectedNetInterface* interf;
  while ((interf = iter.next()) != NULL) {
    // Check whether we've asked to exclude this interface:
    if (interf == exceptInterface)
      continue;

    // Check that the packet's source address makes it OK to
    // be relayed across this interface:
    UsageEnvironment& saveEnv = env();
    // because the following call may delete "this"
    if (!interf->SourceAddrOKForRelaying(saveEnv, sourceAddr)) {
      if (strcmp(saveEnv.getResultMsg(), "") != 0) {
				// Treat this as a fatal error
	return -1;
      } else {
	continue;
      }
    }

    if (numMembers == 0) {
      // We know that we're going to forward to at least one
      // member, so fill in the tunnel encapsulation trailer.
      // (Note: Allow for it not being 4-byte-aligned.)
      TunnelEncapsulationTrailer* trailerInPacket
	= (TunnelEncapsulationTrailer*)&data[size];
      TunnelEncapsulationTrailer* trailer;

      Boolean misaligned = ((unsigned long)trailerInPacket & 3) != 0;
      unsigned trailerOffset;
      u_int8_t tunnelCmd;
      if (isSSM()) {
	// add an 'auxilliary address' before the trailer
	trailerOffset = TunnelEncapsulationTrailerAuxSize;
	tunnelCmd = TunnelDataAuxCmd;
      } else {
	trailerOffset = 0;
	tunnelCmd = TunnelDataCmd;
      }
      unsigned trailerSize = TunnelEncapsulationTrailerSize + trailerOffset;
      unsigned tmpTr[TunnelEncapsulationTrailerMaxSize];
      if (misaligned) {
	trailer = (TunnelEncapsulationTrailer*)&tmpTr;
      } else {
	trailer = trailerInPacket;
      }
      trailer += trailerOffset;

      if (fDests != NULL) {
	trailer->address() = fDests->fGroupEId.groupAddress().s_addr;
	trailer->port() = fDests->fPort; // structure copy, outputs in network order
      }
      trailer->ttl() = ttlToFwd;
      trailer->command() = tunnelCmd;

      if (isSSM()) {
	trailer->auxAddress() = sourceFilterAddress().s_addr;
      }

      if (misaligned) {
	memmove(trailerInPacket, trailer-trailerOffset, trailerSize);
      }

      size += trailerSize;
    }

    interf->write(data, size);
    ++numMembers;
  }

  return numMembers;
}

UsageEnvironment& operator<<(UsageEnvironment& s, const Groupsock& g) {
  UsageEnvironment& s1 = s << timestampString() << " Groupsock("
			   << g.socketNum() << ": "
			   << our_inet_ntoa(g.groupAddress())
			   << ", " << g.port() << ", ";
  if (g.isSSM()) {
    return s1 << "SSM source: "
	      <<  our_inet_ntoa(g.sourceFilterAddress()) << ")";
  } else {
    return s1 << (unsigned)(g.ttl()) << ")";
  }
}


////////// GroupsockLookupTable //////////


// A hash table used to index Groupsocks by socket number.

static HashTable* getSocketTable(UsageEnvironment& env) {
  if (env.groupsockPriv == NULL) { // We need to create it
    env.groupsockPriv = HashTable::create(ONE_WORD_HASH_KEYS);
  }
  return (HashTable*)(env.groupsockPriv);
}

static Boolean unsetGroupsockBySocket(Groupsock const* groupsock) {
  do {
    if (groupsock == NULL) break;

    int sock = groupsock->socketNum();
    // Make sure "sock" is in bounds:
    if (sock < 0) break;

    HashTable* sockets = getSocketTable(groupsock->env());
    if (sockets == NULL) break;

    Groupsock* gs = (Groupsock*)sockets->Lookup((char*)(long)sock);
    if (gs == NULL || gs != groupsock) break;
    sockets->Remove((char*)(long)sock);

    if (sockets->IsEmpty()) {
      // We can also delete the table (to reclaim space):
      delete sockets;
      (gs->env()).groupsockPriv = NULL;
    }

    return True;
  } while (0);

  return False;
}

static Boolean setGroupsockBySocket(UsageEnvironment& env, int sock,
				    Groupsock* groupsock) {
  do {
    // Make sure the "sock" parameter is in bounds:
    if (sock < 0) {
      char buf[100];
      sprintf(buf, "trying to use bad socket (%d)", sock);
      env.setResultMsg(buf);
      break;
    }

    HashTable* sockets = getSocketTable(env);
    if (sockets == NULL) break;

    // Make sure we're not replacing an existing Groupsock
    // That shouldn't happen
    Boolean alreadyExists
      = (sockets->Lookup((char*)(long)sock) != 0);
    if (alreadyExists) {
      char buf[100];
      sprintf(buf,
	      "Attempting to replace an existing socket (%d",
	      sock);
      env.setResultMsg(buf);
      break;
    }

    sockets->Add((char*)(long)sock, groupsock);
    return True;
  } while (0);

  return False;
}

static Groupsock* getGroupsockBySocket(UsageEnvironment& env, int sock) {
  do {
    // Make sure the "sock" parameter is in bounds:
    if (sock < 0) break;

    HashTable* sockets = getSocketTable(env);
    if (sockets == NULL) break;

    return (Groupsock*)sockets->Lookup((char*)(long)sock);
  } while (0);

  return NULL;
}

Groupsock*
GroupsockLookupTable::Fetch(UsageEnvironment& env,
			    netAddressBits groupAddress,
			    Port port, u_int8_t ttl,
			    Boolean& isNew) {
  isNew = False;
  Groupsock* groupsock;
  do {
    groupsock = (Groupsock*) fTable.Lookup(groupAddress, (~0), port);
    if (groupsock == NULL) { // we need to create one:
      groupsock = AddNew(env, groupAddress, (~0), port, ttl);
      if (groupsock == NULL) break;
      isNew = True;
    }
  } while (0);

  return groupsock;
}

Groupsock*
GroupsockLookupTable::Fetch(UsageEnvironment& env,
			    netAddressBits groupAddress,
			    netAddressBits sourceFilterAddr, Port port,
			    Boolean& isNew) {
  isNew = False;
  Groupsock* groupsock;
  do {
    groupsock
      = (Groupsock*) fTable.Lookup(groupAddress, sourceFilterAddr, port);
    if (groupsock == NULL) { // we need to create one:
      groupsock = AddNew(env, groupAddress, sourceFilterAddr, port, 0);
      if (groupsock == NULL) break;
      isNew = True;
    }
  } while (0);

  return groupsock;
}

Groupsock*
GroupsockLookupTable::Lookup(netAddressBits groupAddress, Port port) {
  return (Groupsock*) fTable.Lookup(groupAddress, (~0), port);
}

Groupsock*
GroupsockLookupTable::Lookup(netAddressBits groupAddress,
			     netAddressBits sourceFilterAddr, Port port) {
  return (Groupsock*) fTable.Lookup(groupAddress, sourceFilterAddr, port);
}

Groupsock* GroupsockLookupTable::Lookup(UsageEnvironment& env, int sock) {
  return getGroupsockBySocket(env, sock);
}

Boolean GroupsockLookupTable::Remove(Groupsock const* groupsock) {
  unsetGroupsockBySocket(groupsock);
  return fTable.Remove(groupsock->groupAddress().s_addr,
		       groupsock->sourceFilterAddress().s_addr,
		       groupsock->port());
}

Groupsock* GroupsockLookupTable::AddNew(UsageEnvironment& env,
					netAddressBits groupAddress,
					netAddressBits sourceFilterAddress,
					Port port, u_int8_t ttl) {
  Groupsock* groupsock;
  do {
    struct in_addr groupAddr; groupAddr.s_addr = groupAddress;
    if (sourceFilterAddress == netAddressBits(~0)) {
      // regular, ISM groupsock
      groupsock = new Groupsock(env, groupAddr, port, ttl);
    } else {
      // SSM groupsock
      struct in_addr sourceFilterAddr;
      sourceFilterAddr.s_addr = sourceFilterAddress;
      groupsock = new Groupsock(env, groupAddr, sourceFilterAddr, port);
    }

    if (groupsock == NULL || groupsock->socketNum() < 0) break;

    if (!setGroupsockBySocket(env, groupsock->socketNum(), groupsock)) break;

    fTable.Add(groupAddress, sourceFilterAddress, port, (void*)groupsock);
  } while (0);

  return groupsock;
}

GroupsockLookupTable::Iterator::Iterator(GroupsockLookupTable& groupsocks)
  : fIter(AddressPortLookupTable::Iterator(groupsocks.fTable)) {
}

Groupsock* GroupsockLookupTable::Iterator::next() {
  return (Groupsock*) fIter.next();
};
