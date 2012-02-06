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

#include "../sockets/serialport.h"
#include "../util/baudrate.h"
#include "../util/timeutils.h"

using namespace std;
using namespace PLATFORM;

void FormatWindowsError(int iErrorCode, CStdString &strMessage)
{
  if (iErrorCode != ERROR_SUCCESS)
  {
    char strAddMessage[1024];
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, iErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), strAddMessage, 1024, NULL);
    strMessage.append(": ");
    strMessage.append(strAddMessage);
  }
}

bool SetTimeouts(serial_socket_t socket, int* iError, bool bBlocking)
{
  if (socket == INVALID_HANDLE_VALUE)
	  return false;

  COMMTIMEOUTS cto;
  if (!GetCommTimeouts(socket, &cto))
  {
    *iError = GetLastError();
    return false;
  }

  if (bBlocking)
  {
    cto.ReadIntervalTimeout         = 0;
    cto.ReadTotalTimeoutConstant    = 0;
    cto.ReadTotalTimeoutMultiplier  = 0;
  }
  else
  {
    cto.ReadIntervalTimeout         = MAXDWORD;
    cto.ReadTotalTimeoutConstant    = 0;
    cto.ReadTotalTimeoutMultiplier  = 0;
  }

  if (!SetCommTimeouts(socket, &cto))
  {
    *iError = GetLastError();
    return false;
  }

  return true;
}

void CSerialSocket::Close(void)
{
  SerialSocketClose(m_socket);
}

void CSerialSocket::Shutdown(void)
{
  SerialSocketClose(m_socket);
}

ssize_t CSerialSocket::Write(void* data, size_t len)
{
  return SerialSocketWrite(m_socket, &m_iError, data, len);
}

ssize_t CSerialSocket::Read(void* data, size_t len, uint64_t iTimeoutMs /* = 0 */)
{
  return SerialSocketRead(m_socket, &m_iError, data, len, iTimeoutMs);
}

bool CSerialSocket::Open(uint64_t iTimeoutMs /* = 0 */)
{
  iTimeoutMs = 0;
  CStdString strComPath = "\\\\.\\" + m_strName;
  CLockObject lock(m_mutex);
  m_socket = CreateFile(strComPath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (m_socket == INVALID_HANDLE_VALUE)
  {
    m_strError = "Unable to open COM port";
    FormatWindowsError(GetLastError(), m_strError);
    return false;
  }

  COMMCONFIG commConfig = {0};
  DWORD dwSize = sizeof(commConfig);
  commConfig.dwSize = dwSize;
  if (GetDefaultCommConfig(strComPath.c_str(), &commConfig,&dwSize))
  {
    if (!SetCommConfig(m_socket, &commConfig,dwSize))
    {
      m_strError = "unable to set default config";
      FormatWindowsError(GetLastError(), m_strError);
    }
  }
  else
  {
    m_strError = "unable to get default config";
    FormatWindowsError(GetLastError(), m_strError);
  }

  if (!SetupComm(m_socket, 64, 64))
  {
    m_strError = "unable to set up the com port";
    FormatWindowsError(GetLastError(), m_strError);
  }

  if (!SetBaudRate(m_iBaudrate))
  {
    m_strError = "unable to set baud rate";
    FormatWindowsError(GetLastError(), m_strError);
    Close();
    return false;
  }

  if (!SetTimeouts(m_socket, &m_iError, false))
  {
    m_strError = "unable to set timeouts";
    FormatWindowsError(GetLastError(), m_strError);
    Close();
    return false;
  }

  m_bIsOpen = true;
  return m_bIsOpen;
}

bool CSerialSocket::SetBaudRate(uint32_t baudrate)
{
  int32_t rate = IntToBaudrate(baudrate);
  if (rate < 0)
    m_iBaudrate = baudrate > 0 ? baudrate : 0;
  else
    m_iBaudrate = rate;

  DCB dcb;
  memset(&dcb,0,sizeof(dcb));
  dcb.DCBlength = sizeof(dcb);
  dcb.BaudRate      = IntToBaudrate(m_iBaudrate);
  dcb.fBinary       = true;
  dcb.fDtrControl   = DTR_CONTROL_DISABLE;
  dcb.fRtsControl   = RTS_CONTROL_DISABLE;
  dcb.fOutxCtsFlow  = false;
  dcb.fOutxDsrFlow  = false;
  dcb.fOutX         = false;
	dcb.fInX          = false;
  dcb.fAbortOnError = true;

  if (m_iParity == SERIAL_PARITY_NONE)
    dcb.Parity = NOPARITY;
  else if (m_iParity == SERIAL_PARITY_EVEN)
    dcb.Parity = EVENPARITY;
  else
    dcb.Parity = ODDPARITY;

  if (m_iStopbits == SERIAL_STOP_BITS_TWO)
    dcb.StopBits = TWOSTOPBITS;
  else
    dcb.StopBits = ONESTOPBIT;

  dcb.ByteSize = (BYTE)m_iDatabits;

  if(!SetCommState(m_socket,&dcb))
  {
    m_strError = "SetCommState failed";
    FormatWindowsError(GetLastError(), m_strError);
    return false;
  }

  return true;
}
