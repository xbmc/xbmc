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
// C++ header

#ifndef _DIGEST_AUTHENTICATION_HH
#define _DIGEST_AUTHENTICATION_HH

#ifndef _BOOLEAN_HH
#include <Boolean.hh>
#endif

// A class used for digest authentication.
// The "realm", and "nonce" fields are supplied by the server
// (in a "401 Unauthorized" response).
// The "username" and "password" fields are supplied by the client.
class Authenticator {
public:
  Authenticator();
  Authenticator(const Authenticator& orig);
  Authenticator& operator=(const Authenticator& rightSide);
  virtual ~Authenticator();

  void reset();
  void setRealmAndNonce(char const* realm, char const* nonce);
  void setRealmAndRandomNonce(char const* realm);
      // as above, except that the nonce is created randomly.
      // (This is used by servers.)
  void setUsernameAndPassword(char const* username, char const* password,
			      Boolean passwordIsMD5 = False);
      // If "passwordIsMD5" is True, then "password" is actually the value computed
      // by md5(<username>:<realm>:<actual-password>)

  char const* realm() const { return fRealm; }
  char const* nonce() const { return fNonce; }
  char const* username() const { return fUsername; }
  char const* password() const { return fPassword; }

  char const* computeDigestResponse(char const* cmd, char const* url) const;
  void reclaimDigestResponse(char const* responseStr) const;

private:
  void resetRealmAndNonce();
  void resetUsernameAndPassword();
  void assignRealmAndNonce(char const* realm, char const* nonce);
  void assignUsernameAndPassword(char const* username, char const* password,
				 Boolean passwordIsMD5);
  void assign(char const* realm, char const* nonce,
	      char const* username, char const* password, Boolean passwordIsMD5);

private:
  char* fRealm; char* fNonce;
  char* fUsername; char* fPassword;
  Boolean fPasswordIsMD5;
};

#endif
