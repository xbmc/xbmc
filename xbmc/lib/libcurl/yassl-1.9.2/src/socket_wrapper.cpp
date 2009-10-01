/* socket_wrapper.cpp                           
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


/* The socket wrapper source implements a Socket class that hides the 
 * differences between Berkely style sockets and Windows sockets, allowing 
 * transparent TCP access.
 */


#include "runtime.hpp"
#include "socket_wrapper.hpp"

#ifndef _WIN32
    #include <errno.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/ioctl.h>
    #include <string.h>
    #include <fcntl.h>
#endif // _WIN32

#if defined(__sun) || defined(__SCO_VERSION__) || defined(__NETWARE__)
    #include <sys/filio.h>
#endif

#ifdef _WIN32
    const int SOCKET_EINVAL = WSAEINVAL;
    const int SOCKET_EWOULDBLOCK = WSAEWOULDBLOCK;
    const int SOCKET_EAGAIN = WSAEWOULDBLOCK;
#else
    const int SOCKET_EINVAL = EINVAL;
    const int SOCKET_EWOULDBLOCK = EWOULDBLOCK;
    const int SOCKET_EAGAIN = EAGAIN;
#endif // _WIN32


namespace yaSSL {


Socket::Socket(socket_t s) 
    : socket_(s), wouldBlock_(false), nonBlocking_(false)
{}


void Socket::set_fd(socket_t s)
{
    socket_ = s;
}


socket_t Socket::get_fd() const
{
    return socket_;
}


Socket::~Socket()
{
    // don't close automatically now
}


void Socket::closeSocket()
{
    if (socket_ != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(socket_);
#else
        close(socket_);
#endif
        socket_ = INVALID_SOCKET;
    }
}


uint Socket::get_ready() const
{
#ifdef _WIN32
    unsigned long ready = 0;
    ioctlsocket(socket_, FIONREAD, &ready);
#else
    /*
       64-bit Solaris requires the variable passed to
       FIONREAD be a 32-bit value.
    */
    unsigned int ready = 0;
    ioctl(socket_, FIONREAD, &ready);
#endif

    return ready;
}


uint Socket::send(const byte* buf, unsigned int sz, int flags) const
{
    const byte* pos = buf;
    const byte* end = pos + sz;

    while (pos != end) {
        int sent = ::send(socket_, reinterpret_cast<const char *>(pos),
                          static_cast<int>(end - pos), flags);

        if (sent == -1)
            return 0;

        pos += sent;
    }

    return sz;
}


uint Socket::receive(byte* buf, unsigned int sz, int flags)
{
    wouldBlock_ = false;

    int recvd = ::recv(socket_, reinterpret_cast<char *>(buf), sz, flags);

    // idea to seperate error from would block by arnetheduck@gmail.com
    if (recvd == -1) {
        if (get_lastError() == SOCKET_EWOULDBLOCK || 
            get_lastError() == SOCKET_EAGAIN) {
            wouldBlock_  = true; // would have blocked this time only
            nonBlocking_ = true; // socket nonblocking, win32 only way to tell
            return 0;
        }
    }
    else if (recvd == 0)
        return static_cast<uint>(-1);

    return recvd;
}


// wait if blocking for input, return false for error
bool Socket::wait()
{
    byte b;
    return receive(&b, 1, MSG_PEEK) != static_cast<uint>(-1);
}


void Socket::shutDown(int how)
{
    shutdown(socket_, how);
}


int Socket::get_lastError()
{
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}


bool Socket::WouldBlock() const
{
    return wouldBlock_;
}


bool Socket::IsNonBlocking() const
{
    return nonBlocking_;
}


void Socket::set_lastError(int errorCode)
{
#ifdef _WIN32
    WSASetLastError(errorCode);
#else
    errno = errorCode;
#endif
}


} // namespace
