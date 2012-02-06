/*
 * This file is part of the libCEC(R) library.
 *
 * libCEC(R) is Copyright (C) 2011-2012 Pulse-Eight Limited.  All rights reserved.
 * libCEC(R) is an original work, containing original code.
 *
 * libCEC(R) is a trademark of Pulse-Eight Limited.
 *
 * This program is dual-licensed; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * Alternatively, you can license this library under a commercial license,
 * please contact Pulse-Eight Licensing for more information.
 *
 * For more information contact:
 * Pulse-Eight Licensing       <license@pulse-eight.com>
 *     http://www.pulse-eight.com/
 *     http://www.pulse-eight.net/
 */

#include "../os.h"
#include <stdio.h>
#include <fcntl.h>
#include "../sockets/serialport.h"
#include "../util/baudrate.h"
#include "../posix/os-socket.h"

#if defined(__APPLE__)
#ifndef XCASE
#define XCASE	0
#endif
#ifndef OLCUC
#define OLCUC	0
#endif
#ifndef IUCLC
#define IUCLC	0
#endif
#endif
using namespace std;
using namespace PLATFORM;

void CSerialSocket::Close(void)
{
  SocketClose(m_socket);
}

void CSerialSocket::Shutdown(void)
{
  SocketClose(m_socket);
}

ssize_t CSerialSocket::Write(void* data, size_t len)
{
  return SocketWrite(m_socket, &m_iError, data, len);
}

ssize_t CSerialSocket::Read(void* data, size_t len, uint64_t iTimeoutMs /* = 0 */)
{
  return SocketRead(m_socket, &m_iError, data, len, iTimeoutMs);
}

//setting all this stuff up is a pain in the ass
bool CSerialSocket::Open(uint64_t iTimeoutMs /* = 0 */)
{
  iTimeoutMs = 0;
  CLockObject lock(m_mutex);

  if (m_iDatabits != SERIAL_DATA_BITS_FIVE && m_iDatabits != SERIAL_DATA_BITS_SIX &&
      m_iDatabits != SERIAL_DATA_BITS_SEVEN && m_iDatabits != SERIAL_DATA_BITS_EIGHT)
  {
    m_strError = "Databits has to be between 5 and 8";
    return false;
  }

  if (m_iStopbits != SERIAL_STOP_BITS_ONE && m_iStopbits != SERIAL_STOP_BITS_TWO)
  {
    m_strError = "Stopbits has to be 1 or 2";
    return false;
  }

  if (m_iParity != SERIAL_PARITY_NONE && m_iParity != SERIAL_PARITY_EVEN && m_iParity != SERIAL_PARITY_ODD)
  {
    m_strError = "Parity has to be none, even or odd";
    return false;
  }

  m_socket = open(m_strName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);

  if (m_socket == INVALID_SERIAL_SOCKET_VALUE)
  {
    m_strError = strerror(errno);
    return false;
  }

  SocketSetBlocking(m_socket, false);

  if (!SetBaudRate(m_iBaudrate))
    return false;

  m_options.c_cflag |= (CLOCAL | CREAD);
  m_options.c_cflag &= ~HUPCL;

  m_options.c_cflag &= ~CSIZE;
  if (m_iDatabits == SERIAL_DATA_BITS_FIVE)  m_options.c_cflag |= CS5;
  if (m_iDatabits == SERIAL_DATA_BITS_SIX)   m_options.c_cflag |= CS6;
  if (m_iDatabits == SERIAL_DATA_BITS_SEVEN) m_options.c_cflag |= CS7;
  if (m_iDatabits == SERIAL_DATA_BITS_EIGHT) m_options.c_cflag |= CS8;

  m_options.c_cflag &= ~PARENB;
  if (m_iParity == SERIAL_PARITY_EVEN || m_iParity == SERIAL_PARITY_ODD)
    m_options.c_cflag |= PARENB;
  if (m_iParity == SERIAL_PARITY_ODD)
    m_options.c_cflag |= PARODD;

#ifdef CRTSCTS
  m_options.c_cflag &= ~CRTSCTS;
#elif defined(CNEW_RTSCTS)
  m_options.c_cflag &= ~CNEW_RTSCTS;
#endif

  if (m_iStopbits == SERIAL_STOP_BITS_ONE) m_options.c_cflag &= ~CSTOPB;
  else m_options.c_cflag |= CSTOPB;
  
  //I guessed a little here
  m_options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | XCASE | ECHOK | ECHONL | ECHOCTL | ECHOPRT | ECHOKE | TOSTOP);

  if (m_iParity == SERIAL_PARITY_NONE)
    m_options.c_iflag &= ~INPCK;
  else
    m_options.c_iflag |= INPCK | ISTRIP;

  m_options.c_iflag &= ~(IXON | IXOFF | IXANY | BRKINT | INLCR | IGNCR | ICRNL | IUCLC | IMAXBEL);
  m_options.c_oflag &= ~(OPOST | ONLCR | OCRNL);

  if (tcsetattr(m_socket, TCSANOW, &m_options) != 0)
  {
    m_strError = strerror(errno);
    return false;
  }
  
  SocketSetBlocking(m_socket, true);
  m_bIsOpen = true;

  return true;
}

bool CSerialSocket::SetBaudRate(uint32_t baudrate)
{
  int rate = IntToBaudrate(baudrate);
  if (rate == -1)
  {
    char buff[255];
    sprintf(buff, "%i is not a valid baudrate", baudrate);
    m_strError = buff;
    return false;
  }

  //get the current port attributes
  if (tcgetattr(m_socket, &m_options) != 0)
  {
    m_strError = strerror(errno);
    return false;
  }

  if (cfsetispeed(&m_options, rate) != 0)
  {
    m_strError = strerror(errno);
    return false;
  }

  if (cfsetospeed(&m_options, rate) != 0)
  {
    m_strError = strerror(errno);
    return false;
  }

  return true;
}
