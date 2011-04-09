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
// Copyright (c) 1996-2010, Live Networks, Inc.  All rights reserved
// "Group Endpoint Id"
// Implementation

#include "GroupEId.hh"
#include "strDup.hh"
#include <string.h>

////////// Scope //////////

void Scope::assign(u_int8_t ttl, const char* publicKey) {
  fTTL = ttl;

  fPublicKey = strDup(publicKey == NULL ? "nokey" : publicKey);
}

void Scope::clean() {
  delete[] fPublicKey;
  fPublicKey = NULL;
}


Scope::Scope(u_int8_t ttl, const char* publicKey) {
  assign(ttl, publicKey);
}

Scope::Scope(const Scope& orig) {
  assign(orig.ttl(), orig.publicKey());
}

Scope& Scope::operator=(const Scope& rightSide) {
  if (&rightSide != this) {
    if (publicKey() == NULL
	|| strcmp(publicKey(), rightSide.publicKey()) != 0) {
      clean();
      assign(rightSide.ttl(), rightSide.publicKey());
    } else { // need to assign TTL only
      fTTL = rightSide.ttl();
    }
  }

  return *this;
}

Scope::~Scope() {
  clean();
}

unsigned Scope::publicKeySize() const {
  return fPublicKey == NULL ? 0 : strlen(fPublicKey);
}

////////// GroupEId //////////

GroupEId::GroupEId(struct in_addr const& groupAddr,
		   portNumBits portNum, Scope const& scope,
		   unsigned numSuccessiveGroupAddrs) {
  struct in_addr sourceFilterAddr;
  sourceFilterAddr.s_addr = ~0; // indicates no source filter

  init(groupAddr, sourceFilterAddr, portNum, scope, numSuccessiveGroupAddrs);
}

GroupEId::GroupEId(struct in_addr const& groupAddr,
		   struct in_addr const& sourceFilterAddr,
		   portNumBits portNum,
		   unsigned numSuccessiveGroupAddrs) {
  init(groupAddr, sourceFilterAddr, portNum, 255, numSuccessiveGroupAddrs);
}

GroupEId::GroupEId() {
}

Boolean GroupEId::isSSM() const {
  return fSourceFilterAddress.s_addr != netAddressBits(~0);
}


void GroupEId::init(struct in_addr const& groupAddr,
		    struct in_addr const& sourceFilterAddr,
		    portNumBits portNum,
		    Scope const& scope,
		    unsigned numSuccessiveGroupAddrs) {
  fGroupAddress = groupAddr;
  fSourceFilterAddress = sourceFilterAddr;
  fNumSuccessiveGroupAddrs = numSuccessiveGroupAddrs;
  fPortNum = portNum;
  fScope = scope;
}
