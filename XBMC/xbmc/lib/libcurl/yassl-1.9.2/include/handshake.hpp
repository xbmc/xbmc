/* handshake.hpp                               
 *
 * Copyright (C) 2003 Sawtooth Consulting Ltd.
 *
 * This file is part of yaSSL, an SSL implementation written by Todd A Ouska
 * (todd at yassl.com, see www.yassl.com).
 *
 * yaSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * There are special exceptions to the terms and conditions of the GPL as it
 * is applied to yaSSL. View the full text of the exception in the file
 * FLOSS-EXCEPTIONS in the directory of this software distribution.
 *
 * yaSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

/* The handshake header declares function prototypes for creating and reading
 * the various handshake messages.
 */



#ifndef yaSSL_HANDSHAKE_HPP
#define yaSSL_HANDSHAKE_HPP

#include "yassl_types.hpp"


namespace yaSSL {

// forward decls
class  SSL;
class  Finished;
class  Data;
class  Alert;
struct Hashes;

enum BufferOutput { buffered, unbuffered };

void sendClientHello(SSL&);
void sendServerHello(SSL&, BufferOutput = buffered);
void sendServerHelloDone(SSL&, BufferOutput = buffered);
void sendClientKeyExchange(SSL&, BufferOutput = buffered);
void sendServerKeyExchange(SSL&, BufferOutput = buffered);
void sendChangeCipher(SSL&, BufferOutput = buffered);
void sendFinished(SSL&, ConnectionEnd, BufferOutput = buffered);
void sendCertificate(SSL&, BufferOutput = buffered);
void sendCertificateRequest(SSL&, BufferOutput = buffered);
void sendCertificateVerify(SSL&, BufferOutput = buffered);
int  sendData(SSL&, const void*, int);
int  sendAlert(SSL& ssl, const Alert& alert);

int  receiveData(SSL&, Data&, bool peek = false); 
void processReply(SSL&);

void buildFinished(SSL&, Finished&, const opaque*);
void build_certHashes(SSL&, Hashes&);

void hmac(SSL&, byte*, const byte*, uint, ContentType, bool verify = false);
void TLS_hmac(SSL&, byte*, const byte*, uint, ContentType,
              bool verify = false);
void PRF(byte* digest, uint digLen, const byte* secret, uint secLen,
         const byte* label, uint labLen, const byte* seed, uint seedLen);

} // naemspace

#endif // yaSSL_HANDSHAKE_HPP
