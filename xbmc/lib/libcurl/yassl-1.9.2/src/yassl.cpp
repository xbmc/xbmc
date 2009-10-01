/* yassl.cpp                                
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


/* yaSSL implements external API
 */

#include "runtime.hpp"
#include "yassl.hpp"
#include "yassl_int.hpp"
#include "handshake.hpp"
#include <stdio.h>

#include "openssl/ssl.h"  // get rid of this



namespace yaSSL {



struct Base {
    SSL_METHOD* method_;
    SSL_CTX*    ctx_;
    SSL*        ssl_;

    char*       ca_;
    char*       cert_;
    char*       key_;

    DH*         dh_;

    Base() : method_(0), ctx_(0), ssl_(0), ca_(0), cert_(0), key_(0), dh_(0)
    {}

    ~Base()
    {
        if (dh_) DH_free(dh_);
        delete[] key_;
        delete[] cert_;
        delete[] ca_;
        SSL_CTX_free(ctx_);   // frees method_ too
        SSL_free(ssl_);
    }
};


void SetDH(Base&);

void SetUpBase(Base& base, ConnectionEnd end, SOCKET_T s)
{
    base.method_ = new SSL_METHOD(end, ProtocolVersion(3,1));
    base.ctx_ =    new SSL_CTX(base.method_);

    if (base.ca_)
        if (SSL_CTX_load_verify_locations(base.ctx_,
            base.ca_, 0) != SSL_SUCCESS) assert(0);
    if (base.cert_)
        if (SSL_CTX_use_certificate_file(base.ctx_,
            base.cert_, SSL_FILETYPE_PEM) != SSL_SUCCESS) assert(0);
    if (base.key_)
        if (SSL_CTX_use_PrivateKey_file(base.ctx_, base.key_,
            SSL_FILETYPE_PEM) != SSL_SUCCESS) assert(0);

    if (end == server_end) SetDH(base);

    base.ssl_ = new SSL(base.ctx_);
    base.ssl_->useSocket().set_fd(s);
}


void SetDH(Base& base)
{
    static unsigned char dh512_p[] =
    {
      0xDA,0x58,0x3C,0x16,0xD9,0x85,0x22,0x89,0xD0,0xE4,0xAF,0x75,
      0x6F,0x4C,0xCA,0x92,0xDD,0x4B,0xE5,0x33,0xB8,0x04,0xFB,0x0F,
      0xED,0x94,0xEF,0x9C,0x8A,0x44,0x03,0xED,0x57,0x46,0x50,0xD3,
      0x69,0x99,0xDB,0x29,0xD7,0x76,0x27,0x6B,0xA2,0xD3,0xD4,0x12,
      0xE2,0x18,0xF4,0xDD,0x1E,0x08,0x4C,0xF6,0xD8,0x00,0x3E,0x7C,
      0x47,0x74,0xE8,0x33,
    };

    static unsigned char dh512_g[] =
    {
      0x02,
    };

    if ( (base.dh_ = DH_new()) ) {
        base.dh_->p = BN_bin2bn(dh512_p, sizeof(dh512_p), 0);
        base.dh_->g = BN_bin2bn(dh512_g, sizeof(dh512_g), 0);
    }
    if (!base.dh_->p || !base.dh_->g) {
        DH_free(base.dh_);
        base.dh_ = 0;
    }
    SSL_CTX_set_tmp_dh(base.ctx_, base.dh_);
}


void NewCopy(char*& dst, const char* src)
{
    size_t len = strlen(src) + 1;
    dst = new char[len];

    strncpy(dst, src, len);
}


// Client Implementation
struct Client::ClientImpl {
    Base base_;
};


Client::Client() : pimpl_(new ClientImpl)
{}


Client::~Client() { delete pimpl_; }


int Client::Connect(SOCKET_T s)
{
    SetUpBase(pimpl_->base_, client_end, s);
    return SSL_connect(pimpl_->base_.ssl_);
}


int Client::Write(const void* buffer, int sz)
{
    return sendData(*pimpl_->base_.ssl_, buffer, sz);
}


int Client::Read(void* buffer, int sz)
{
    Data data(min(sz, MAX_RECORD_SIZE), static_cast<opaque*>(buffer));
    return receiveData(*pimpl_->base_.ssl_, data);
}


void Client::SetCA(const char* name)
{
    NewCopy(pimpl_->base_.ca_, name);
}


void Client::SetCert(const char* name)
{
    NewCopy(pimpl_->base_.cert_, name);
}


void Client::SetKey(const char* name)
{
    NewCopy(pimpl_->base_.key_, name);
}



// Server Implementation
struct Server::ServerImpl {
    Base base_;
};


Server::Server() : pimpl_(new ServerImpl)
{}


Server::~Server() { delete pimpl_; }


int Server::Accept(SOCKET_T s)
{
    SetUpBase(pimpl_->base_, server_end, s);
    return SSL_accept(pimpl_->base_.ssl_);
}


int Server::Write(const void* buffer, int sz)
{
    return sendData(*pimpl_->base_.ssl_, buffer, sz);
}


int Server::Read(void* buffer, int sz)
{
    Data data(min(sz, MAX_RECORD_SIZE), static_cast<opaque*>(buffer));
    return receiveData(*pimpl_->base_.ssl_, data);
}


void Server::SetCA(const char* name)
{
    NewCopy(pimpl_->base_.ca_, name);
}


void Server::SetCert(const char* name)
{
    NewCopy(pimpl_->base_.cert_, name);
}


void Server::SetKey(const char* name)
{
    NewCopy(pimpl_->base_.key_, name);
}



} // namespace yaSSL
