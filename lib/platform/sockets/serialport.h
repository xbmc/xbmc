#pragma once
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
#include "../util/buffer.h"

#include <string>
#include <stdint.h>

#if !defined(__WINDOWS__)
#include <termios.h>
#endif

#include "socket.h"

namespace PLATFORM
{
  enum SerialParity
  {
    SERIAL_PARITY_NONE = 0,
    SERIAL_PARITY_EVEN,
    SERIAL_PARITY_ODD
  };

  enum SerialStopBits
  {
    SERIAL_STOP_BITS_ONE = 1,
    SERIAL_STOP_BITS_TWO = 2
  };

  enum SerialDataBits
  {
    SERIAL_DATA_BITS_FIVE  = 5,
    SERIAL_DATA_BITS_SIX   = 6,
    SERIAL_DATA_BITS_SEVEN = 7,
    SERIAL_DATA_BITS_EIGHT = 8
  };

  class CSerialSocket : public CCommonSocket<serial_socket_t>
  {
    public:
      CSerialSocket(const CStdString &strName, uint32_t iBaudrate, SerialDataBits iDatabits = SERIAL_DATA_BITS_EIGHT, SerialStopBits iStopbits = SERIAL_STOP_BITS_ONE, SerialParity iParity = SERIAL_PARITY_NONE) :
          CCommonSocket<serial_socket_t>(INVALID_SERIAL_SOCKET_VALUE, strName),
          m_bIsOpen(false),
          m_iBaudrate(iBaudrate),
          m_iDatabits(iDatabits),
          m_iStopbits(iStopbits),
          m_iParity(iParity) {}

      virtual ~CSerialSocket(void) {}

      virtual bool Open(uint64_t iTimeoutMs = 0);
      virtual void Close(void);
      virtual void Shutdown(void);
      virtual ssize_t Write(void* data, size_t len);
      virtual ssize_t Read(void* data, size_t len, uint64_t iTimeoutMs = 0);

      virtual bool IsOpen(void)
      {
        return m_socket != INVALID_SERIAL_SOCKET_VALUE &&
            m_bIsOpen;
      }

      virtual bool SetBaudRate(uint32_t baudrate);

    protected:
  #ifndef __WINDOWS__
      struct termios  m_options;
  #endif

      bool            m_bIsOpen;
      uint32_t        m_iBaudrate;
      SerialDataBits  m_iDatabits;
      SerialStopBits  m_iStopbits;
      SerialParity    m_iParity;
  };

  class CSerialPort : public CProtectedSocket<CSerialSocket>
  {
  public:
    CSerialPort(const CStdString &strName, uint32_t iBaudrate, SerialDataBits iDatabits = SERIAL_DATA_BITS_EIGHT, SerialStopBits iStopbits = SERIAL_STOP_BITS_ONE, SerialParity iParity = SERIAL_PARITY_NONE) :
      CProtectedSocket<CSerialSocket> (new CSerialSocket(strName, iBaudrate, iDatabits, iStopbits, iParity)) {}
    virtual ~CSerialPort(void) {}
  };
};
