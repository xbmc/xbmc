/* yassl.hpp                                
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
 * yaSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */


/* yaSSL externel header defines yaSSL API
 */


#ifndef yaSSL_EXT_HPP
#define yaSSL_EXT_HPP


namespace yaSSL {


#ifdef _WIN32
    typedef unsigned int SOCKET_T;
#else
    typedef int          SOCKET_T;
#endif


class Client {
public:
    Client();
    ~Client();

    // basics
    int Connect(SOCKET_T);
    int Write(const void*, int);
    int Read(void*, int);

    // options
    void SetCA(const char*);
    void SetCert(const char*);
    void SetKey(const char*);
private:
    struct ClientImpl;
    ClientImpl* pimpl_;

    Client(const Client&);              // hide copy
    Client& operator=(const Client&);   // and assign  
};


class Server {
public:
    Server();
    ~Server();

    // basics
    int Accept(SOCKET_T);
    int Write(const void*, int);
    int Read(void*, int);

    // options
    void SetCA(const char*);
    void SetCert(const char*);
    void SetKey(const char*);
private:
    struct ServerImpl;
    ServerImpl* pimpl_;

    Server(const Server&);              // hide copy
    Server& operator=(const Server&);   // and assign
};


} // namespace yaSSL
#endif // yaSSL_EXT_HPP
