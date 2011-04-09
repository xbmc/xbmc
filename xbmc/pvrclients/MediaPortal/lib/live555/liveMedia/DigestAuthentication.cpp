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
// A class used for digest authentication.
// Implementation

#include "DigestAuthentication.hh"
#include "our_md5.h"
#include <strDup.hh>
#include <GroupsockHelper.hh> // for gettimeofday()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Authenticator::Authenticator() {
  assign(NULL, NULL, NULL, NULL, False);
}

Authenticator::Authenticator(const Authenticator& orig) {
  assign(orig.realm(), orig.nonce(), orig.username(), orig.password(),
	 orig.fPasswordIsMD5);
}

Authenticator& Authenticator::operator=(const Authenticator& rightSide) {
  if (&rightSide != this) {
    reset();
    assign(rightSide.realm(), rightSide.nonce(),
	   rightSide.username(), rightSide.password(), rightSide.fPasswordIsMD5);
  }

  return *this;
}

Authenticator::~Authenticator() {
  reset();
}

void Authenticator::reset() {
  resetRealmAndNonce();
  resetUsernameAndPassword();
}

void Authenticator::setRealmAndNonce(char const* realm, char const* nonce) {
  resetRealmAndNonce();
  assignRealmAndNonce(realm, nonce);
}

void Authenticator::setRealmAndRandomNonce(char const* realm) {
  resetRealmAndNonce();

  // Construct data to seed the random nonce:
  struct {
    struct timeval timestamp;
    unsigned counter;
  } seedData;
  gettimeofday(&seedData.timestamp, NULL);
  static unsigned counter = 0;
  seedData.counter = ++counter;

  // Use MD5 to compute a 'random' nonce from this seed data:
  char nonceBuf[33];
  our_MD5Data((unsigned char*)(&seedData), sizeof seedData, nonceBuf);

  assignRealmAndNonce(realm, nonceBuf);
}

void Authenticator::setUsernameAndPassword(char const* username,
					   char const* password,
					   Boolean passwordIsMD5) {
  resetUsernameAndPassword();
  assignUsernameAndPassword(username, password, passwordIsMD5);
}

char const* Authenticator::computeDigestResponse(char const* cmd,
						 char const* url) const {
  // The "response" field is computed as:
  //    md5(md5(<username>:<realm>:<password>):<nonce>:md5(<cmd>:<url>))
  // or, if "fPasswordIsMD5" is True:
  //    md5(<password>:<nonce>:md5(<cmd>:<url>))
  char ha1Buf[33];
  if (fPasswordIsMD5) {
    strncpy(ha1Buf, password(), 32);
    ha1Buf[32] = '\0'; // just in case
  } else {
    unsigned const ha1DataLen = strlen(username()) + 1
      + strlen(realm()) + 1 + strlen(password());
    unsigned char* ha1Data = new unsigned char[ha1DataLen+1];
    sprintf((char*)ha1Data, "%s:%s:%s", username(), realm(), password());
    our_MD5Data(ha1Data, ha1DataLen, ha1Buf);
    delete[] ha1Data;
  }

  unsigned const ha2DataLen = strlen(cmd) + 1 + strlen(url);
  unsigned char* ha2Data = new unsigned char[ha2DataLen+1];
  sprintf((char*)ha2Data, "%s:%s", cmd, url);
  char ha2Buf[33];
  our_MD5Data(ha2Data, ha2DataLen, ha2Buf);
  delete[] ha2Data;

  unsigned const digestDataLen
    = 32 + 1 + strlen(nonce()) + 1 + 32;
  unsigned char* digestData = new unsigned char[digestDataLen+1];
  sprintf((char*)digestData, "%s:%s:%s",
          ha1Buf, nonce(), ha2Buf);
  char const* result = our_MD5Data(digestData, digestDataLen, NULL);
  delete[] digestData;
  return result;
}

void Authenticator::reclaimDigestResponse(char const* responseStr) const {
  free((char*)responseStr); // NOT delete, because it was malloc-allocated
}

void Authenticator::resetRealmAndNonce() {
  delete[] fRealm; fRealm = NULL;
  delete[] fNonce; fNonce = NULL;
}

void Authenticator::resetUsernameAndPassword() {
  delete[] fUsername; fUsername = NULL;
  delete[] fPassword; fPassword = NULL;
  fPasswordIsMD5 = False;
}

void Authenticator::assignRealmAndNonce(char const* realm, char const* nonce) {
  fRealm = strDup(realm);
  fNonce = strDup(nonce);
}

void Authenticator
::assignUsernameAndPassword(char const* username, char const* password,
			    Boolean passwordIsMD5) {
  fUsername = strDup(username);
  fPassword = strDup(password);
  fPasswordIsMD5 = passwordIsMD5;
}

void Authenticator::assign(char const* realm, char const* nonce,
			   char const* username, char const* password,
			   Boolean passwordIsMD5) {
  assignRealmAndNonce(realm, nonce);
  assignUsernameAndPassword(username, password, passwordIsMD5);
}
